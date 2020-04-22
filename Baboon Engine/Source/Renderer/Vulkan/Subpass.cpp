#include "Subpass.h"
#include "CommandBuffer.h"
#include "Common.h"
#include "Core/ServiceLocator.h"
#include "VulkanContext.h"
#include "Device.h"
#include "Buffer.h"
#include <glm/gtc/matrix_transform.hpp>
#include <stb/stb_image.h>
#include "VulkanImage.h"
#include "VulkanSampler.h"
#include "VulkanImageView.h"
#include "Cameras/Camera.h"


Subpass::Subpass(VulkanContext& render_context, ShaderSource&& vertex_shader, ShaderSource&& fragment_shader):
    m_RenderContext(render_context),
    m_VertexShader(std::move(vertex_shader)),
    m_FragmentShader(std::move(fragment_shader))

{

}




//TODO: Make this render a triangle! I will need to setup shaders and geometry to do so
TestTriangleSubPass::TestTriangleSubPass(VulkanContext& render_context, ShaderSource&& vertex_shader, ShaderSource&& fragment_shader, const Camera* p_Camera):
    Subpass{ render_context,std::move(vertex_shader),std::move(fragment_shader) },
    m_Camera(p_Camera)
{
    auto& device = render_context.getDevice();

     std::vector<Vertex> vertices = {
    {{-0.5f, -0.5f, -4.0f}, {1.0f, 0.0f, 0.0f},{0.0f, 0.0f}},
    {{0.5f, -0.5f, -4.0f}, {0.0f, 1.0f, 0.0f},{1.0f, 0.0f}},
    {{0.5f, 0.5f, -4.0f}, {0.0f, 0.0f, 1.0f},{1.0f, 1.0f}},
    {{-0.5f, 0.5f, -4.0f}, {1.0f, 1.0f, 1.0f},{0.0f, 1.0f}}
    };
   
    VkDeviceSize size = vertices.size() * sizeof(Vertex);
  
    m_TrianglePos = new Buffer(device, size, VK_BUFFER_USAGE_VERTEX_BUFFER_BIT, VMA_MEMORY_USAGE_GPU_TO_CPU);
    m_TrianglePos->update(vertices.data(), size);
    
    
    std::vector<uint16_t> indices = {
    0, 1, 2, 2, 3, 0
    };
    size = indices.size() * sizeof(uint16_t);
    m_TriangleIndices = new Buffer(device, size, VK_BUFFER_USAGE_INDEX_BUFFER_BIT, VMA_MEMORY_USAGE_GPU_TO_CPU);
    m_TriangleIndices->update(indices.data(), size);

  
    size = sizeof(UBO);
    m_UniformBuffer = new Buffer(device, size, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, VMA_MEMORY_USAGE_GPU_TO_CPU);


    //TODO: Encapsulate this into some sort of create texture in the scene loader
    auto& command_buffer = device.requestCommandBuffer();
    command_buffer.begin(VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT, 0);

    unsigned char* pPixels = nullptr;
    int width, height, texChannels;
    pPixels = stbi_load("Textures/baboon.jpg", &width, &height, &texChannels, STBI_rgb_alpha);


    VkExtent3D extent{ width,height,1 };
    m_TestTexture.m_Image = new VulkanImage(device, extent, VK_FORMAT_R8G8B8A8_UNORM, VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT, VMA_MEMORY_USAGE_GPU_ONLY);
    m_TestTexture.m_View = new VulkanImageView(*m_TestTexture.m_Image, VK_IMAGE_VIEW_TYPE_2D);

    size = width * height * 4 * sizeof(unsigned char);//we are forcing 4 channels with the STBI_rgb_alpha flag
    Buffer stage_buffer{ device,
                              size,
                              VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
                              VMA_MEMORY_USAGE_CPU_ONLY };


    stage_buffer.update(pPixels,size);
    stbi_image_free(pPixels);//Since its already copied in the buffer

    /////Lets upload image to the gpu
    {
        ImageMemoryBarrier memory_barrier{};
        memory_barrier.old_layout = VK_IMAGE_LAYOUT_UNDEFINED;
        memory_barrier.new_layout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
        memory_barrier.src_access_mask = 0;
        memory_barrier.dst_access_mask = VK_ACCESS_TRANSFER_WRITE_BIT;
        memory_barrier.src_stage_mask = VK_PIPELINE_STAGE_HOST_BIT;
        memory_barrier.dst_stage_mask = VK_PIPELINE_STAGE_TRANSFER_BIT;

        command_buffer.imageBarrier(*m_TestTexture.m_View, memory_barrier);
    }

    std::vector<VkBufferImageCopy> buffer_copy_regions(1);//TODO: We are assuming mipmaps = 1
    auto& copy_region = buffer_copy_regions[0];
    copy_region.bufferOffset = 0;//mipmap.offset;
    copy_region.imageSubresource = m_TestTexture.m_View->getSubresourceLayers();
    // Update miplevel
    copy_region.imageSubresource.mipLevel = 0;//mipmap.level;
    VkExtent3D mipExtent{ width,height,1 };
    copy_region.imageExtent = mipExtent;//mipmap.extent;

    command_buffer.copy_buffer_to_image(stage_buffer, *m_TestTexture.m_Image, buffer_copy_regions);

    //Transition image to readable by the shader
    {
        ImageMemoryBarrier memory_barrier{};
        memory_barrier.old_layout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
        memory_barrier.new_layout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
        memory_barrier.src_access_mask = VK_ACCESS_TRANSFER_WRITE_BIT;
        memory_barrier.dst_access_mask = VK_ACCESS_SHADER_READ_BIT;
        memory_barrier.src_stage_mask = VK_PIPELINE_STAGE_TRANSFER_BIT;
        memory_barrier.dst_stage_mask = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;

        command_buffer.imageBarrier(*m_TestTexture.m_View, memory_barrier);
    }


    command_buffer.end();

    auto& queue = m_RenderContext.getDevice().getGraphicsQueue();

    queue.submit(command_buffer, device.requestFence());

    device.getFencePool().wait();
    device.getFencePool().reset();
    device.getCommandPool().reset_pool();
    device.wait_idle();

    //!TODO END/////////////////////////////////////////////////////////////////////


    //I think this same sampler can be used for all the textures TODO: Make it shareable
    VkSamplerCreateInfo samplerInfo = {};
    samplerInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
    samplerInfo.magFilter = VK_FILTER_LINEAR;
    samplerInfo.minFilter = VK_FILTER_LINEAR;

    samplerInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_REPEAT;
    samplerInfo.addressModeV = VK_SAMPLER_ADDRESS_MODE_REPEAT;
    samplerInfo.addressModeW = VK_SAMPLER_ADDRESS_MODE_REPEAT;

    samplerInfo.anisotropyEnable = VK_TRUE;
    samplerInfo.maxAnisotropy = 16;

    samplerInfo.borderColor = VK_BORDER_COLOR_INT_OPAQUE_BLACK;

    samplerInfo.unnormalizedCoordinates = VK_FALSE;
    samplerInfo.compareEnable = VK_FALSE;
    samplerInfo.compareOp = VK_COMPARE_OP_ALWAYS;

    samplerInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
    samplerInfo.mipLodBias = 0.0f;
    samplerInfo.minLod = 0.0f;
    samplerInfo.maxLod = 0.0f;
    samplerInfo.anisotropyEnable = VK_FALSE;
    m_TestTexture.m_Sampler = new VulkanSampler(device, samplerInfo);


    m_PersistentCommandsPerFrame = new PersistentCommandsPerFrame();

}
void TestTriangleSubPass::prepare()//setup shaders, To be called when adding subpass to the pipeline
{
    //Warming up shaders so vkCreateShaderModule is called
    auto& device = m_RenderContext.getDevice();
    auto& vert_module = device.getResourcesCache().request_shader_module(VK_SHADER_STAGE_VERTEX_BIT, m_VertexShader);
    auto& frag_module = device.getResourcesCache().request_shader_module(VK_SHADER_STAGE_FRAGMENT_BIT,m_FragmentShader);
}




void TestTriangleSubPass::draw(CommandBuffer& primary_commandBuffer) {//render geometry within subpass



    if (m_Camera->GetDirty())//If camera moves we need to re-record
    {
        m_PersistentCommandsPerFrame->setDirty();
    }


    auto& device = m_RenderContext.getDevice();
    auto& activeFrame = m_RenderContext.getActiveFrame();
   

    auto persistentCommands = m_PersistentCommandsPerFrame->getPersistentCommands(activeFrame.getHashId().c_str(), device,activeFrame);
    CommandBuffer* command_buffer = persistentCommands->m_PersistentCommandsPerFrame;

    if (persistentCommands->m_NeedsSecondaryCommandsRecording)
    {
       
        recordCommandBuffers(command_buffer, &primary_commandBuffer);
        persistentCommands->m_NeedsSecondaryCommandsRecording = false;

    }
    primary_commandBuffer.execute_commands(*command_buffer);


}

void TestTriangleSubPass::recordCommandBuffers(CommandBuffer* command_buffer, CommandBuffer* primary_commandBuffer)
{

    auto& device = m_RenderContext.getDevice();
    auto& renderTarget = m_RenderContext.getActiveFrame().getRenderTarget();//Grab the render target

    //Set viewport and scissors
    auto& extent = renderTarget.getExtent();
    VkViewport viewport{};
    viewport.width = static_cast<float>(extent.width);
    viewport.height = static_cast<float>(extent.height);
    viewport.minDepth = 0.0f;
    viewport.maxDepth = 1.0f;
    VkRect2D scissor{};
    scissor.extent = extent;



    command_buffer->begin(VK_COMMAND_BUFFER_USAGE_RENDER_PASS_CONTINUE_BIT , primary_commandBuffer);
    command_buffer->setViewport(0, { viewport });
    command_buffer->setScissor(0, { scissor });
    command_buffer->setColorBlendState(primary_commandBuffer->getColorBlendState());
    command_buffer->setDepthStencilState(primary_commandBuffer->getDepthStencilState());



    auto& vert_module = device.getResourcesCache().request_shader_module(VK_SHADER_STAGE_VERTEX_BIT, m_VertexShader);
    auto& frag_module = device.getResourcesCache().request_shader_module(VK_SHADER_STAGE_FRAGMENT_BIT, m_FragmentShader);
    std::vector<ShaderModule*> shader_modules{ &vert_module, &frag_module };

    auto& pipeline_layout = device.getResourcesCache().request_pipeline_layout(shader_modules);

    command_buffer->bindPipelineLayout(pipeline_layout);


    RasterizationState rasterState{};
    rasterState.m_FrontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;
    command_buffer->setRasterState(rasterState);



    auto& descriptor_set_layout = pipeline_layout.getDescriptorSetLayout(0);
    if (auto layout_binding = descriptor_set_layout.getLayoutBinding("baseTexture"))//TODO: When we create textures store them in a map with their shader name and replace it here
    {
        command_buffer->bind_image(*m_TestTexture.m_View,
            *m_TestTexture.m_Sampler,
            0, layout_binding->binding, 0);
    }




    auto vertex_input_resources = pipeline_layout.getResources(ShaderResourceType::Input, VK_SHADER_STAGE_VERTEX_BIT);

    VertexInputState vertex_input_state;
    VkVertexInputBindingDescription bindingDescription = {};
    std::vector<VkVertexInputAttributeDescription> attributeDescriptions = {};

    //This is specific for this vertex format, has to be improved, one pipeline for each vertexFormat?
    Vertex::GetVertexDescription(&bindingDescription);
    Vertex::GetAttributesDescription(attributeDescriptions);

    vertex_input_state.m_Bindings.push_back(bindingDescription);
    for (auto& attributeDescription : attributeDescriptions)
    {
        vertex_input_state.m_Attributes.push_back(attributeDescription);
    }
    command_buffer->setVertexInputState(vertex_input_state);


    ///TODO:: Move this to an animate func and link it with the camera
    //m_TestUBO.model = glm::translate(m_TestUBO.model, glm::vec3(0.0001f));
    m_TestUBO.proj = m_Camera->GetProjMatrix();
    m_TestUBO.view = m_Camera->GetViewMatrix();
    m_UniformBuffer->update(&m_TestUBO, sizeof(UBO));//TODO: This to be updated every frame for the camera, Consider using pushconstant for the camera matrices???

    //Bind the matrices uniform buffer
    command_buffer->bind_buffer(*m_UniformBuffer, 0, sizeof(UBO), 0, 1, 0);

    //Bind vertex buffer
    std::vector<std::reference_wrapper<Buffer>> buffers;
    buffers.emplace_back(std::ref(*m_TrianglePos));
    command_buffer->bind_vertex_buffers(0, std::move(buffers), { 0 });

    //Bind Indices buffer
    command_buffer->bind_index_buffer(*m_TriangleIndices, 0, VK_INDEX_TYPE_UINT16);




    //command_buffer->draw(6, 1, 0, 0);
    command_buffer->draw_indexed(6, 1, 0, 0, 0);



    command_buffer->end();
}

