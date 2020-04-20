#include "RendererVulkan.h"
#include "defines.h"
#include <algorithm>
#include <limits>
#include <fstream>
#include "Core\Model.h"
#include "Core\Scene.h"
#include "Cameras\Camera.h"


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
	
}

void RendererVulkan::WaitToDestroy()
{
	vkDeviceWaitIdle(m_LogicalDevice->get_handle());//Wait till the device is idle so we can destroy stuff
}

void RendererVulkan::Update()
{
    if (m_GUI)
        m_GUI->DoUI();
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
void RendererVulkan::SetupRenderCalls()
{

}


int RendererVulkan::Init(std::vector<const char*>& required_extensions, GLFWwindow* i_window, const Camera* p_Camera)
{

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
    m_RenderContext->prepare();



    auto& vertexShader = ShaderSource{ "Shaders/vert.spv" };
    auto& fragmentShader = ShaderSource{ "Shaders/frag.spv" };

  
    auto subpass = std::make_unique<TestTriangleSubPass>(*m_RenderContext,std::move(vertexShader),std::move(fragmentShader),p_Camera);//Here we are adding our own test subpass TODO: Add the real thing
    m_RenderPath = std::make_unique<RenderPath>();
    m_RenderPath->add_subpass(std::move(subpass));

    
    
//TODO make gui work!    
    m_GUI = std::make_unique<VulkanImGUI>(*m_RenderContext);
    m_GUI->Init(i_window);
    
    
    return true;
    //return (result == VK_SUCCESS);

}




void RendererVulkan::DrawFrame()
{
	 
  auto& command_buffer = m_RenderContext->begin();//Grab a command buffer from the render context
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
    m_RenderPath->draw(command_buffer, renderTarget); 

  if (m_GUI)
      m_GUI->Draw(command_buffer);
      
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


void  RendererVulkan::CreateVertexBuffer(const void*  i_data, size_t iBufferSize)
{

	
}

void RendererVulkan::CreateIndexBuffer(const void*  i_data, size_t iBufferSize)
{
	
}


void RendererVulkan::DeleteVertexBuffer()
{
	
}
void RendererVulkan::DeleteIndexBuffer()
{

}

void RendererVulkan::CreateStaticUniformBuffer(const void*  i_data, size_t iBufferSize)
{



}
void RendererVulkan::CreateInstancedUniformBuffer(const void*  i_data, size_t iBufferSize)
{
	

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



