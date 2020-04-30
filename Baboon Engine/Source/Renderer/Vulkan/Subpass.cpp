#include "Subpass.h"
#include "CommandBuffer.h"
#include "Common.h"
#include "Core/ServiceLocator.h"
#include "VulkanContext.h"
#include "Device.h"
#include "VulkanBuffer.h"
#include <glm/gtc/matrix_transform.hpp>
#include <stb/stb_image.h>
#include "VulkanTexture.h"
#include "RendererVulkan.h"

#include "Cameras/Camera.h"


Subpass::Subpass(VulkanContext& render_context, std::weak_ptr<ShaderSource> vertex_shader, std::weak_ptr<ShaderSource> fragment_shader):
    m_RenderContext(render_context),
    m_VertexShader(vertex_shader),
    m_FragmentShader(fragment_shader)

{
   
}

Subpass::~Subpass()
{
}

void Subpass::invalidatePersistentCommands()
{
    m_PersistentCommandsPerFrame.resetPersistentCommands();
}

void Subpass::setReRecordCommands()
{
    m_PersistentCommandsPerFrame.setDirty();
}



TestTriangleSubPass::TestTriangleSubPass(VulkanContext& render_context, std::weak_ptr<ShaderSource> vertex_shader, std::weak_ptr<ShaderSource> fragment_shader, const Camera* p_Camera):
    Subpass{ render_context,vertex_shader,fragment_shader },
    m_Camera(p_Camera)
{
    auto renderer = ServiceLocator::GetRenderer();
    auto& device = render_context.getDevice();
/*
     std::vector<Vertex> vertices = {
    {{-0.5f, -0.5f, -4.0f}, {1.0f, 0.0f, 0.0f},{0.0f, 0.0f}},
    {{0.5f, -0.5f, -4.0f}, {0.0f, 1.0f, 0.0f},{1.0f, 0.0f}},
    {{0.5f, 0.5f, -4.0f}, {0.0f, 0.0f, 1.0f},{1.0f, 1.0f}},
    {{-0.5f, 0.5f, -4.0f}, {1.0f, 1.0f, 1.0f},{0.0f, 1.0f}}
    };
   
    VkDeviceSize size = vertices.size() * sizeof(Vertex);
    m_TrianglePos = (VulkanBuffer*)renderer->CreateVertexBuffer(vertices.data(), size);

    
    std::vector<uint16_t> indices = {
    0, 1, 2, 2, 3, 0
    };
    size = indices.size() * sizeof(uint16_t);
    m_TriangleIndices = (VulkanBuffer*)renderer->CreateIndexBuffer(indices.data(), size);

  
    


*/
    
    
   
     //unsigned char pPixels[] = { 255,255,255,255 };
    //m_TestTexture = (VulkanTexture*)renderer->CreateTexture(pPixels, 1, 1);
    

   

}
void TestTriangleSubPass::prepare()//setup shaders, To be called when adding subpass to the pipeline
{
    //Warming up shaders so vkCreateShaderModule is called
    auto& device = m_RenderContext.getDevice();
    //auto& vert_module = device.getResourcesCache().request_shader_module(VK_SHADER_STAGE_VERTEX_BIT, m_VertexShader);
    //auto& frag_module = device.getResourcesCache().request_shader_module(VK_SHADER_STAGE_FRAGMENT_BIT,m_FragmentShader);
}




void TestTriangleSubPass::draw(CommandBuffer& primary_commandBuffer) 
{//render geometry within subpass

    auto scene = ServiceLocator::GetSceneManager()->GetCurrentScene();
    if (!scene->IsInit())
        return;
        
    auto& device = m_RenderContext.getDevice();
    auto& activeFrame = m_RenderContext.getActiveFrame();
   

    auto persistentCommands = m_PersistentCommandsPerFrame.getPersistentCommands(activeFrame.getHashId().c_str(), device,activeFrame);
    CommandBuffer* command_buffer = persistentCommands->getCommandBuffer();

    if (persistentCommands->getDirty())
    {
       
        recordCommandBuffers(command_buffer, &primary_commandBuffer);
        persistentCommands->clearDirty();

    }
    primary_commandBuffer.execute_commands(*command_buffer);


}

void TestTriangleSubPass::recordCommandBuffers(CommandBuffer* command_buffer, CommandBuffer* primary_commandBuffer)
{

    auto camera = ServiceLocator::GetCameraManager()->GetCamera(CameraManager::eCameraType_Main);
    auto scene = ServiceLocator::GetSceneManager()->GetCurrentScene();

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

    //Bind the matrices uniform buffer
    command_buffer->bind_buffer(*((VulkanBuffer*)camera->GetCameraUniformBuffer()), 0, sizeof(UBOCamera), 0, 1, 0);

    //Bind vertex buffer
    std::vector<std::reference_wrapper<VulkanBuffer>> buffers;
    buffers.emplace_back(std::ref(*((VulkanBuffer*)scene->GetVerticesBuffer())));

    command_buffer->bind_vertex_buffers(0, std::move(buffers), { 0 });
    //Bind Indices buffer
    command_buffer->bind_index_buffer(*((VulkanBuffer*)scene->GetIndicesBuffer()), 0, VK_INDEX_TYPE_UINT32);

    
    std::multimap<float, Model*> sceneOpaqueModels;//using multimap to get the sorting automatically from when inserting on it
    std::multimap<float, Model*> transparent;
    scene->GetSortedOpaqueAndTransparent(sceneOpaqueModels, transparent);
    
    
    for (auto node_it = sceneOpaqueModels.begin(); node_it != sceneOpaqueModels.end(); node_it++)
    {
        Model& model = *node_it->second;
        drawModel(model, command_buffer);
       
    }


    // Enable alpha blending
    ColorBlendAttachmentState color_blend_attachment{};
    color_blend_attachment.m_BlendEnable = VK_TRUE;
    color_blend_attachment.m_SrcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA;
    color_blend_attachment.m_DstColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
    color_blend_attachment.m_SrcAlphaBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;

    ColorBlendState color_blend_state{};
    color_blend_state.m_Attachments.resize(1);
    color_blend_state.m_Attachments[0] = color_blend_attachment;
    command_buffer->setColorBlendState(color_blend_state);

    DepthStencilState depth_stencil_state{};
    command_buffer->setDepthStencilState(depth_stencil_state);

   
    for (auto node_it = transparent.begin(); node_it != transparent.end(); node_it++)
    {
        Model& model = *node_it->second;
        drawModel(model, command_buffer);

    }



    command_buffer->end();
}

void TestTriangleSubPass::drawModel(Model& model, CommandBuffer* command_buffer)
{

    auto& device = m_RenderContext.getDevice();
    auto renderVulkan =(RendererVulkan*) ServiceLocator::GetRenderer();

    auto pVertexShader = m_VertexShader.lock();
    if (!pVertexShader)
    {
        m_VertexShader = renderVulkan->getShaderSourcePool().getShaderSource("./Shaders/shader.vert");
        pVertexShader = m_VertexShader.lock();
    }
    auto pFragmentShader = m_FragmentShader.lock();
    if (!pFragmentShader)
    {
        m_FragmentShader = renderVulkan->getShaderSourcePool().getShaderSource("./Shaders/shader.frag");
        pFragmentShader = m_FragmentShader.lock();
    }



    auto& vert_module = device.getResourcesCache().request_shader_module(VK_SHADER_STAGE_VERTEX_BIT, pVertexShader, model.getShaderVariant());
    auto& frag_module = device.getResourcesCache().request_shader_module(VK_SHADER_STAGE_FRAGMENT_BIT, pFragmentShader, model.getShaderVariant());
    std::vector<ShaderModule*> shader_modules{ &vert_module, &frag_module };

    auto& pipeline_layout = device.getResourcesCache().request_pipeline_layout(shader_modules);

    command_buffer->bindPipelineLayout(pipeline_layout);


    RasterizationState rasterState{};
    rasterState.m_CullMode = VK_CULL_MODE_NONE;
    rasterState.m_FrontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;
    command_buffer->setRasterState(rasterState);

    //auto vertex_input_resources = pipeline_layout.getResources(ShaderResourceType::Input, VK_SHADER_STAGE_VERTEX_BIT);

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



    auto& descriptor_set_layout = pipeline_layout.getDescriptorSetLayout(0);


   


    auto textures = model.GetMaterial()->getTextures();

    if (textures->size() == 0)
    {
        command_buffer->forceResourceBindingDirty();//TODO: In case of empty shaders with no bindings at all, when flushing resourcesBinding is dirty won't be dirty hence not updating the descriptor set creating a validation error! Better way than forcing it?
    }
    else
    {
        for (auto texture : *textures)
        {
            if (auto layout_binding = descriptor_set_layout.getLayoutBinding(texture.first))
            {
                VulkanTexture* vulkanTex = (VulkanTexture*)texture.second;
                if (vulkanTex)
                {
                    command_buffer->bind_image(*vulkanTex->getImageView(),
                        *vulkanTex->getSampler(),
                        0, layout_binding->binding, 0);
                }
            }
        }
    }


    int nIndices = model.GetMesh()->GetNIndices();
    int indexStart = model.GetMesh()->GetIndexStartPosition();
    command_buffer->pushConstants(0, model.getModelMatrix());
  
    command_buffer->draw_indexed(nIndices, 1, indexStart, model.GetMesh()->GetVertexStartPosition(), 0);
}

