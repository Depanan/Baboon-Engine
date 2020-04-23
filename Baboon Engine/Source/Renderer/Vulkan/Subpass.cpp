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
    unsigned char* pPixels = nullptr;
    int width, height, texChannels;
    pPixels = stbi_load("Textures/baboon.jpg", &width, &height, &texChannels, STBI_rgb_alpha);
    m_TestTexture = (VulkanTexture*)renderer->CreateTexture(pPixels, width, height);
    stbi_image_free(pPixels);//Since its already copied in the buffer
    

    m_PersistentCommandsPerFrame = new PersistentCommandsPerFrame();

}
void TestTriangleSubPass::prepare()//setup shaders, To be called when adding subpass to the pipeline
{
    //Warming up shaders so vkCreateShaderModule is called
    auto& device = m_RenderContext.getDevice();
    auto& vert_module = device.getResourcesCache().request_shader_module(VK_SHADER_STAGE_VERTEX_BIT, m_VertexShader);
    auto& frag_module = device.getResourcesCache().request_shader_module(VK_SHADER_STAGE_FRAGMENT_BIT,m_FragmentShader);
}




void TestTriangleSubPass::draw(CommandBuffer& primary_commandBuffer) 
{//render geometry within subpass

    auto scene = ServiceLocator::GetSceneManager()->GetScene();
    if (!scene->IsInit())
        return;

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

    auto camera = ServiceLocator::GetCameraManager()->GetCamera(CameraManager::eCameraType_Main);
    auto scene = ServiceLocator::GetSceneManager()->GetScene();


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
    rasterState.m_CullMode = VK_CULL_MODE_NONE;
    rasterState.m_FrontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;
    command_buffer->setRasterState(rasterState);



    auto& descriptor_set_layout = pipeline_layout.getDescriptorSetLayout(0);
    if (auto layout_binding = descriptor_set_layout.getLayoutBinding("baseTexture"))//TODO: When we create textures store them in a map with their shader name and replace it here
    {
        command_buffer->bind_image(*m_TestTexture->getImageView(),
            *m_TestTexture->getSampler(),
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


   

    //Bind the matrices uniform buffer
    command_buffer->bind_buffer(*((VulkanBuffer*)camera->GetCameraUniformBuffer()), 0, sizeof(UBOCamera), 0, 1, 0);

    //Bind vertex buffer
    std::vector<std::reference_wrapper<VulkanBuffer>> buffers;
    buffers.emplace_back(std::ref(*((VulkanBuffer*)scene->GetVerticesBuffer())));
    
    command_buffer->bind_vertex_buffers(0, std::move(buffers), { 0 });//TODO: Here get the big buffer chunk from the scene
    //Bind Indices buffer
    command_buffer->bind_index_buffer(*((VulkanBuffer*)scene->GetIndicesBuffer()), 0, VK_INDEX_TYPE_UINT16);//TODO: Here get the big buffer chunk from the scene




    std::vector <Model> sceneModels = *(scene->GetModels());
    for (int i = 0; i< sceneModels.size();i++)
    {
        Model& model = sceneModels[i];
        int nIndices = sceneModels[i].GetMesh()->GetNIndices();
        int indexStart = sceneModels[i].GetMesh()->GetIndexStartPosition();
        command_buffer->pushConstants(0, model.getModelMatrix());
        command_buffer->draw_indexed(nIndices, 1, indexStart, sceneModels[i].GetMesh()->GetVertexStartPosition(), 0);
    }

    



    command_buffer->end();
}

