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


Subpass::Subpass(VulkanContext& render_context, std::string vertex_shader, std::string fragment_shader):
    m_RenderContext(render_context),
    m_VertexShaderPath(vertex_shader),
    m_FragmentShaderPath(fragment_shader)
{
    getVertexShader();
    getFragmentShader();
   
}

 Subpass::~Subpass()
{
    delete m_ThreadPool;
}

 void Subpass::updateRenderTargetAttachments()
 {
     auto& render_target = m_RenderContext.getActiveFrame().getRenderTarget();

     render_target.setOutputAttachments(m_OutputAttachments);
     render_target.setInputAttachments(m_InputAttachments);
 }

 void Subpass::invalidatePersistentCommands()
{
    m_PersistentCommandsPerFrame.resetPersistentCommands();
}

void Subpass::setReRecordCommands()
{
    m_PersistentCommandsPerFrame.setAllDirty();
}

std::shared_ptr<ShaderSource> Subpass::getVertexShader()
{
    auto renderer = (RendererVulkan*)ServiceLocator::GetRenderer();

    auto pVertexShader = m_VertexShader.lock();
    if (!pVertexShader)
    {
        m_VertexShader = renderer->getShaderSourcePool().getShaderSource(m_VertexShaderPath);
        pVertexShader = m_VertexShader.lock();
    }
    return pVertexShader;
}

std::shared_ptr<ShaderSource> Subpass::getFragmentShader()
{
    auto renderer = (RendererVulkan*)ServiceLocator::GetRenderer();
    auto pFragmentShader = m_FragmentShader.lock();
    if (!pFragmentShader)
    {
        m_FragmentShader = renderer->getShaderSourcePool().getShaderSource(m_FragmentShaderPath);
        pFragmentShader = m_FragmentShader.lock();
    }
    return pFragmentShader;
}

void Subpass::drawBatchList(std::vector<RenderBatch>& batches, CommandBuffer* primary_commandBuffer, std::vector<CommandBuffer*>& recordedCommands)
{
    if (batches.size() == 0)
        return;
    auto& device = m_RenderContext.getDevice();
    auto& activeFrame = m_RenderContext.getActiveFrame();
    size_t nBatches = batches.size();
    size_t nCommandBuffers = 1;//batches.size();
    size_t threadsToUse = m_ThreadPool->threads.size();
    if (nBatches < threadsToUse)
    {
        threadsToUse = nBatches;
    }
    size_t nBatchesPerThread = nBatches / threadsToUse;
    size_t remainderBatches = nBatches % threadsToUse;


    size_t beginIndex = 0;


    for (int i = 0; i < threadsToUse; i++)
    {
        size_t nBatches = nBatchesPerThread;
        if (remainderBatches > 0)
        {
            nBatches++;
            remainderBatches--;
        }
        size_t endIndex = beginIndex + nBatches;
        auto persistentCommandsPerThread = m_PersistentCommandsPerFrame.getPersistentCommands(activeFrame.getHashId(), i, device, activeFrame);

        auto& command_buffers = persistentCommandsPerThread->getCommandBuffers(nCommandBuffers);

        recordedCommands.insert(recordedCommands.end(), command_buffers.begin(), command_buffers.end());

        m_ThreadPool->threads[i]->addJob([this, command_buffers, &primary_commandBuffer, &batches, beginIndex, endIndex]() {recordCommandBuffers(command_buffers, primary_commandBuffer, batches, beginIndex, endIndex); });

        beginIndex = endIndex;
    }
    m_ThreadPool->wait();
}


void Subpass::recordBatches(CommandBuffer* command_buffer, CommandBuffer* primary_commandBuffer, std::vector<RenderBatch>& batches, size_t beginIndex, size_t endIndex)
{
    for (int i = 0; i < (endIndex - beginIndex); i++)
    {
        auto& batch = batches[beginIndex + i];
        for (auto node_it = batch.m_ModelsByDistance.begin(); node_it != batch.m_ModelsByDistance.end(); node_it++)
        {
            const Model& model = node_it->second;
            drawModel(model, command_buffer);
        }
    }

}
void Subpass::recordCommandBuffers(std::vector<CommandBuffer*> command_buffers, CommandBuffer* primary_commandBuffer, std::vector<RenderBatch>& batches, size_t beginIndex, size_t endIndex)
{
    assert(command_buffers.size() > 0, "Command buffers cant be empty!");

    auto scene = ServiceLocator::GetSceneManager()->GetCurrentScene();
    auto& renderTarget = m_RenderContext.getActiveFrame().getRenderTarget();//Grab the render target

    size_t nBatches = endIndex - beginIndex;
    size_t nBatchesPerCommand= nBatches / command_buffers.size();
    size_t remainderBatches = nBatches % command_buffers.size();
    size_t localBeginIndex = beginIndex;
    for (CommandBuffer* command_buffer : command_buffers)
    {
        size_t nBatches = nBatchesPerCommand;
        if (remainderBatches > 0)
        {
            nBatches++;
            remainderBatches--;
        }
        size_t localEndIndex = localBeginIndex + nBatches;

        //Set viewport and scissors
        /*auto& extent = renderTarget.getExtent();
        VkViewport viewport{};
        viewport.width = static_cast<float>(extent.width);
        viewport.height = static_cast<float>(extent.height);
        viewport.minDepth = 0.0f;
        viewport.maxDepth = 1.0f;
        VkRect2D scissor{};
        scissor.extent = extent;*/

        command_buffer->begin(VK_COMMAND_BUFFER_USAGE_RENDER_PASS_CONTINUE_BIT, primary_commandBuffer);
        //command_buffer->setViewport(0, { viewport });
        //command_buffer->setScissor(0, { scissor });
  
        recordBatches(command_buffer, primary_commandBuffer, batches, localBeginIndex, localEndIndex);

        command_buffer->end();

        localBeginIndex = localEndIndex;

    }

}

void Subpass::drawModel(const Model& model, CommandBuffer* command_buffer)
{

    auto& device = m_RenderContext.getDevice();
    auto renderVulkan =(RendererVulkan*) ServiceLocator::GetRenderer();

    auto pVertexShader = getVertexShader();
    auto pFragmentShader = getFragmentShader();

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
            VulkanBuffer* vBuff = (VulkanBuffer*)buffer_iter->second.first;

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





GeometrySubpass::GeometrySubpass(VulkanContext& render_context, std::string vertex_shader, std::string fragment_shader, size_t nThreads) :
    Subpass{ render_context,vertex_shader,fragment_shader }

{
    auto renderer = ServiceLocator::GetRenderer();
    auto& device = render_context.getDevice();

    m_ThreadPool = new ThreadPool();
    m_ThreadPool->setThreadCount(nThreads);
}
void GeometrySubpass::prepare()//setup shaders, To be called when adding subpass to the pipeline
{
    //Warming up shaders so vkCreateShaderModule is called
    auto& device = m_RenderContext.getDevice();
    auto renderer = (RendererVulkan*)ServiceLocator::GetRenderer();


    auto pVertexShader = getVertexShader();
    auto pFragmentShader = getFragmentShader();

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

}

void GeometrySubpass::draw(CommandBuffer& primary_commandBuffer)
{

    auto scene = ServiceLocator::GetSceneManager()->GetCurrentScene();
    if (!scene->IsInit())
        return;

    auto& activeFrame = m_RenderContext.getActiveFrame();

    if (m_PersistentCommandsPerFrame.getDirty(activeFrame.getHashId())) {

        std::vector<CommandBuffer*>& recordedCommands = m_PersistentCommandsPerFrame.startRecording(activeFrame.getHashId());
        std::vector<RenderBatch>& batchesOpaque = scene->GetOpaqueBatches();


        primary_commandBuffer.bind_buffer(*(m_RenderContext.getActiveFrame().getCameraUniformBuffer()), 0, sizeof(UBOCamera), 0, 1, 0);
        drawBatchList(batchesOpaque, &primary_commandBuffer, recordedCommands);
        m_PersistentCommandsPerFrame.clearDirty(activeFrame.getHashId());

    }

    primary_commandBuffer.execute_commands(m_PersistentCommandsPerFrame.getPreRecordedCommands(activeFrame.getHashId()));

}



LightSubpass::LightSubpass(VulkanContext& render_context,  std::string vertex_shader, std::string fragment_shader):
     Subpass(render_context, vertex_shader, fragment_shader) 
{

}

void LightSubpass::draw(CommandBuffer& primary_command)
{
    auto scene = ServiceLocator::GetSceneManager()->GetCurrentScene();
    if (!scene->IsInit())
        return;


    auto& device = m_RenderContext.getDevice();
    auto renderVulkan = (RendererVulkan*)ServiceLocator::GetRenderer();
    auto& render_target = m_RenderContext.getActiveFrame().getRenderTarget();

    std::vector<CommandBuffer*>& recordedCommands = m_PersistentCommandsPerFrame.startRecording(m_RenderContext.getActiveFrame().getHashId());
    auto persistentCommands = m_PersistentCommandsPerFrame.getPersistentCommands(m_RenderContext.getActiveFrame().getHashId(), 0, device, m_RenderContext.getActiveFrame());
    auto& command_buffers = persistentCommands->getCommandBuffers(1);
    auto& command_buffer = *command_buffers[0];
    
    //Set viewport and scissors
    /*auto& extent = render_target.getExtent();
    VkViewport viewport{};
    viewport.width = static_cast<float>(extent.width);
    viewport.height = static_cast<float>(extent.height);
    viewport.minDepth = 0.0f;
    viewport.maxDepth = 1.0f;
    VkRect2D scissor{};
    scissor.extent = extent;*/
    command_buffer.begin(VK_COMMAND_BUFFER_USAGE_RENDER_PASS_CONTINUE_BIT, &primary_command);

    /*command_buffer.setViewport(0, { viewport });
    command_buffer.setScissor(0, { scissor });*/

    auto pVertexShader = getVertexShader();
    auto pFragmentShader = getFragmentShader();
    ShaderVariant emptyVariant;
    auto& vert_module = device.getResourcesCache().request_shader_module(VK_SHADER_STAGE_VERTEX_BIT, pVertexShader, emptyVariant);
    auto& frag_module = device.getResourcesCache().request_shader_module(VK_SHADER_STAGE_FRAGMENT_BIT, pFragmentShader, emptyVariant);
    std::vector<ShaderModule*> shader_modules{ &vert_module, &frag_module };

    auto& pipeline_layout = device.getResourcesCache().request_pipeline_layout(shader_modules);
    command_buffer.bindPipelineLayout(pipeline_layout);

    // Get image views of the attachments
   
    auto& target_views = render_target.getViews();

    // Bind depth, albedo, and normal as input attachments
    auto& depth_view = target_views.at(1);
    command_buffer.bind_input(depth_view, 0, 0, 0);

    auto& albedo_view = target_views.at(2);
    command_buffer.bind_input(albedo_view, 0, 1, 0);

    auto& normal_view = target_views.at(3);
    command_buffer.bind_input(normal_view, 0, 2, 0);



    //Bind matrices
    command_buffer.bind_buffer(*(m_RenderContext.getActiveFrame().getCameraUniformBuffer()), 0, sizeof(UBOCamera), 0, 3, 0);

    //Bind the lights uniform buffer
    command_buffer.bind_buffer(*((VulkanBuffer*)(scene->getLightsUniformBuffer())), 0, sizeof(UBODeferredLights), 0, 4, 0);

    // Set cull mode to front as full screen triangle is clock-wise
    RasterizationState rasterization_state;
    rasterization_state.m_CullMode = VK_CULL_MODE_FRONT_BIT;
    command_buffer.setRasterState(rasterization_state);


    // Draw full screen triangle triangle
    command_buffer.draw(3, 1, 0, 0);


    command_buffer.end();
    std::vector<CommandBuffer*> commands = { &command_buffer };
    primary_command.execute_commands(commands);

}






TransparentSubpass::TransparentSubpass(VulkanContext& render_context, std::string vertex_shader, std::string fragment_shader, size_t nThreads) :
    Subpass{ render_context,vertex_shader,fragment_shader }

{
    auto renderer = ServiceLocator::GetRenderer();
    auto& device = render_context.getDevice();

    m_ThreadPool = new ThreadPool();
    m_ThreadPool->setThreadCount(nThreads);
}
void TransparentSubpass::prepare()//setup shaders, To be called when adding subpass to the pipeline
{
    //Warming up shaders so vkCreateShaderModule is called
    auto& device = m_RenderContext.getDevice();
    auto renderer = (RendererVulkan*)ServiceLocator::GetRenderer();

    auto pVertexShader = getVertexShader();
    auto pFragmentShader = getFragmentShader();

    auto scene = ServiceLocator::GetSceneManager()->GetCurrentScene();
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


void TransparentSubpass::draw(CommandBuffer& primary_commandBuffer)
{

    auto scene = ServiceLocator::GetSceneManager()->GetCurrentScene();
    if (!scene->IsInit())
        return;

    auto& activeFrame = m_RenderContext.getActiveFrame();

    if (m_PersistentCommandsPerFrame.getDirty(activeFrame.getHashId())) {

        std::vector<CommandBuffer*>& recordedCommands = m_PersistentCommandsPerFrame.startRecording(activeFrame.getHashId());
        std::vector<RenderBatch>& batchesTransparent = scene->GetTransparentBatches();
        primary_commandBuffer.bind_buffer(*(m_RenderContext.getActiveFrame().getCameraUniformBuffer()), 0, sizeof(UBOCamera), 0, 1, 0);
        primary_commandBuffer.bind_buffer(*((VulkanBuffer*)(scene->getLightsUniformBuffer())), 0, sizeof(UBODeferredLights), 0, 4, 0);

        // Enable alpha blending
        ColorBlendAttachmentState color_blend_attachment{};
        color_blend_attachment.m_BlendEnable = VK_TRUE;
        color_blend_attachment.m_SrcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA;
        color_blend_attachment.m_DstColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
        color_blend_attachment.m_SrcAlphaBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;

        ColorBlendState color_blend_state{};
        color_blend_state.m_Attachments.resize(getOutputAttachments().size());
        color_blend_state.m_Attachments[0] = color_blend_attachment;
        primary_commandBuffer.setColorBlendState(color_blend_state);

        DepthStencilState depth_stencil_state{};
        depth_stencil_state.m_DepthWriteEnable = false;
        primary_commandBuffer.setDepthStencilState(depth_stencil_state);



        drawBatchList(batchesTransparent, &primary_commandBuffer, recordedCommands);
        m_PersistentCommandsPerFrame.clearDirty(activeFrame.getHashId());

    }

    primary_commandBuffer.execute_commands(m_PersistentCommandsPerFrame.getPreRecordedCommands(activeFrame.getHashId()));

}

