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



ForwardSubpass::ForwardSubpass(VulkanContext& render_context, std::weak_ptr<ShaderSource> vertex_shader, std::weak_ptr<ShaderSource> fragment_shader, const Camera* p_Camera):
    Subpass{ render_context,vertex_shader,fragment_shader },
    m_Camera(p_Camera)
{
    auto renderer = ServiceLocator::GetRenderer();
    auto& device = render_context.getDevice();


}
void ForwardSubpass::prepare()//setup shaders, To be called when adding subpass to the pipeline
{
    //Warming up shaders so vkCreateShaderModule is called
    auto& device = m_RenderContext.getDevice();
    auto renderer = (RendererVulkan*)ServiceLocator::GetRenderer();



    auto pVertexShader = m_VertexShader.lock();
    if (!pVertexShader)
    {
        m_VertexShader = renderer->getShaderSourcePool().getShaderSource("./Shaders/shader.vert");
        pVertexShader = m_VertexShader.lock();
    }
    auto pFragmentShader = m_FragmentShader.lock();
    if (!pFragmentShader)
    {
        m_FragmentShader = renderer->getShaderSourcePool().getShaderSource("./Shaders/shader.frag");
        pFragmentShader = m_FragmentShader.lock();
    }


    
    auto scene = ServiceLocator::GetSceneManager()->GetCurrentScene();
    std::vector<RenderBatch>& batchesOpaque = scene->GetOpaqueBatches();
    for (auto batch : batchesOpaque)
    {
        for (auto node_it = batch.m_ModelsByDistance.begin(); node_it != batch.m_ModelsByDistance.end(); node_it++)
        {
            Model& model = node_it->second;
            auto& vert_module = device.getResourcesCache().request_shader_module(VK_SHADER_STAGE_VERTEX_BIT, pVertexShader, model.getShaderVariant());
            auto& frag_module = device.getResourcesCache().request_shader_module(VK_SHADER_STAGE_FRAGMENT_BIT, pFragmentShader, model.getShaderVariant());
            
        }
    }

    std::vector<RenderBatch>& batchesTransparent = scene->GetTransparentBatches();
    for (auto batch : batchesTransparent)
    {
        for (auto node_it = batch.m_ModelsByDistance.begin(); node_it != batch.m_ModelsByDistance.end(); node_it++)
        {
            Model& model = node_it->second;
            auto& vert_module = device.getResourcesCache().request_shader_module(VK_SHADER_STAGE_VERTEX_BIT, pVertexShader, model.getShaderVariant());
            auto& frag_module = device.getResourcesCache().request_shader_module(VK_SHADER_STAGE_FRAGMENT_BIT, pFragmentShader, model.getShaderVariant());
            
        }
    }
}




void ForwardSubpass::draw(CommandBuffer& primary_commandBuffer) 
{//render geometry within subpass

    auto scene = ServiceLocator::GetSceneManager()->GetCurrentScene();
    if (!scene->IsInit())
        return;
        
    auto& device = m_RenderContext.getDevice();
    auto& activeFrame = m_RenderContext.getActiveFrame();
   

    auto persistentCommands = m_PersistentCommandsPerFrame.getPersistentCommands(activeFrame.getHashId(), device,activeFrame);
    CommandBuffer* command_buffer = persistentCommands->getCommandBuffer();

    if (persistentCommands->getDirty())
    {
       
        recordCommandBuffers(command_buffer, &primary_commandBuffer);
        persistentCommands->clearDirty();

    }
    primary_commandBuffer.execute_commands(*command_buffer);


}

void ForwardSubpass::recordCommandBuffers(CommandBuffer* command_buffer, CommandBuffer* primary_commandBuffer)
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
    command_buffer->bind_buffer(*(m_RenderContext.getActiveFrame().getCameraUniformBuffer()), 0, sizeof(UBOCamera), 0, 1, 0);

    //Bind the lights uniform buffer
    command_buffer->bind_buffer(*((VulkanBuffer*)(scene->getLightsUniformBuffer())), 0, sizeof(UBOLight), 0, 4, 0);

    

    
    std::vector<RenderBatch>& batchesOpaque = scene->GetOpaqueBatches();
  
    for (auto batch : batchesOpaque)
    {
        for (auto node_it = batch.m_ModelsByDistance.begin(); node_it != batch.m_ModelsByDistance.end(); node_it++)
        {
            const Model& model = node_it->second;
            drawModel(model, command_buffer);
        }
       
           
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


    std::vector<RenderBatch>& batchesTransparent = scene->GetTransparentBatches();
    for (auto batch : batchesTransparent)
    {
        for (auto node_it = batch.m_ModelsByDistance.begin(); node_it != batch.m_ModelsByDistance.end(); node_it++)
        {
            const Model& model = node_it->second;
            drawModel(model, command_buffer);
        }


    }


  

    command_buffer->end();
}

void ForwardSubpass::drawModel(const Model& model, CommandBuffer* command_buffer)
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




    auto vertex_input_resources = pipeline_layout.getResources(ShaderResourceType::Input, VK_SHADER_STAGE_VERTEX_BIT);

    VertexInputState vertex_input_state;

    for (auto& input_resource : vertex_input_resources)
    {
        AttributeDescription attributeDescription;

        if (!model.GetMesh().GetAttributeDescription(input_resource.name, attributeDescription))
        {
            continue;
        }

        VkVertexInputAttributeDescription vertex_attribute{};
        vertex_attribute.binding = input_resource.location;
        vertex_attribute.format = attributeDescription.m_Format;
        vertex_attribute.location = input_resource.location;
        vertex_attribute.offset = attributeDescription.m_Offset;

        vertex_input_state.m_Attributes.push_back(vertex_attribute);

        VkVertexInputBindingDescription vertex_binding{};
        vertex_binding.binding = input_resource.location;
        vertex_binding.stride = attributeDescription.m_Stride;

        vertex_input_state.m_Bindings.push_back(vertex_binding);
    }

    command_buffer->setVertexInputState(vertex_input_state);

    
    //Bind Indices buffer
    command_buffer->bind_index_buffer(*((VulkanBuffer*)model.GetMesh().GetIndicesBuffer()), 0, VK_INDEX_TYPE_UINT32);


    for (auto& input_resource : vertex_input_resources)
    {
        const auto& buffer_iter = model.GetMesh().GetVerticesBuffers().find(input_resource.name);

        if (buffer_iter != model.GetMesh().GetVerticesBuffers().end())
        {
            //std::vector<std::reference_wrapper<VulkanBuffer>> buffers;
            VulkanBuffer* vBuff = (VulkanBuffer*)buffer_iter->second.first;
            //buffers.emplace_back(std::ref(*vBuff));

            // Bind vertex buffers only for the attribute locations defined
            command_buffer->bind_vertex_buffer(input_resource.location, *vBuff, { 0 });
        }
    }




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


    int nIndices = model.GetNIndices();
    int indexStart = model.GetIndexStartPosition();
    command_buffer->pushConstants(0, model.getModelMatrix());
  
    command_buffer->draw_indexed(nIndices, 1, indexStart, model.GetVertexStartPosition(), 0);
}

