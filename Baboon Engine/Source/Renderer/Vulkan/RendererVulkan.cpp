#include "RendererVulkan.h"
#include "defines.h"
#include <algorithm>
#include <limits>
#include <fstream>
#include "Core\Model.h"
#include "Core\Scene.h"
#include "Cameras\Camera.h"
#include "VulkanTexture.h"


void PrintVulkanSupportedExtensions()
{
	uint32_t extensionCount = 0;
	vkEnumerateInstanceExtensionProperties(nullptr, &extensionCount, nullptr);
	std::vector<VkExtensionProperties> extensions(extensionCount);
	vkEnumerateInstanceExtensionProperties(nullptr, &extensionCount, extensions.data());
  LOGINFO("available extensions:");
	for (const auto& extension : extensions) {
    LOGINFO("\t" + extension.extensionName);
	}
}

void RendererVulkan::OnWindowResize(int i_NewW, int i_NewH)
{
    
    if (m_RenderPath)
    {
        auto& subpasses = m_RenderPath->getSubPasses();
        for (int i = 0;i<subpasses.size();i++)
        {
            subpasses[i]->invalidatePersistentCommands();
        }
    }
}

void RendererVulkan::WaitToDestroy()
{
	vkDeviceWaitIdle(m_LogicalDevice->get_handle());//Wait till the device is idle so we can destroy stuff
}

void RendererVulkan::Update()
{
    m_LogicalDevice->getResourcesCache().GarbageCollect();

    if (m_SceneLoaded)
    {
        if (m_RenderPath)
        {
            auto& subpasses = m_RenderPath->getSubPasses();
            for (int i = 0; i < subpasses.size(); i++)
            {
                subpasses[i]->invalidatePersistentCommands();
                subpasses[i]->prepare();
                
            }
        }
        m_SceneLoaded = false;
    }
    if (m_Dirty)
    {
        reRecordCommands();
        m_Dirty = false;
    }
    
        
   
   
    
}

float RendererVulkan::GetMainRTAspectRatio()
{
  auto extent = m_RenderContext->getSurfaceExtent();
	return extent.width / (float)extent.height;
}
float RendererVulkan::GetMainRTWidth()
{
	return m_RenderContext->getSurfaceExtent().width;
}
float RendererVulkan::GetMainRTHeight() {
	return m_RenderContext->getSurfaceExtent().height;
}

void RendererVulkan::ReloadShader(std::string shaderPath)
{
   
    //So, the plan is to iterate over the shader source pool and reload the shader source with the new modified source code. That will generate a new ShaderSource::Id, which is used for hashing ShaderSource. The result of this will be that when trying to fetch a shader module, it won't exist
    //in the cache so we will need to create a new one. The same for the pipeline and so on...
    LOGDEBUG("Reloading shader: " + shaderPath);
    m_ShaderSourcePool.reloadShader(shaderPath);
    reRecordCommands();
}


int RendererVulkan::Init(std::vector<const char*>& required_extensions, GLFWwindow* i_window, const Camera* p_Camera)
{
    m_ThreadCount = std::thread::hardware_concurrency();

    PrintVulkanSupportedExtensions();
    if (m_bEnableValidationLayers)
        required_extensions.push_back(VK_EXT_DEBUG_REPORT_EXTENSION_NAME);

    m_Instance = std::make_unique<Instance>(required_extensions, m_bEnableValidationLayers);
    createSurface(i_window);
    pickPhysicalDevice();
    m_LogicalDevice = std::make_unique<Device>(m_PhysicalDevice, m_Surface, m_VvalidationLayers, deviceExtensions);


    int width, height;
    glfwGetWindowSize(i_window, &width, &height);

    m_RenderContext = std::make_unique<VulkanContext>(*m_LogicalDevice, m_Surface, width, height);
    m_RenderContext->prepare(m_ThreadCount,RenderTarget::DEFERRED_CREATE_FUNC);

   
  
    auto geoSubPass = std::make_unique<GeometrySubpass>(*m_RenderContext, "./Shaders/geo.vert", "./Shaders/geo.frag", m_ThreadCount);
    geoSubPass->setOutputAttachments({ 1,2,3 });

   
    auto lightSubPass = std::make_unique<LightSubpass>(*m_RenderContext, "./Shaders/light.vert", "./Shaders/light.frag");
    lightSubPass->setInputAttachments({ 1,2,3 });
    lightSubPass->setDisableDepthAttachment();//We can't read and write to depth!

    auto transparentSubpass = std::make_unique<TransparentSubpass>(*m_RenderContext, "./Shaders/transparent.vert", "./Shaders/transparent.frag", m_ThreadCount);

    m_RenderPath = std::make_unique<RenderPath>();
    m_RenderPath->add_subpass(std::move(geoSubPass));
    m_RenderPath->add_subpass(std::move(lightSubPass));
    m_RenderPath->add_subpass(std::move(transparentSubpass));
    

    std::vector<VkClearValue> clear_value{ 4 };
    clear_value[0].color = { {0.4f, 0.4f, 0.4f, 0.0f} };//w is the specular
    clear_value[1].depthStencil = { 1.0f, 0 };
    clear_value[2].color = { {0.4f, 0.4f, 0.4f, 0.0f} };
    clear_value[3].color = { {0.0f, 0.0f, 0.0f, 0.0f} };//First 3 components are the normal, last is the ambient. Initialised to 1.0 since the "sky" needs all the ambient. TODO: make last component material id instead

    std::vector<LoadStoreInfo> load_store{ 4 };

    // Swapchain
    load_store[0].load_op = VK_ATTACHMENT_LOAD_OP_CLEAR;
    load_store[0].store_op = VK_ATTACHMENT_STORE_OP_STORE;

    // Depth
    load_store[1].load_op = VK_ATTACHMENT_LOAD_OP_CLEAR;
    load_store[1].store_op = VK_ATTACHMENT_STORE_OP_STORE;

    // Albedo
    load_store[2].load_op = VK_ATTACHMENT_LOAD_OP_CLEAR;
    load_store[2].store_op = VK_ATTACHMENT_STORE_OP_STORE;

    // Normal
    load_store[3].load_op = VK_ATTACHMENT_LOAD_OP_CLEAR;
    load_store[3].store_op = VK_ATTACHMENT_STORE_OP_STORE;

    m_RenderPath->setClearValue(clear_value);
    m_RenderPath->setLoadStoreValue(load_store);
    
    VulkanImGUI* gui = (VulkanImGUI* )ServiceLocator::GetGUI();
    gui->Init(i_window, m_RenderContext.get(), this);
    


    //Shadow stuff init
    VkExtent3D shadowExtent = { SHADOWMAP_RESOLUTION,SHADOWMAP_RESOLUTION,1 };
    VulkanImage shadowMap= VulkanImage(            *m_LogicalDevice, shadowExtent,
                                                            SHADOWMAP_FORMAT, VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
                                                            VMA_MEMORY_USAGE_GPU_ONLY, VK_SAMPLE_COUNT_1_BIT,
                                                            1, MAX_DEFERRED_DIR_LIGHTS);//TODO: Assuming the dir lights are the shadow casters

    m_ShadowRT = RenderTarget::SHADOWMAP_CREATE_FUNC(std::move(shadowMap));
    m_ShadowPath = std::make_unique<RenderPath>();
    auto shadowSubpass = std::make_unique<ShadowSubpass>(*m_RenderContext, "./Shaders/shadow.vert", "./Shaders/shadow.geom");
    m_ShadowPath->add_subpass(std::move(shadowSubpass));

    std::vector<VkClearValue> shadowClear{ 1 };
    shadowClear[0].depthStencil = { 1.0f, 0 };
    std::vector<LoadStoreInfo> shadow_LoadStore{ 1 };
    shadow_LoadStore[0].load_op = VK_ATTACHMENT_LOAD_OP_CLEAR;
    shadow_LoadStore[0].store_op = VK_ATTACHMENT_STORE_OP_STORE;
    m_ShadowPath->setClearValue(shadowClear);
    m_ShadowPath->setLoadStoreValue(shadow_LoadStore);
    ////////////////////////////////////////////////Shadow stuff init/////////////////////////////





    ServiceLocator::GetSceneManager()->GetSubject().Register(this);
    ServiceLocator::GetCameraManager()->GetSubject().Register(this);

    return true;
    //return (result == VK_SUCCESS);

}




void RendererVulkan::DrawFrame()
{

  




  auto& command_buffer = m_RenderContext->begin();//Grab a command buffer from the render context



   //Shadows should not depend on previous frames therefore doing it before all the swapchain sync stuff
  if (ServiceLocator::GetSceneManager()->GetCurrentScene()->IsInit() && m_ShadowPath)
  {
      auto& command_bufferShadows = m_RenderContext->getActiveFrame().requestCommandBuffer(m_LogicalDevice->getGraphicsQueue());
      auto res = command_bufferShadows.begin(VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT);//Call begin to start recording commands
      assert(!res, "Error starting commandbuffer recording");


      ImageMemoryBarrier beginBarrier{};
      beginBarrier.old_layout = VK_IMAGE_LAYOUT_UNDEFINED;
      beginBarrier.new_layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
      beginBarrier.src_access_mask = 0;
      beginBarrier.dst_access_mask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
      beginBarrier.src_stage_mask = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
      beginBarrier.dst_stage_mask = VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT | VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT;

      command_bufferShadows.imageBarrier(m_ShadowRT->getViews()[0], beginBarrier);



      VkViewport viewport{};
      viewport.width = SHADOWMAP_RESOLUTION;
      viewport.height = SHADOWMAP_RESOLUTION;
      viewport.minDepth = 0.0f;
      viewport.maxDepth = 1.0f;
      command_bufferShadows.setViewport(0, { viewport });

      VkRect2D scissor{};
      scissor.extent = VkExtent2D{ SHADOWMAP_RESOLUTION,SHADOWMAP_RESOLUTION };
      command_bufferShadows.setScissor(0, { scissor });



      m_ShadowPath->draw(command_bufferShadows, *m_ShadowRT);
      command_bufferShadows.endRenderPass();

      //TODO: This is very harsh sync when it works make it proper
      ImageMemoryBarrier memory_barrier{};
      memory_barrier.old_layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
      memory_barrier.new_layout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
      memory_barrier.src_access_mask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
      memory_barrier.src_stage_mask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
      memory_barrier.dst_stage_mask = VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT;

      command_bufferShadows.imageBarrier(m_ShadowRT->getViews()[0], memory_barrier);
      command_bufferShadows.end();
      m_LogicalDevice->getGraphicsQueue().submit(command_bufferShadows, m_LogicalDevice->requestFence());
      m_LogicalDevice->getFencePool().wait();
      m_LogicalDevice->getFencePool().reset();
      m_LogicalDevice->getCommandPool().reset_pool();
  }
  



  auto result = command_buffer.begin(VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT);//Call begin to start recording commands
  assert(!result, "Error starting commandbuffer recording");

  
  auto& renderTarget = m_RenderContext->getActiveFrame().getRenderTarget();//Grab the render target
  renderTarget.startOfFrameMemoryBarrier(command_buffer);//Call this function to do the render target images memory transitions



  //Set viewport and scissors
  auto& extent = renderTarget.getExtent();

  VkViewport viewport{};
  viewport.width = static_cast<float>(extent.width);
  viewport.height = static_cast<float>(extent.height);
  viewport.minDepth = 0.0f;
  viewport.maxDepth = 1.0f;
  command_buffer.setViewport(0, { viewport });

  VkRect2D scissor{};
  scissor.extent = extent;
  command_buffer.setScissor(0, { scissor });
 

 
  if(m_RenderPath)//If we have a pipeline set, call its draw function
    m_RenderPath->draw(command_buffer, renderTarget, VK_SUBPASS_CONTENTS_SECONDARY_COMMAND_BUFFERS);

  VulkanImGUI* gui = (VulkanImGUI*)ServiceLocator::GetGUI();
  if (gui)
  {
      gui->Draw(command_buffer);
  }
      
      
  command_buffer.endRenderPass();

  renderTarget.presentFrameMemoryBarrier(command_buffer);//Transition the images so they can be presented

  command_buffer.end();//End recording the command buffer
  m_RenderContext->submit(command_buffer);//Submit the command buffer to the graphics queue

	
}


void RendererVulkan::Destroy()	
{
    m_RenderContext.reset();//Forcing the swapchain to be destroyed before the surface otherwise validation complains
    if (m_Surface != VK_NULL_HANDLE)
    {
        vkDestroySurfaceKHR(m_Instance->get_handle(), m_Surface, nullptr);
    }
}


void RendererVulkan::createSurface( GLFWwindow* i_window)
{
	VkWin32SurfaceCreateInfoKHR createInfo = {};
	createInfo.sType = VK_STRUCTURE_TYPE_WIN32_SURFACE_CREATE_INFO_KHR;
	createInfo.hwnd = glfwGetWin32Window(i_window);
	createInfo.hinstance = GetModuleHandle(nullptr);

  
	auto CreateWin32SurfaceKHR = (PFN_vkCreateWin32SurfaceKHR)vkGetInstanceProcAddr(m_Instance->get_handle(), "vkCreateWin32SurfaceKHR");

	if (!CreateWin32SurfaceKHR || CreateWin32SurfaceKHR(m_Instance->get_handle(), &createInfo,
		nullptr, &m_Surface) != VK_SUCCESS) {
		throw std::runtime_error("failed to create window surface!");
	}
	
}

static void printPhysicalDeviceInfo(VkPhysicalDevice device)
{
	VkPhysicalDeviceProperties deviceProperties;
	vkGetPhysicalDeviceProperties(device, &deviceProperties);
  LOGINFO("\n*Device Name = " + deviceProperties.deviceName);
}

 bool  RendererVulkan::isDeviceSuitable(VkPhysicalDevice device) {

     VkPhysicalDeviceProperties properties{};
     vkGetPhysicalDeviceProperties(device, &properties);
     return  (properties.deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU);
}


void  RendererVulkan::pickPhysicalDevice()
{
	uint32_t deviceCount = 0;
	vkEnumeratePhysicalDevices(m_Instance->get_handle(), &deviceCount, nullptr);


	if (deviceCount == 0) 
	{
		throw std::runtime_error("failed to find GPUs with Vulkan support!");
	}


	std::vector<VkPhysicalDevice> devices(deviceCount);
	vkEnumeratePhysicalDevices(m_Instance->get_handle(), &deviceCount, devices.data());


	for (const auto& device : devices) {
		if (isDeviceSuitable(device)) {
			m_PhysicalDevice = device;
      LOGINFO("\nUsing phys device ");
			printPhysicalDeviceInfo(m_PhysicalDevice);
			break;
		}
	}

	if (m_PhysicalDevice == VK_NULL_HANDLE) {
		throw std::runtime_error("failed to find a suitable GPU!");
	}

}

void RendererVulkan::reRecordCommands()
{
    if (m_RenderPath)
    {
        auto& subpasses = m_RenderPath->getSubPasses();
        for (int i = 0; i < subpasses.size(); i++)
        {
            subpasses[i]->setReRecordCommands();
        }
    }
}


Buffer* RendererVulkan::CreateVertexBuffer( void*  i_data, size_t iBufferSize)
{
    Buffer* buffer;

    m_Buffers.emplace_back(*m_LogicalDevice, iBufferSize, VK_BUFFER_USAGE_VERTEX_BUFFER_BIT, VMA_MEMORY_USAGE_GPU_TO_CPU);
    buffer = &m_Buffers.back();
    if(i_data)
        buffer->update(i_data, iBufferSize);

    return buffer;
	
}

Buffer* RendererVulkan::CreateIndexBuffer( void*  i_data, size_t iBufferSize)
{
    Buffer* buffer;
    m_Buffers.emplace_back(*m_LogicalDevice, iBufferSize, VK_BUFFER_USAGE_INDEX_BUFFER_BIT, VMA_MEMORY_USAGE_GPU_TO_CPU);
    buffer = &m_Buffers.back();
    if(i_data)
        buffer->update(i_data, iBufferSize);

    return buffer;
}


void RendererVulkan::DeleteBuffer(Buffer* buffer)
{
    VulkanBuffer* vulkanBuffer = (VulkanBuffer*)buffer;
    auto it = find_if(m_Buffers.begin(), m_Buffers.end(), [vulkanBuffer](VulkanBuffer& bufferInContainer) {
        
        return vulkanBuffer->getHandle() == bufferInContainer.getHandle();
    
    });
    if (it != m_Buffers.end())
    {
        m_Buffers.erase(it);
        buffer = nullptr;
    }
}

void RendererVulkan::DeleteTexture(Texture* texture)
{
    VulkanTexture* vulkanTexture = (VulkanTexture*)texture;
    auto itImage = find_if(m_Images.begin(), m_Images.end(), [vulkanTexture](VulkanImage& imageInContainer) {

        return vulkanTexture->getImage()->getHandle() == imageInContainer.getHandle();

    });

    auto itImageView = find_if(m_ImageViews.begin(), m_ImageViews.end(), [vulkanTexture](VulkanImageView& imageViewInContainer) {

        return vulkanTexture->getImageView()->getHandle() == imageViewInContainer.getHandle();

    });
    if (itImageView != m_ImageViews.end())
    {
        m_ImageViews.erase(itImageView);
    }
    if (itImage != m_Images.end())
    {
        m_Images.erase(itImage);
    }
    
}




Buffer* RendererVulkan::CreateStaticUniformBuffer( void*  i_data, size_t iBufferSize)
{
    Buffer* buffer;

    m_Buffers.emplace_back(*m_LogicalDevice, iBufferSize, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, VMA_MEMORY_USAGE_GPU_TO_CPU);
    buffer = &m_Buffers.back();
    if(i_data)
        buffer->update(i_data, iBufferSize);

    return buffer;

}
Buffer* RendererVulkan::CreateInstancedUniformBuffer( void*  i_data, size_t iBufferSize)
{
	
    return nullptr;
}

void RendererVulkan::DeleteStaticUniformBuffer()
{



}
void RendererVulkan::DeleteInstancedUniformBuffer()
{


}

void RendererVulkan::UpdateTimesAndFPS(std::chrono::time_point<std::chrono::high_resolution_clock>  i_tStartTime)
{
	m_FrameCounter++;
	auto tEnd = std::chrono::high_resolution_clock::now();
	auto tDiff = std::chrono::duration<double, std::milli>(tEnd - i_tStartTime).count();
	m_LastFrameTime = (float)tDiff / 1000.0f;

	
	m_FpsTimer += (float)tDiff;
	if (m_FpsTimer > 1000.0f)//Update each second
	{
		m_LastFPS = static_cast<uint32_t>(1.0f / m_LastFrameTime);
		m_FpsTimer = 0.0f;
		m_FrameCounter = 0.0f;
	}
	
}

void RendererVulkan::ObserverUpdate(int message, void* data)
{
    if (message == Subject::Message::SCENELOADED)
    {
        //LOGINFO("Renderer knows scene loaded");
        m_SceneLoaded = true;
      
    }
   
    else if (message == Subject::CAMERADIRTY)
    {
        //LOGINFO("Renderer knows CAMERADIRTY");
        auto& renderFrames = m_RenderContext->getRenderFrames();
        for (auto& frame : renderFrames)
            frame->setCameraUniformDirty();
        m_Dirty = true;
    }
    else if (message == Subject::SCENEDIRTY)
    {
        m_Dirty = true;
    }
    else if (message == Subject::LIGHTDIRTY)
    {
        m_Dirty = true;
        auto& renderFrames = m_RenderContext->getRenderFrames();
        for (auto& frame : renderFrames)
            frame->setShadowsUniformDirty();
        m_Dirty = true;
    }
}





Texture* RendererVulkan::CreateTexture(void* pPixels, int i_Widht, int i_Height)
{
    VulkanTexture* texture = new VulkanTexture();
    auto& command_buffer = m_LogicalDevice->requestCommandBuffer();
    command_buffer.begin(VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT, 0);




    VkExtent3D extent{ i_Widht,i_Height,1 };

    m_Images.emplace_back(*m_LogicalDevice, extent, VK_FORMAT_R8G8B8A8_UNORM, VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT, VMA_MEMORY_USAGE_GPU_ONLY);

    texture->setImage(&m_Images.back());

    m_ImageViews.emplace_back(*texture->getImage(), VK_IMAGE_VIEW_TYPE_2D);
    texture->setImageView(&m_ImageViews.back());

    size_t size = i_Widht * i_Height * 4 * sizeof(unsigned char);//we are forcing 4 channels with the STBI_rgb_alpha flag
    VulkanBuffer stage_buffer{ *m_LogicalDevice,
                              size,
                              VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
                              VMA_MEMORY_USAGE_CPU_ONLY };


    stage_buffer.update(pPixels, size);
   

    /////Lets upload image to the gpu
    {
        ImageMemoryBarrier memory_barrier{};
        memory_barrier.old_layout = VK_IMAGE_LAYOUT_UNDEFINED;
        memory_barrier.new_layout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
        memory_barrier.src_access_mask = 0;
        memory_barrier.dst_access_mask = VK_ACCESS_TRANSFER_WRITE_BIT;
        memory_barrier.src_stage_mask = VK_PIPELINE_STAGE_HOST_BIT;
        memory_barrier.dst_stage_mask = VK_PIPELINE_STAGE_TRANSFER_BIT;

        command_buffer.imageBarrier(*texture->getImageView(), memory_barrier);
    }

    std::vector<VkBufferImageCopy> buffer_copy_regions(1);//TODO: We are assuming mipmaps = 1
    auto& copy_region = buffer_copy_regions[0];
    copy_region.bufferOffset = 0;//mipmap.offset;
    copy_region.imageSubresource = texture->getImageView()->getSubresourceLayers();
    // Update miplevel
    copy_region.imageSubresource.mipLevel = 0;//mipmap.level;
    VkExtent3D mipExtent{ i_Widht,i_Height,1 };
    copy_region.imageExtent = mipExtent;//mipmap.extent;

    command_buffer.copy_buffer_to_image(stage_buffer, *texture->getImage(), buffer_copy_regions);

    //Transition image to readable by the shader
    {
        ImageMemoryBarrier memory_barrier{};
        memory_barrier.old_layout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
        memory_barrier.new_layout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
        memory_barrier.src_access_mask = VK_ACCESS_TRANSFER_WRITE_BIT;
        memory_barrier.dst_access_mask = VK_ACCESS_SHADER_READ_BIT;
        memory_barrier.src_stage_mask = VK_PIPELINE_STAGE_TRANSFER_BIT;
        memory_barrier.dst_stage_mask = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;

        command_buffer.imageBarrier(*texture->getImageView(), memory_barrier);
    }


    command_buffer.end();

    auto& queue = m_LogicalDevice->getResourceTransferQueue();

    queue.submit(command_buffer, m_LogicalDevice->requestFence());

    m_LogicalDevice->getFencePool().wait();
    m_LogicalDevice->getFencePool().reset();
    m_LogicalDevice->getCommandPool().reset_pool();
    //m_LogicalDevice->wait_idle();

    //!TODO END/////////////////////////////////////////////////////////////////////


    //I think this same sampler can be used for all the textures TODO: Make it shareable
    if (m_Samplers.empty())
    {
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

        m_Samplers.emplace_back(*m_LogicalDevice, samplerInfo);
    }
    texture->setSampler(&m_Samplers.back());
    return texture;
}




