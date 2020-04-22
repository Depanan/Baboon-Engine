#include "CommandBuffer.h"
#include "CommandPool.h"
#include "Core\ServiceLocator.h"
#include "Device.h"
#include <cassert>
#include "resources/RenderTarget.h"
#include "resources/DescriptorSet.h"
#include "Buffer.h"
#include "VulkanSampler.h"
#include "RenderFrame.h"


bool is_dynamic_buffer_descriptor_type(VkDescriptorType descriptor_type)
{
    return descriptor_type == VK_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC ||
        descriptor_type == VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC;
}

bool is_buffer_descriptor_type(VkDescriptorType descriptor_type)
{
    return descriptor_type == VK_DESCRIPTOR_TYPE_STORAGE_BUFFER ||
        descriptor_type == VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER ||
        is_dynamic_buffer_descriptor_type(descriptor_type);
}


CommandBuffer::CommandBuffer(CommandPool& commandPool, VkCommandBufferLevel level):
    m_Pool(commandPool),
    m_Level(level)
{
    VkCommandBufferAllocateInfo allocateInfo{ VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO };

    allocateInfo.commandPool = m_Pool.getHandle();
    allocateInfo.commandBufferCount = 1;
    allocateInfo.level = level;

    VkResult result = vkAllocateCommandBuffers(m_Pool.getDevice().get_handle(), &allocateInfo, &m_CommandBuffer);

    if (result != VK_SUCCESS)
    {
        LOGERROR("Cannot create command buffer");
    }
}


CommandBuffer::~CommandBuffer()
{
    // Destroy command buffer
    if (m_CommandBuffer != VK_NULL_HANDLE)
    {
        vkFreeCommandBuffers(m_Pool.getDevice().get_handle(), m_Pool.getHandle(), 1, &m_CommandBuffer);
    }
}


VkResult CommandBuffer::reset(ResetMode reset_mode)
{
    VkResult result = VK_SUCCESS;

    assert(reset_mode == m_Pool.getResetMode() && "Command buffer reset mode must match the one used by the pool to allocate it");

    m_State = State::Initial;

    if (reset_mode == ResetMode::ResetIndividually)
    {
        result = vkResetCommandBuffer(m_CommandBuffer, VK_COMMAND_BUFFER_RESET_RELEASE_RESOURCES_BIT);
    }

    return result;
}


VkResult CommandBuffer::begin(VkCommandBufferUsageFlags flags, CommandBuffer* primary_cmd_buf)
{
    if (isRecording())
    {
        return VK_NOT_READY;
    }
    m_State = State::Recording;

    // Reset state
    m_PipelineState.reset();
    m_ResourceBindingState.reset();
    m_DescriptorSet_Binding_State.clear();

    VkCommandBufferBeginInfo       beginInfo { VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO };
    VkCommandBufferInheritanceInfo inheritance { VK_STRUCTURE_TYPE_COMMAND_BUFFER_INHERITANCE_INFO };
    beginInfo.flags = flags;

    if (m_Level == VK_COMMAND_BUFFER_LEVEL_SECONDARY)
    {
        assert(primary_cmd_buf && "A primary command buffer pointer must be provided when calling begin from a secondary one");

        auto render_pass_binding = primary_cmd_buf->get_current_render_pass();
        m_CurrentRenderPass.render_pass = render_pass_binding.render_pass;
        m_CurrentRenderPass.framebuffer = render_pass_binding.framebuffer;

        inheritance.renderPass = m_CurrentRenderPass.render_pass->getHandle();
        inheritance.framebuffer = m_CurrentRenderPass.framebuffer->getHandle();
        inheritance.subpass = primary_cmd_buf->get_current_subpass_index();

        beginInfo.pInheritanceInfo = &inheritance;
    }

    return vkBeginCommandBuffer(m_CommandBuffer, &beginInfo);
}

VkResult CommandBuffer::end()
{
    assert(isRecording() && "Command buffer is not recording, please call begin before end");
    VkResult result = vkEndCommandBuffer(m_CommandBuffer);
    if (!result)
    {
        m_State = State::Executable;
        
    }
    return result;
}
void CommandBuffer::beginRenderPass(const RenderTarget& render_target, const std::vector<LoadStoreInfo>& load_store_infos, const std::vector<VkClearValue>& clear_values, const std::vector<std::unique_ptr<Subpass>>& subpasses, VkSubpassContents contents)
{

   m_PipelineState.reset();
   m_ResourceBindingState.reset();
   m_DescriptorSet_Binding_State.clear();

   std::vector<SubpassInfo> subpass_infos(subpasses.size());


   auto subpass_info_it = subpass_infos.begin();
   for (auto& subpass : subpasses)
   {
       subpass_info_it->input_attachments = subpass->getInputAttachments();
       subpass_info_it->output_attachments = subpass->getOutputAttachments();
       //subpass_info_it->color_resolve_attachments = subpass->get_color_resolve_attachments();
       subpass_info_it->m_DisableDepthAttachment = subpass->getDisableDepthAttachment();
       //subpass_info_it->depth_stencil_resolve_mode = subpass->get_depth_stencil_resolve_mode();
       //subpass_info_it->depth_stencil_resolve_attachment = subpass->get_depth_stencil_resolve_attachment();

       ++subpass_info_it;
   }

   
   m_CurrentRenderPass.render_pass = &(m_Pool.getDevice().getResourcesCache().request_render_pass(render_target.getAttachments(), load_store_infos, subpass_infos));
   m_CurrentRenderPass.framebuffer = &(m_Pool.getDevice().getResourcesCache().request_framebuffer(render_target, *m_CurrentRenderPass.render_pass));

   VkRenderPassBeginInfo begin_info{ VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO };
   begin_info.renderPass = m_CurrentRenderPass.render_pass->getHandle();
   begin_info.framebuffer = m_CurrentRenderPass.framebuffer->getHandle();
   begin_info.renderArea.extent = render_target.getExtent();
   begin_info.clearValueCount = clear_values.size();
   begin_info.pClearValues = clear_values.data();

   vkCmdBeginRenderPass(m_CommandBuffer, &begin_info, contents);




   // Update blend state attachments for first subpass
   auto blend_state = m_PipelineState.getColorBlendState();
   blend_state.m_Attachments.resize(m_CurrentRenderPass.render_pass->getColorOutputCount(m_PipelineState.getSubpassIndex()));
   m_PipelineState.setColorBlendState(blend_state);

}

void CommandBuffer::endRenderPass()
{
    vkCmdEndRenderPass(m_CommandBuffer);
}

void CommandBuffer::nextSubpass()
{

    // Increment subpass index
    m_PipelineState.setSubpassIndex(m_PipelineState.getSubpassIndex() + 1);

    // Update blend state attachments
    auto blend_state = m_PipelineState.getColorBlendState();
    blend_state.m_Attachments.resize(m_CurrentRenderPass.render_pass->getColorOutputCount(m_PipelineState.getSubpassIndex()));
    m_PipelineState.setColorBlendState(blend_state);

    // Reset descriptor sets
    m_ResourceBindingState.reset();
    m_DescriptorSet_Binding_State.clear();

    // Clear stored push constants
    //stored_push_constants.clear();

    vkCmdNextSubpass(m_CommandBuffer, VK_SUBPASS_CONTENTS_INLINE);


}


void CommandBuffer::imageBarrier(VulkanImageView& image_view, const ImageMemoryBarrier& memory_barrier)
{
    // Adjust barrier's subresource range for depth images
    auto subresource_range = image_view.getSubResourceRange();
    auto format = image_view.getFormat();
    if (is_depth_only_format(format))
    {
        subresource_range.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;
    }
    else if (is_depth_stencil_format(format))
    {
        subresource_range.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT | VK_IMAGE_ASPECT_STENCIL_BIT;
    }

    VkImageMemoryBarrier image_memory_barrier{ VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER };
    image_memory_barrier.oldLayout = memory_barrier.old_layout;
    image_memory_barrier.newLayout = memory_barrier.new_layout;
    image_memory_barrier.image = image_view.getImage()->getHandle();
    image_memory_barrier.subresourceRange = subresource_range;
    image_memory_barrier.srcAccessMask = memory_barrier.src_access_mask;
    image_memory_barrier.dstAccessMask = memory_barrier.dst_access_mask;

    VkPipelineStageFlags src_stage_mask = memory_barrier.src_stage_mask;
    VkPipelineStageFlags dst_stage_mask = memory_barrier.dst_stage_mask;

    vkCmdPipelineBarrier(
        m_CommandBuffer,
        src_stage_mask,
        dst_stage_mask,
        0,
        0, nullptr,
        0, nullptr,
        1,
        &image_memory_barrier);

}


void CommandBuffer::setViewport(uint32_t first_viewport, const std::vector<VkViewport>& viewports)
{
    vkCmdSetViewport(m_CommandBuffer, first_viewport, viewports.size(), viewports.data());
}

void CommandBuffer::setScissor(uint32_t first_scissor, const std::vector<VkRect2D>& scissors)
{
    vkCmdSetScissor(m_CommandBuffer, first_scissor, scissors.size(), scissors.data());
}

void CommandBuffer::draw(uint32_t vertex_count, uint32_t instance_count, uint32_t first_vertex, uint32_t first_instance)
{
    flushPipelineState();//Flush changes in the pipeline 
    flushDescriptorState();//AKA flush Shader uniforms: Matrices textures etc

    vkCmdDraw(m_CommandBuffer, vertex_count, instance_count, first_vertex, first_instance);
}

void CommandBuffer::draw_indexed(uint32_t index_count, uint32_t instance_count, uint32_t first_index, int32_t vertex_offset, uint32_t first_instance)
{
    flushPipelineState();

    flushDescriptorState();//AKA flush Shader uniforms: Matrices textures etc

    vkCmdDrawIndexed(m_CommandBuffer, index_count, instance_count, first_index, vertex_offset, first_instance);
}


void CommandBuffer::bindPipelineLayout(PipelineLayout& pipeline_layout)
{
    m_PipelineState.setPipelineLayout(pipeline_layout);
}

void CommandBuffer::execute_commands(CommandBuffer& secondary_command_buffer)
{
    vkCmdExecuteCommands(getHandle(), 1, &secondary_command_buffer.getHandle());
}

void CommandBuffer::pushConstants(uint32_t offset, const std::vector<uint8_t>& values)
{
    const PipelineLayout& pipeline_layout = m_PipelineState.getPipelineLayout();

    VkShaderStageFlags shader_stage = pipeline_layout.getPushConstantRangeStage(offset, values.size());

    if (shader_stage)
    {
        vkCmdPushConstants(getHandle(), pipeline_layout.getHandle(), shader_stage, offset, values.size(), values.data());
    }
    else
    {
        LOGERROR("Push constant range [{}, {}] not found", offset, values.size());
    }
}

void CommandBuffer::flushPipelineState()
{
    // Create a new pipeline only if the graphics state changed
    if (!m_PipelineState.isDirty())
    {
        return;
    }
    m_PipelineState.clearDirty();

    m_PipelineState.setRenderPass(*m_CurrentRenderPass.render_pass);
    auto& pipeline = m_Pool.getDevice().getResourcesCache().request_pipeline(m_PipelineState);

    vkCmdBindPipeline(m_CommandBuffer,
        VK_PIPELINE_BIND_POINT_GRAPHICS,
        pipeline.getHandle());

}


void CommandBuffer::bind_buffer(const Buffer& buffer, VkDeviceSize offset, VkDeviceSize range, uint32_t set, uint32_t binding, uint32_t array_element)
{
    m_ResourceBindingState.bind_buffer(buffer, offset, range, set, binding, array_element);
}

void CommandBuffer::copy_buffer_to_image(const Buffer& buffer, const VulkanImage& image, const std::vector<VkBufferImageCopy>& regions)
{
    vkCmdCopyBufferToImage(getHandle(), buffer.getHandle(),
        image.getHandle(), VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
        regions.size(), regions.data());
}

void CommandBuffer::bind_vertex_buffers(uint32_t first_binding, std::vector<std::reference_wrapper<Buffer>> buffers, const std::vector<VkDeviceSize>& offsets)
{
    std::vector<VkBuffer> buffer_handles(buffers.size(), VK_NULL_HANDLE);
    std::transform(buffers.begin(), buffers.end(), buffer_handles.begin(),
        [](const Buffer& buffer) { return buffer.getHandle(); });
    vkCmdBindVertexBuffers(getHandle(), first_binding, buffer_handles.size(), buffer_handles.data(), offsets.data());
    
}

void CommandBuffer::bind_index_buffer(Buffer& buffer, VkDeviceSize offset, VkIndexType index_type)
{
    vkCmdBindIndexBuffer(getHandle(), buffer.getHandle(), offset, index_type);
}
void CommandBuffer::bind_image(const VulkanImageView& image_view, const VulkanSampler& sampler, uint32_t set, uint32_t binding, uint32_t array_element)
{
    m_ResourceBindingState.bind_image(image_view, sampler, set, binding, array_element);
}


template <class T>
using BindingMap = std::map<uint32_t, std::map<uint32_t, T>>;

void CommandBuffer::flushDescriptorState()
{
    const auto& pipeline_layout = m_PipelineState.getPipelineLayout();

    std::unordered_set<uint32_t> update_descriptor_sets;

    // Iterate over the shader sets to check if they have already been bound
  // If they have, add the set so that the command buffer later updates it
    for (auto& set_it : pipeline_layout.getShaderSets())
    {
        uint32_t descriptor_set_id = set_it.first;

        auto descriptor_set_layout_it = m_DescriptorSet_Binding_State.find(descriptor_set_id);

        if (descriptor_set_layout_it != m_DescriptorSet_Binding_State.end())
        {
            if (descriptor_set_layout_it->second->getHandle() != pipeline_layout.getDescriptorSetLayout(descriptor_set_id).getHandle())
            {
                update_descriptor_sets.emplace(descriptor_set_id);
            }
        }
    }

    // Validate that the bound descriptor set layouts exist in the pipeline layout
    for (auto set_it = m_DescriptorSet_Binding_State.begin(); set_it != m_DescriptorSet_Binding_State.end();)
    {
        if (!pipeline_layout.hasDescriptorSetLayout(set_it->first))
        {
            set_it = m_DescriptorSet_Binding_State.erase(set_it);
        }
        else
        {
            ++set_it;
        }
    }


    // Check if a descriptor set needs to be created
    if (m_ResourceBindingState.is_dirty() || !update_descriptor_sets.empty())
    {
        m_ResourceBindingState.clear_dirty();

        // Iterate over all of the resource sets bound by the command buffer
        for (auto& resource_set_it : m_ResourceBindingState.get_resource_sets())
        {
            uint32_t descriptor_set_id = resource_set_it.first;
            auto& resource_set = resource_set_it.second;

            // Don't update resource set if it's not in the update list OR its state hasn't changed
            if (!resource_set.is_dirty() && (update_descriptor_sets.find(descriptor_set_id) == update_descriptor_sets.end()))
            {
                continue;
            }

            // Clear dirty flag for resource set
            m_ResourceBindingState.clear_dirty(descriptor_set_id);

            // Skip resource set if a descriptor set layout doesn't exist for it
            if (!pipeline_layout.hasDescriptorSetLayout(descriptor_set_id))
            {
                continue;
            }

            auto& descriptor_set_layout = pipeline_layout.getDescriptorSetLayout(descriptor_set_id);

            // Make descriptor set layout bound for current set
            m_DescriptorSetLayout_BindingState[descriptor_set_id] = &descriptor_set_layout;

            BindingMap<VkDescriptorBufferInfo> buffer_infos;
            BindingMap<VkDescriptorImageInfo>  image_infos;

            std::vector<uint32_t> dynamic_offsets;

            // The bindings we want to update before binding, if empty we update all bindings
            std::vector<uint32_t> bindings_to_update;

            // Iterate over all resource bindings
            for (auto& binding_it : resource_set.get_resource_bindings())
            {
                auto  binding_index = binding_it.first;
                auto& binding_resources = binding_it.second;

                // Check if binding exists in the pipeline layout
                if (auto binding_info = descriptor_set_layout.getLayoutBinding(binding_index))
                {
                    // If update after bind is enabled, we store the binding index of each binding that need to be updated before being bound
                    /*if (update_after_bind && !(descriptor_set_layout.get_layout_binding_flag(binding_index) & VK_DESCRIPTOR_BINDING_UPDATE_AFTER_BIND_BIT_EXT))
                    {
                        bindings_to_update.push_back(binding_index);
                    }*/

                    // Iterate over all binding resources
                    for (auto& element_it : binding_resources)
                    {
                        auto  array_element = element_it.first;
                        auto& resource_info = element_it.second;

                        // Pointer references
                        auto& buffer = resource_info.m_Buffer;
                        auto& sampler = resource_info.m_Sampler;
                        auto& image_view = resource_info.m_ImageView;

                        // Get buffer info
                        if (buffer != nullptr && is_buffer_descriptor_type(binding_info->descriptorType))
                        {
                            VkDescriptorBufferInfo buffer_info{};

                            buffer_info.buffer = resource_info.m_Buffer->getHandle();
                            buffer_info.offset = resource_info.m_Offset;
                            buffer_info.range = resource_info.m_Range;

                            if (is_dynamic_buffer_descriptor_type(binding_info->descriptorType))
                            {
                                dynamic_offsets.push_back(buffer_info.offset);

                                buffer_info.offset = 0;
                            }

                            buffer_infos[binding_index][array_element] = std::move(buffer_info);
                        }

                        // Get image info
                        else if (image_view != nullptr || sampler != VK_NULL_HANDLE)
                        {
                            // Can be null for input attachments
                            VkDescriptorImageInfo image_info{};
                            image_info.sampler = sampler ? sampler->getHandle() : VK_NULL_HANDLE;
                            image_info.imageView = image_view->getHandle();

                            if (image_view != nullptr)
                            {
                                // Add image layout info based on descriptor type
                                switch (binding_info->descriptorType)
                                {
                                case VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER:
                                    image_info.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
                                    break;
                                case VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT:
                                    if (is_depth_stencil_format(image_view->getFormat()))
                                    {
                                        image_info.imageLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL;
                                    }
                                    else
                                    {
                                        image_info.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
                                    }
                                    break;
                                case VK_DESCRIPTOR_TYPE_STORAGE_IMAGE:
                                    image_info.imageLayout = VK_IMAGE_LAYOUT_GENERAL;
                                    break;

                                default:
                                    continue;
                                }
                            }

                            image_infos[binding_index][array_element] = std::move(image_info);
                        }
                        
                    }
                }
            }

            // Request a descriptor set from the render frame, and write the buffer infos and image infos of all the specified bindings
          
           auto& descriptor_set = m_Pool.getRenderFrame()->requestDescriptorSet(descriptor_set_layout, buffer_infos, image_infos, m_Pool.getThreadIndex());
           descriptor_set.update(bindings_to_update);

           VkDescriptorSet descriptor_set_handle = descriptor_set.getHandle();

            // Bind descriptor set
            vkCmdBindDescriptorSets(m_CommandBuffer,
                VK_PIPELINE_BIND_POINT_GRAPHICS,
                pipeline_layout.getHandle(),
                descriptor_set_id,
                1, &descriptor_set_handle,
               dynamic_offsets.size(),
                dynamic_offsets.data());
        }
    }
    
}

