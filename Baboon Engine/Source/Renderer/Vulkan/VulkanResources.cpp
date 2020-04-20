#include "VulkanResources.h"
#include "PipelineState.h"
#include <glm/gtx/hash.hpp>
#include "resources/DescriptorPool.h"


template <class T>
void hash_combine(size_t& seed, const T& v)
{
    std::hash<T> hasher;
    glm::detail::hash_combine(seed, hasher(v));
}



//Template
template <typename T, typename... Args>
void hash_param(size_t& seed, const T& first_arg, const Args&... args)
{
    hash_param(seed, first_arg);

    hash_param(seed, args...);
}


template <class T, class... A>
T& request_resource(Device& device, std::unordered_map<std::size_t, T>& resources, A&... args)
{

    std::size_t hash{ 0U };
    hash_param(hash, args...);

    auto res_it = resources.find(hash);

    if (res_it != resources.end())
    {
        return res_it->second;
    }

    // If we do not have it already, create and cache it
    const char* res_type = typeid(T).name();
    size_t      res_id = resources.size();



    T resource(device, args...);

    auto res_ins_it = resources.emplace(hash, std::move(resource));

    if (!res_ins_it.second)
    {
        LOGERROR(" Cannot insert new THING");
    }

    res_it = res_ins_it.first;

    return res_it->second;
}


template <class T, class... A>
T& request_resource(Device& device, std::mutex& resource_mutex, std::unordered_map<std::size_t, T>& resources, A&... args)
{
    std::lock_guard<std::mutex> guard(resource_mutex);

    auto& res = request_resource(device, resources, args...);

    return res;
}


//Specializations




template <>
void hash_param<PipelineState>(
    size_t& seed,
    const PipelineState& value)
{
    hash_combine(seed, value);
}


template <>
void hash_param<RenderTarget>(
    size_t& seed,
    const RenderTarget& value)
{
    hash_combine(seed, value);
}

template <>
void hash_param<RenderPass>(
    size_t& seed,
    const RenderPass& value)
{
    hash_combine(seed, value);
}
template <>
void hash_param<ShaderSource>(
    size_t& seed,
    const ShaderSource& value)
{
    hash_combine(seed, value);
}
template <>
void hash_param<VkShaderStageFlagBits>(
    size_t& seed,
    const VkShaderStageFlagBits& value)
{
    hash_combine(seed, value);
}


template <>
void hash_param<std::vector<Attachment>>(
    size_t& seed,
    const std::vector<Attachment>& value)
{
    for (auto& attachment : value)
    {
        hash_combine(seed, attachment);
    }
}

template <>
 void hash_param<std::vector<LoadStoreInfo>>(
    size_t& seed,
    const std::vector<LoadStoreInfo>& value)
{
    for (auto& load_store_info : value)
    {
        hash_combine(seed, load_store_info);
    }
}


template <>
 void hash_param<std::vector<SubpassInfo>>(
    size_t& seed,
    const std::vector<SubpassInfo>& value)
{
    for (auto& subpass_info : value)
    {
        hash_combine(seed, subpass_info);
    }
}

 template <>
 void hash_param<std::vector<ShaderModule*>>(
     size_t& seed,
     const std::vector<ShaderModule*>& value)
 {
     for (auto& shaderModule : value)
     {
         hash_combine(seed, shaderModule);
     }
 }
 template <>
 inline void hash_param<std::vector<ShaderResource>>(
     size_t& seed,
     const std::vector<ShaderResource>& value)
 {
     for (auto& resource : value)
     {
         hash_combine(seed, resource);
     }
 }




//!Specializations


//Here we are inserting our hash specializations in the std namespace
namespace std
{

   


    template <>
    struct hash<RenderPass>
    {
        std::size_t operator()(const RenderPass& render_pass) const
        {
            std::size_t result = 0;

            hash_combine(result, render_pass.getHandle());

            return result;
        }
    };

    template <>
    struct hash<Attachment>
    {
        std::size_t operator()(const Attachment& attachment) const
        {
            std::size_t result = 0;

            hash_combine(result, static_cast<std::underlying_type<VkFormat>::type>(attachment.m_Format));
            hash_combine(result, static_cast<std::underlying_type<VkSampleCountFlagBits>::type>(attachment.m_Samples));

            return result;
        }
    };

    template <>
    struct hash<LoadStoreInfo>
    {
        std::size_t operator()(const LoadStoreInfo& load_store_info) const
        {
            std::size_t result = 0;

            hash_combine(result, static_cast<std::underlying_type<VkAttachmentLoadOp>::type>(load_store_info.load_op));
            hash_combine(result, static_cast<std::underlying_type<VkAttachmentStoreOp>::type>(load_store_info.store_op));

            return result;
        }
    };

    template <>
    struct hash<SubpassInfo>
    {
        std::size_t operator()(const SubpassInfo& subpass_info) const
        {
            std::size_t result = 0;

            for (uint32_t output_attachment : subpass_info.output_attachments)
            {
                hash_combine(result, output_attachment);
            }

            for (uint32_t input_attachment : subpass_info.input_attachments)
            {
                hash_combine(result, input_attachment);
            }

            return result;
        }
    };

    template <>
    struct hash<RenderTarget>
    {
        std::size_t operator()(const RenderTarget& render_target) const
        {
            std::size_t result = 0;

            for (auto& view : render_target.getViews())
            {
                hash_combine(result, view.getHandle());
            }

            return result;
        }
    };

    template <>
    struct hash<ShaderSource>
    {
        std::size_t operator()(const ShaderSource& shader_source) const
        {
            std::size_t result = 0;

            hash_combine(result, shader_source.get_id());

            return result;
        }
    };

    template <>
    struct hash<ShaderModule>
    {
        std::size_t operator()(const ShaderModule& shader_module) const
        {
            std::size_t result = 0;

            hash_combine(result, shader_module.getId());

            return result;
        }
    };

    template <>
    struct hash<VkVertexInputAttributeDescription>
    {
        std::size_t operator()(const VkVertexInputAttributeDescription& vertex_attrib) const
        {
            std::size_t result = 0;

            hash_combine(result, vertex_attrib.binding);
            hash_combine(result, static_cast<std::underlying_type<VkFormat>::type>(vertex_attrib.format));
            hash_combine(result, vertex_attrib.location);
            hash_combine(result, vertex_attrib.offset);

            return result;
        }
    };

    template <>
    struct hash<VkVertexInputBindingDescription>
    {
        std::size_t operator()(const VkVertexInputBindingDescription& vertex_binding) const
        {
            std::size_t result = 0;

            hash_combine(result, vertex_binding.binding);
            hash_combine(result, static_cast<std::underlying_type<VkVertexInputRate>::type>(vertex_binding.inputRate));
            hash_combine(result, vertex_binding.stride);

            return result;
        }
    };

    template <>
    struct hash<StencilOpState>
    {
        std::size_t operator()(const StencilOpState& stencil) const
        {
            std::size_t result = 0;

            hash_combine(result, static_cast<std::underlying_type<VkCompareOp>::type>(stencil.m_CompareOp));
            hash_combine(result, static_cast<std::underlying_type<VkStencilOp>::type>(stencil.m_DepthFailOp));
            hash_combine(result, static_cast<std::underlying_type<VkStencilOp>::type>(stencil.m_FailOp));
            hash_combine(result, static_cast<std::underlying_type<VkStencilOp>::type>(stencil.m_PassOp));

            return result;
        }
    };
    template <>
    struct hash<ColorBlendAttachmentState>
    {
        std::size_t operator()(const ColorBlendAttachmentState& color_blend_attachment) const
        {
            std::size_t result = 0;

            hash_combine(result, static_cast<std::underlying_type<VkBlendOp>::type>(color_blend_attachment.m_AlphaBlendOp));
            hash_combine(result, color_blend_attachment.m_BlendEnable);
            hash_combine(result, static_cast<std::underlying_type<VkBlendOp>::type>(color_blend_attachment.m_ColorBlendOp));
            hash_combine(result, color_blend_attachment.m_ColorWriteMask);
            hash_combine(result, static_cast<std::underlying_type<VkBlendFactor>::type>(color_blend_attachment.m_DstAlphaBlendFactor));
            hash_combine(result, static_cast<std::underlying_type<VkBlendFactor>::type>(color_blend_attachment.m_DstColorBlendFactor));
            hash_combine(result, static_cast<std::underlying_type<VkBlendFactor>::type>(color_blend_attachment.m_SrcAlphaBlendFactor));
            hash_combine(result, static_cast<std::underlying_type<VkBlendFactor>::type>(color_blend_attachment.m_SrcColorBlendFactor));

            return result;
        }
    };


    template <>
    struct hash<PipelineState>
    {
        std::size_t operator()(const PipelineState& pipeline_state) const
        {
            std::size_t result = 0;

            hash_combine(result, pipeline_state.getPipelineLayout().getHandle());

            // For graphics only
            if (auto render_pass = pipeline_state.getRenderPass())
            {
                hash_combine(result, render_pass->getHandle());
            }

            //hash_combine(result, pipeline_state.get_specialization_constant_state());

            hash_combine(result, pipeline_state.getSubpassIndex());

            for (auto shader_module : pipeline_state.getPipelineLayout().getShaderModules())
            {
                hash_combine(result, shader_module->getId());
            }

            // VkPipelineVertexInputStateCreateInfo
            for (auto& attribute : pipeline_state.getVertexInputState().m_Attributes)
            {
                hash_combine(result, attribute);
            }

            for (auto& binding : pipeline_state.getVertexInputState().m_Bindings)
            {
                hash_combine(result, binding);
            }

            // VkPipelineInputAssemblyStateCreateInfo
            hash_combine(result, pipeline_state.getInputAssemblyState().m_PrimitiveRestartEnabled);
            hash_combine(result, static_cast<std::underlying_type<VkPrimitiveTopology>::type>(pipeline_state.getInputAssemblyState().m_Topology));

            //VkPipelineViewportStateCreateInfo
            hash_combine(result, pipeline_state.getViewportState().m_ViewportCount);
            hash_combine(result, pipeline_state.getViewportState().m_ScissorCount);

            // VkPipelineRasterizationStateCreateInfo
            hash_combine(result, pipeline_state.getRasterizationState().m_CullMode);
            hash_combine(result, pipeline_state.getRasterizationState().m_DepthBiasEnabled);
            hash_combine(result, pipeline_state.getRasterizationState().m_DepthClampEnabled);
            hash_combine(result, static_cast<std::underlying_type<VkFrontFace>::type>(pipeline_state.getRasterizationState().m_FrontFace));
            hash_combine(result, static_cast<std::underlying_type<VkPolygonMode>::type>(pipeline_state.getRasterizationState().m_PolygonMode));
            hash_combine(result, pipeline_state.getRasterizationState().m_RasterizerDiscardEnabled);

            // VkPipelineMultisampleStateCreateInfo
            hash_combine(result, pipeline_state.getMultisampleState().m_AlphaToCoverageEnabled);
            hash_combine(result, pipeline_state.getMultisampleState().m_AlphaToOneEnabled);
            hash_combine(result, pipeline_state.getMultisampleState().m_minSampleShading);
            hash_combine(result, static_cast<std::underlying_type<VkSampleCountFlagBits>::type>(pipeline_state.getMultisampleState().m_RasterizationSamples));
            hash_combine(result, pipeline_state.getMultisampleState().m_SampleShadingEnabled);
            hash_combine(result, pipeline_state.getMultisampleState().m_SampleMask);

            // VkPipelineDepthStencilStateCreateInfo
            hash_combine(result, pipeline_state.getDepthStencilState().m_Back);
            hash_combine(result, pipeline_state.getDepthStencilState().m_DepthBoundTestEnable);
            hash_combine(result, static_cast<std::underlying_type<VkCompareOp>::type>(pipeline_state.getDepthStencilState().m_DepthCompareOp));
            hash_combine(result, pipeline_state.getDepthStencilState().m_DepthTestEnable);
            hash_combine(result, pipeline_state.getDepthStencilState().m_DepthWriteEnable);
            hash_combine(result, pipeline_state.getDepthStencilState().m_Front);
            hash_combine(result, pipeline_state.getDepthStencilState().m_StencilTestEnable);

            // VkPipelineColorBlendStateCreateInfo
            hash_combine(result, static_cast<std::underlying_type<VkLogicOp>::type>(pipeline_state.getColorBlendState().m_LogicOp));
            hash_combine(result, pipeline_state.getColorBlendState().m_LogicOpEnabled);

            for (auto& attachment : pipeline_state.getColorBlendState().m_Attachments)
            {
                hash_combine(result, attachment);
            }
            
            return result;
        }
    };

    template <>
    struct hash<ShaderResource>
    {
        std::size_t operator()(const ShaderResource& shader_resource) const
        {
            std::size_t result = 0;

            if (shader_resource.type == ShaderResourceType::Input ||
                shader_resource.type == ShaderResourceType::Output ||
                shader_resource.type == ShaderResourceType::PushConstant ||
                shader_resource.type == ShaderResourceType::SpecializationConstant)
            {
                return result;
            }

            hash_combine(result, shader_resource.set);
            hash_combine(result, shader_resource.binding);
            hash_combine(result, static_cast<std::underlying_type<ShaderResourceType>::type>(shader_resource.type));
            hash_combine(result, shader_resource.mode);

            return result;
        }
    };

}


template <>
void hash_param<DescriptorSetLayout>(
    size_t& seed,
    const DescriptorSetLayout& value)
{
    hash_combine(seed, value);
}

template <>
void hash_param<DescriptorPool>(
    size_t& seed,
    const DescriptorPool& value)
{
    hash_combine(seed, value);
}

template <>
inline void hash_param<std::map<uint32_t, std::map<uint32_t, VkDescriptorBufferInfo>>>(
    size_t& seed,
    const std::map<uint32_t, std::map<uint32_t, VkDescriptorBufferInfo>>& value)
{
    for (auto& binding_set : value)
    {
        hash_combine(seed, binding_set.first);

        for (auto& binding_element : binding_set.second)
        {
            hash_combine(seed, binding_element.first);
            hash_combine(seed, binding_element.second);
        }
    }
}

template <>
inline void hash_param<std::map<uint32_t, std::map<uint32_t, VkDescriptorImageInfo>>>(
    size_t& seed,
    const std::map<uint32_t, std::map<uint32_t, VkDescriptorImageInfo>>& value)
{
    for (auto& binding_set : value)
    {
        hash_combine(seed, binding_set.first);

        for (auto& binding_element : binding_set.second)
        {
            hash_combine(seed, binding_element.first);
            hash_combine(seed, binding_element.second);
        }
    }
}


namespace std
{

    template <>
    struct hash<DescriptorSetLayout>
    {
        std::size_t operator()(const DescriptorSetLayout& descriptor_set_layout) const
        {
            std::size_t result = 0;

            hash_combine(result, descriptor_set_layout.getHandle());

            return result;
        }
    };
    template <>
    struct hash<DescriptorPool>
    {
        std::size_t operator()(const DescriptorPool& descriptor_pool) const
        {
            std::size_t result = 0;

            hash_combine(result, descriptor_pool.get_descriptor_set_layout());

            return result;
        }
    };
    template <>
    struct hash<VkDescriptorBufferInfo>
    {
        std::size_t operator()(const VkDescriptorBufferInfo& descriptor_buffer_info) const
        {
            std::size_t result = 0;

            hash_combine(result, descriptor_buffer_info.buffer);
            hash_combine(result, descriptor_buffer_info.range);
            hash_combine(result, descriptor_buffer_info.offset);

            return result;
        }
    };

    template <>
    struct hash<VkDescriptorImageInfo>
    {
        std::size_t operator()(const VkDescriptorImageInfo& descriptor_image_info) const
        {
            std::size_t result = 0;

            hash_combine(result, descriptor_image_info.imageView);
            hash_combine(result, static_cast<std::underlying_type<VkImageLayout>::type>(descriptor_image_info.imageLayout));
            hash_combine(result, descriptor_image_info.sampler);

            return result;
        }
    };
}




VulkanResources::VulkanResources( Device& device):
m_Device(device)
{

}






RenderPass& VulkanResources::request_render_pass(const std::vector<Attachment>& attachments, const std::vector<LoadStoreInfo>& load_store_infos, const std::vector<SubpassInfo>& subpasses)
{
     return request_resource(m_Device,m_RenderPassMutex, m_RenderPasses_Cache, attachments, load_store_infos, subpasses);
}

FrameBuffer& VulkanResources::request_framebuffer(const RenderTarget& render_target, const RenderPass& render_pass)
{
    return request_resource(m_Device, m_FramebufferMutex, m_FrameBuffers_Cache, render_target, render_pass);
}

ShaderModule& VulkanResources::request_shader_module(VkShaderStageFlagBits stage, const ShaderSource& glsl_source)
{
    return request_resource(m_Device, m_ShaderModuleMutex, m_Shaders_Cache, stage, glsl_source);
}

PipelineLayout& VulkanResources::request_pipeline_layout(std::vector<ShaderModule*> shader_modules)
{
    return request_resource(m_Device, m_PipelineLayoutMutex, m_PipelinesLayout_Cache, shader_modules);
}

Pipeline& VulkanResources::request_pipeline(const PipelineState& pipelineState)
{
    return request_resource(m_Device, m_PipelineMutex, m_Pipelines_Cache, pipelineState);
}

DescriptorSetLayout& VulkanResources::request_descriptor_set_layout(const std::vector<ShaderResource>& set_resources)
{
    return request_resource(m_Device, m_DescriptorSetLayoutMutex, m_DescriptorSetLayout_Cache, set_resources);
}

DescriptorPool& VulkanResources::request_descriptor_pool(std::unordered_map<std::size_t, DescriptorPool>& descriptorPoolsCache, DescriptorSetLayout& descriptor_set_layout)
{
    return request_resource(m_Device, descriptorPoolsCache, descriptor_set_layout);
}

DescriptorSet& VulkanResources::request_descriptor_set(std::unordered_map<std::size_t, DescriptorSet>& descriptorSetCache, DescriptorSetLayout& descriptor_set_layout, DescriptorPool& pool,  const BindingMap<VkDescriptorBufferInfo>& buffer_infos, const BindingMap<VkDescriptorImageInfo>& image_infos)
{
    return request_resource(m_Device, descriptorSetCache, descriptor_set_layout, pool, buffer_infos, image_infos);
}

void VulkanResources::clear()
{
    m_RenderPasses_Cache.clear();
    m_FrameBuffers_Cache.clear();
    m_Shaders_Cache.clear();
    m_PipelinesLayout_Cache.clear();
}


