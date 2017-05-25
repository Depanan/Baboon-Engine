#include "RendererVulkan.h"
#include "defines.h"
#include <algorithm>
#include <limits>
#include <fstream>
#include "Core\Model.h"
#include "Core\Scene.h"


bool hasStencilComponent(VkFormat format) {
	return format == VK_FORMAT_D32_SFLOAT_S8_UINT || format == VK_FORMAT_D24_UNORM_S8_UINT;
}


bool checkValidationLayerSupport(const std::vector<const char*>& validationLayers) {
	uint32_t layerCount;
	vkEnumerateInstanceLayerProperties(&layerCount, nullptr);

	std::vector<VkLayerProperties> availableLayers(layerCount);
	vkEnumerateInstanceLayerProperties(&layerCount, availableLayers.data());

	for (const char* layerName : validationLayers) {
		bool layerFound = false;

		for (const auto& layerProperties : availableLayers) {
			if (strcmp(layerName, layerProperties.layerName) == 0) {
				layerFound = true;
				break;
			}
		}

		if (!layerFound) {
			return false;
		}
	}

	return true;
}



void PrintVulkanSupportedExtensions()
{
	uint32_t extensionCount = 0;
	vkEnumerateInstanceExtensionProperties(nullptr, &extensionCount, nullptr);
	std::vector<VkExtensionProperties> extensions(extensionCount);
	vkEnumerateInstanceExtensionProperties(nullptr, &extensionCount, extensions.data());
	std::cout << "available extensions:" << std::endl;

	for (const auto& extension : extensions) {
		std::cout << "\t" << extension.extensionName << std::endl;
	}
}

void RendererVulkan::OnWindowResize(int i_NewW, int i_NewH)
{
	vkDeviceWaitIdle(m_LogicalDevice);

	createSwapChain(i_NewW, i_NewH);
	createImageViews();
	createRenderPass();
	createGraphicsPipeline();
	createDepthResources();
	createFramebuffers();
	createCommandBuffers();
	m_GUI.OnWindowResize();
}

void RendererVulkan::WaitToDestroy()
{
	vkDeviceWaitIdle(m_LogicalDevice);//Wait till the device is idle so we can destroy stuff
}


float RendererVulkan::GetMainRTAspectRatio()
{
	return m_SwapChainExtent.width / (float)m_SwapChainExtent.height;
}
float RendererVulkan::GetMainRTWidth()
{
	return m_SwapChainExtent.width;
}
float RendererVulkan::GetMainRTHeight() {
	return m_SwapChainExtent.height;
}


void RendererVulkan::SetupRenderCalls()
{
	recordDrawCommandBuffers();
	UploadUniforms();
}

void RendererVulkan::UploadUniforms()
{
	Scene* scene = ServiceLocator::GetSceneManager()->GetScene();
	CameraManager* pCamManager = ServiceLocator::GetCameraManager();
	if (!scene->IsInit())
		return;

	m_UpdateUniformsTime += m_LastFrameTime;

	if (m_UpdateUniformsTime < 1.0f/60.0f) {

		return;
	}
	m_UpdateUniformsTime = 0.0f;

	
	const CameraUniforms* camUniforms = pCamManager->GetCamUniforms();
	const InstanceUBO* instanceUniforms = scene->GetInstanceUniforms();

	const int iSceneUniformsIndex = 0;
	const int iInstanceUBOIndex = 0;
	
	VkDeviceSize uploadSize = m_StaticUniformBuffers[iSceneUniformsIndex].GetBufferSize();

	void* data;
	vkMapMemory(m_LogicalDevice, m_StaticUniformBuffers[iSceneUniformsIndex].GetStagingMemory(), 0, uploadSize, 0, &data);
	memcpy(data, camUniforms, uploadSize);
	vkUnmapMemory(m_LogicalDevice, m_StaticUniformBuffers[iSceneUniformsIndex].GetStagingMemory());

	copyVKBuffer(m_StaticUniformBuffers[iSceneUniformsIndex].GetStagingBuffer(), m_StaticUniformBuffers[iSceneUniformsIndex].GetBuffer(), uploadSize);
	
	
	
	
	uploadSize = m_DynamicUniformBuffers[iInstanceUBOIndex].GetBufferSize();

	
	vkMapMemory(m_LogicalDevice, m_DynamicUniformBuffers[iInstanceUBOIndex].GetStagingMemory(), 0, uploadSize, 0, &data);
	memcpy(data, instanceUniforms, uploadSize);
	vkUnmapMemory(m_LogicalDevice, m_DynamicUniformBuffers[iInstanceUBOIndex].GetStagingMemory());

	copyVKBuffer(m_DynamicUniformBuffers[iInstanceUBOIndex].GetStagingBuffer(), m_DynamicUniformBuffers[iInstanceUBOIndex].GetBuffer(), uploadSize);
	

}

void RendererVulkan::DrawFrame()
{
	

	m_GUI.DoUI();

	ServiceLocator::GetCameraManager()->BindCameraUniforms(CameraManager::eCameraType_Main);


	uint32_t imageIndex;
	//The numeric limits integer is the timeout in nanoseconds, we are actually disabling it
	vkAcquireNextImageKHR(m_LogicalDevice, m_SwapChain, std::numeric_limits<uint64_t>::max(), m_ImageAvailableSemaphore, VK_NULL_HANDLE, &imageIndex);
	
	std::vector<VkSemaphore> signalSemaphores;//This are the semaphores we notify when finishing executing the cmd buffers
	signalSemaphores.push_back(m_RenderFinishedSemaphore);

	std::vector<VkSemaphore> waitSemaphores;
	std::vector<VkPipelineStageFlags> waitSemaphoresStages;
	waitSemaphores.push_back(m_ImageAvailableSemaphore);
	waitSemaphoresStages.push_back(VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT);//This means the point of the pipeline we will wait for the image to be available, so we can execute ALL pipeline till here
	
	//TODO: Make this command buffer scene command buffer or static geometry command buffer...
	if (m_CommandBuffers.size() > 0)
	{
		UploadUniforms();//TODO: DO THIS ONLY IF UNIFORMS HAVE BEEN UPDATED

		VkSubmitInfo submitInfo = {};
		submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
		submitInfo.commandBufferCount = 1;
		submitInfo.pCommandBuffers = &m_CommandBuffers[imageIndex];

		if (vkQueueSubmit(m_graphicsQueue, 1, &submitInfo, VK_NULL_HANDLE) != VK_SUCCESS) {
			throw std::runtime_error("failed to submit draw command buffer!");
		}
	}
	
	

	m_GUI.Draw(m_graphicsQueue, imageIndex,signalSemaphores,waitSemaphores, waitSemaphoresStages);

	//And finally, presentation
	VkPresentInfoKHR presentInfo = {};
	presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;

	presentInfo.waitSemaphoreCount = signalSemaphores.size();
	presentInfo.pWaitSemaphores = signalSemaphores.data();//Wait for cmd pool semaphore

	VkSwapchainKHR swapChains[] = { m_SwapChain };
	presentInfo.swapchainCount = 1;
	presentInfo.pSwapchains = swapChains;
	presentInfo.pImageIndices = &imageIndex;
	presentInfo.pResults = nullptr; // Optional, return info could be sent here

	vkQueuePresentKHR(m_PresentQueue, &presentInfo);



	

}

int RendererVulkan::Init(const char** i_requiredExtensions, const unsigned int i_nExtensions, GLFWwindow* i_window)
{
	
	PrintVulkanSupportedExtensions();
	VkResult result = createInstance(i_requiredExtensions, i_nExtensions);
	
#ifndef NDEBUG
		setupDebugCallback();
#endif


	createSurface(i_window);
	pickPhysicalDevice();
	createLogicalDevice();

	int width, height;
	glfwGetWindowSize(i_window, &width, &height);
	createSwapChain(width, height);
	
	createImageViews();
	createRenderPass();
	createDescriptorSetLayout();
	createGraphicsPipeline();
	createCommandPools();
	createDepthResources();
	createFramebuffers();
	createSemaphores();
	createDescriptorPool();
	createCommandBuffers();
	m_TexturesPool.resize(s_TexturePoolSize);
	m_iUsedTextures = 0;

	m_GUI.Init(i_window);


	return (result == VK_SUCCESS);

}
void RendererVulkan::Destroy()	
{

}

VkResult RendererVulkan::createInstance(const char** i_requiredExtensions, const unsigned int i_nExtensions)
{

	std::vector<const char*> extensions;
	for (unsigned int i = 0; i < i_nExtensions; i++) 
	{
		extensions.push_back(i_requiredExtensions[i]);
	}

	if (m_bEnableValidationLayers)
	{
		if(!checkValidationLayerSupport(m_VvalidationLayers))
			throw std::runtime_error("validation layers requested, but not available!");

		extensions.push_back(VK_EXT_DEBUG_REPORT_EXTENSION_NAME);
	}

	m_applicationInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
	m_applicationInfo.pApplicationName = "Hello Triangle";
	m_applicationInfo.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
	m_applicationInfo.pEngineName = "No Engine";
	m_applicationInfo.engineVersion = VK_MAKE_VERSION(1, 0, 0);
	m_applicationInfo.apiVersion = VK_API_VERSION_1_0;


	

	m_instanceInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
	m_instanceInfo.pApplicationInfo = &m_applicationInfo;
	m_instanceInfo.enabledExtensionCount = extensions.size();
	m_instanceInfo.ppEnabledExtensionNames = extensions.data();
	m_instanceInfo.flags = 0;
	
	if (m_bEnableValidationLayers)
	{
		m_instanceInfo.enabledLayerCount = m_VvalidationLayers.size();
		m_instanceInfo.ppEnabledLayerNames = m_VvalidationLayers.data();
	}
	else
	{
		m_instanceInfo.enabledLayerCount = 0;
	}
	
	
	m_instanceInfo.pNext = nullptr;


	
	VkResult result = vkCreateInstance(&m_instanceInfo, nullptr, m_Instance.replace());

	if (result != VK_SUCCESS) {
		throw std::runtime_error("failed to create instance!");
	}
	
	return result;
}


void RendererVulkan::createSurface( GLFWwindow* i_window)
{
	VkWin32SurfaceCreateInfoKHR createInfo = {};
	createInfo.sType = VK_STRUCTURE_TYPE_WIN32_SURFACE_CREATE_INFO_KHR;
	createInfo.hwnd = glfwGetWin32Window(i_window);
	createInfo.hinstance = GetModuleHandle(nullptr);

	auto CreateWin32SurfaceKHR = (PFN_vkCreateWin32SurfaceKHR)vkGetInstanceProcAddr(m_Instance, "vkCreateWin32SurfaceKHR");

	if (!CreateWin32SurfaceKHR || CreateWin32SurfaceKHR(m_Instance, &createInfo,
		nullptr, m_Surface.replace()) != VK_SUCCESS) {
		throw std::runtime_error("failed to create window surface!");
	}
	
}


static void printPhysicalDeviceInfo(VkPhysicalDevice device)
{
	VkPhysicalDeviceProperties deviceProperties;
	vkGetPhysicalDeviceProperties(device, &deviceProperties);

	std::cout << "\n*Device Name = " << deviceProperties.deviceName<<  std::endl;
}




RendererVulkan::QueueFamilyIndices  RendererVulkan::findQueueFamilies(VkPhysicalDevice device) {
	QueueFamilyIndices indices;

	uint32_t queueFamilyCount = 0;
	vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, nullptr);

	std::vector<VkQueueFamilyProperties> queueFamilies(queueFamilyCount);
	vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, queueFamilies.data());



	int i = 0;
	for (const auto& queueFamily : queueFamilies) {
		
		//Graphics support
		if (queueFamily.queueCount > 0 && queueFamily.queueFlags & VK_QUEUE_GRAPHICS_BIT) {
			indices.graphicsFamily = i;
		}
		
		
		//Present suppoort
		VkBool32 presentSupport = false;
		vkGetPhysicalDeviceSurfaceSupportKHR(device, i, m_Surface, &presentSupport);

		if (queueFamily.queueCount > 0 && presentSupport) {
			indices.presentFamily = i;
		}


		if (indices.isComplete()) {
			break;
		}

		i++;
	}


	return indices;
}


RendererVulkan::SwapChainSupportDetails RendererVulkan::querySwapChainSupport(VkPhysicalDevice device) {
	SwapChainSupportDetails details;

	//Capabilities
	vkGetPhysicalDeviceSurfaceCapabilitiesKHR(device, m_Surface, &details.capabilities);

	//Formats
	uint32_t formatCount;
	vkGetPhysicalDeviceSurfaceFormatsKHR(device, m_Surface, &formatCount, nullptr);

	if (formatCount != 0) {
		details.formats.resize(formatCount);
		vkGetPhysicalDeviceSurfaceFormatsKHR(device, m_Surface, &formatCount, details.formats.data());
	}

	//Present modes
	uint32_t presentModeCount;
	vkGetPhysicalDeviceSurfacePresentModesKHR(device, m_Surface, &presentModeCount, nullptr);

	if (presentModeCount != 0) {
		details.presentModes.resize(presentModeCount);
		vkGetPhysicalDeviceSurfacePresentModesKHR(device, m_Surface, &presentModeCount, details.presentModes.data());
	}


	return details;
}


bool  RendererVulkan::checkDeviceExtensionSupport(VkPhysicalDevice device)
{
	uint32_t extensionCount;
	vkEnumerateDeviceExtensionProperties(device, nullptr, &extensionCount, nullptr);

	std::vector<VkExtensionProperties> availableExtensions(extensionCount);
	vkEnumerateDeviceExtensionProperties(device, nullptr, &extensionCount, availableExtensions.data());

	std::set<std::string> requiredExtensions(deviceExtensions.begin(), deviceExtensions.end());

	for (const auto& extension : availableExtensions) {
		requiredExtensions.erase(extension.extensionName);
	}

	return requiredExtensions.empty();

}

 bool  RendererVulkan::isDeviceSuitable(VkPhysicalDevice device) {

	RendererVulkan::QueueFamilyIndices indices = findQueueFamilies(device);

	bool extensionsSupported = checkDeviceExtensionSupport(device);


	bool swapChainAdequate = false;
	if (extensionsSupported) {
		SwapChainSupportDetails swapChainSupport = querySwapChainSupport(device);
		swapChainAdequate = !swapChainSupport.formats.empty() && !swapChainSupport.presentModes.empty();
	}



	return indices.isComplete() && extensionsSupported && swapChainAdequate;
}


void  RendererVulkan::pickPhysicalDevice()
{
	uint32_t deviceCount = 0;
	vkEnumeratePhysicalDevices(m_Instance, &deviceCount, nullptr);


	if (deviceCount == 0) 
	{
		throw std::runtime_error("failed to find GPUs with Vulkan support!");
	}


	std::vector<VkPhysicalDevice> devices(deviceCount);
	vkEnumeratePhysicalDevices(m_Instance, &deviceCount, devices.data());


	for (const auto& device : devices) {
		if (isDeviceSuitable(device)) {
			m_PhysicalDevice = device;
			std::cout << "\nUsing phys device " << std::endl;
			printPhysicalDeviceInfo(m_PhysicalDevice);
			break;
		}
	}

	if (m_PhysicalDevice == VK_NULL_HANDLE) {
		throw std::runtime_error("failed to find a suitable GPU!");
	}

}

void  RendererVulkan::createLogicalDevice()
{
	QueueFamilyIndices indices = findQueueFamilies(m_PhysicalDevice);

	std::vector<VkDeviceQueueCreateInfo> queueCreateInfos;
	std::set<int> uniqueQueueFamilies = { indices.graphicsFamily, indices.presentFamily };

	float queuePriority = 1.0f;
	for (int queueFamily : uniqueQueueFamilies) {
		VkDeviceQueueCreateInfo queueCreateInfo = {};
		queueCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
		queueCreateInfo.queueFamilyIndex = queueFamily;
		queueCreateInfo.queueCount = 1;
		queueCreateInfo.pQueuePriorities = &queuePriority;
		queueCreateInfos.push_back(queueCreateInfo);
	}

	VkPhysicalDeviceFeatures deviceFeatures = {};

	VkDeviceCreateInfo createInfo = {};
	createInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
	createInfo.pQueueCreateInfos = queueCreateInfos.data();
	createInfo.queueCreateInfoCount = (uint32_t)queueCreateInfos.size();
	createInfo.pEnabledFeatures = &deviceFeatures;

	createInfo.enabledExtensionCount = deviceExtensions.size();
	createInfo.ppEnabledExtensionNames = deviceExtensions.data();

	if (m_bEnableValidationLayers) {
		createInfo.enabledLayerCount = m_VvalidationLayers.size();
		createInfo.ppEnabledLayerNames = m_VvalidationLayers.data();
	}
	else {
		createInfo.enabledLayerCount = 0;
	}


	if (vkCreateDevice(m_PhysicalDevice, &createInfo, nullptr, m_LogicalDevice.replace()) != VK_SUCCESS) {
		throw std::runtime_error("failed to create logical device!");
	}

	vkGetDeviceQueue(m_LogicalDevice, indices.graphicsFamily, 0, &m_graphicsQueue);
	vkGetDeviceQueue(m_LogicalDevice, indices.presentFamily, 0, &m_PresentQueue);
	
}


void RendererVulkan::createSwapChain(int width, int height)
{


	SwapChainSupportDetails swapChainSupport = querySwapChainSupport(m_PhysicalDevice);

	VkSurfaceFormatKHR surfaceFormat = chooseSwapSurfaceFormat(swapChainSupport.formats);
	VkPresentModeKHR presentMode = chooseSwapPresentMode(swapChainSupport.presentModes);
	VkExtent2D extent = chooseSwapExtent(swapChainSupport.capabilities, width, height);

	//Number of images in the swapchain, we will try to have +1 the minimum for triple buffering

	uint32_t imageCount = swapChainSupport.capabilities.minImageCount + 1;
	//We clamp if our imagecount is above capabilities
	if (swapChainSupport.capabilities.maxImageCount > 0 && imageCount > swapChainSupport.capabilities.maxImageCount) {
		imageCount = swapChainSupport.capabilities.maxImageCount;
	}
	VkSwapchainCreateInfoKHR createInfo = {};
	createInfo.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
	createInfo.surface = m_Surface;
	createInfo.minImageCount = imageCount;
	createInfo.imageFormat = surfaceFormat.format;
	createInfo.imageColorSpace = surfaceFormat.colorSpace;
	createInfo.imageExtent = extent;
	createInfo.imageArrayLayers = 1;//One unless doing stereoscopic stuff 
	createInfo.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;//Final render target, might change if doing post processing



	QueueFamilyIndices indices = findQueueFamilies(m_PhysicalDevice);
	uint32_t queueFamilyIndices[] = { (uint32_t)indices.graphicsFamily, (uint32_t)indices.presentFamily };

	if (indices.graphicsFamily != indices.presentFamily) { //If they are different we use concurrent. Less performance but more simple to handle
		createInfo.imageSharingMode = VK_SHARING_MODE_CONCURRENT;
		createInfo.queueFamilyIndexCount = 2;
		createInfo.pQueueFamilyIndices = queueFamilyIndices;
	}
	else {
		createInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
		createInfo.queueFamilyIndexCount = 0; // Optional
		createInfo.pQueueFamilyIndices = nullptr; // Optional
	}

	createInfo.preTransform = swapChainSupport.capabilities.currentTransform;//Pixel transformation for the whole render target. Current transform means none 
	createInfo.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;//No alpha blending
	createInfo.presentMode = presentMode;
	createInfo.clipped = VK_TRUE;


	VkSwapchainKHR oldSwapChain = m_SwapChain;
	createInfo.oldSwapchain = oldSwapChain;

	VkSwapchainKHR newSwapChain;//We need to use this to prevent the deleter wrapper deleting the old one 
	if (vkCreateSwapchainKHR(m_LogicalDevice, &createInfo, nullptr, &newSwapChain) != VK_SUCCESS) {
		throw std::runtime_error("failed to create swap chain!");
	}

	m_SwapChain = newSwapChain;
	

	//Retrieving images from swapchain for use during rendering
	vkGetSwapchainImagesKHR(m_LogicalDevice, m_SwapChain, &imageCount, nullptr);
	m_SwapChainImages.resize(imageCount);
	vkGetSwapchainImagesKHR(m_LogicalDevice, m_SwapChain, &imageCount, m_SwapChainImages.data());

	m_SwapChainImageFormat = surfaceFormat.format;
	m_SwapChainExtent = extent;
}


void RendererVulkan::createImageViews()
{
	m_SwapChainImageViews.resize(m_SwapChainImages.size(), VKHandleWrapper <VkImageView>{m_LogicalDevice,vkDestroyImageView});

	for (uint32_t i = 0; i < m_SwapChainImages.size(); i++) {
		createImageView(m_SwapChainImages[i], m_SwapChainImageFormat, VK_IMAGE_ASPECT_COLOR_BIT, m_SwapChainImageViews[i]);
	}

}

void RendererVulkan::createRenderPass() {

	VkSubpassDependency dependency = {};
	dependency.srcSubpass = VK_SUBPASS_EXTERNAL;//This is the implicit vulkan subpass
	dependency.dstSubpass = 0;//Our subpass, so this is transition between first implicit and ours
	dependency.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
	dependency.srcAccessMask = 0;
	dependency.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
	dependency.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;


	VkAttachmentDescription colorAttachment = {};
	colorAttachment.format = m_SwapChainImageFormat;
	colorAttachment.samples = VK_SAMPLE_COUNT_1_BIT;

	colorAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;//Clear render target beforehand
	colorAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;//Store contents in memory, we need that for visualizing

	//We don't care what happens with the stencil here
	colorAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
	colorAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;

	colorAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED; //We don't care about the content the previous frame
	colorAttachment.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;//Here we declare the intention this colorAttachment will be used for

	VkAttachmentReference colorAttachmentRef = {};
	colorAttachmentRef.attachment = 0;
	colorAttachmentRef.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;


	m_SwapChainDepthFormat = VK_FORMAT_D32_SFLOAT;
	VkAttachmentDescription depthAttachment = {};
	depthAttachment.format = m_SwapChainDepthFormat;
	depthAttachment.samples = VK_SAMPLE_COUNT_1_BIT;
	depthAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
	depthAttachment.storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
	depthAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
	depthAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
	depthAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
	depthAttachment.finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

	VkAttachmentReference depthAttachmentRef = {};
	depthAttachmentRef.attachment = 1;
	depthAttachmentRef.layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;


	VkSubpassDescription subpass = {};
	subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;//Could be a compute pass..
	subpass.colorAttachmentCount = 1;
	subpass.pColorAttachments = &colorAttachmentRef;
	subpass.pDepthStencilAttachment = &depthAttachmentRef;	


	std::array<VkAttachmentDescription, 2> attachments = { colorAttachment, depthAttachment };
	VkRenderPassCreateInfo renderPassInfo = {};
	renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
	renderPassInfo.attachmentCount = attachments.size();
	renderPassInfo.pAttachments = attachments.data();
	renderPassInfo.subpassCount = 1;
	renderPassInfo.pSubpasses = &subpass;
	renderPassInfo.dependencyCount = 1;
	renderPassInfo.pDependencies = &dependency;


	if (vkCreateRenderPass(m_LogicalDevice, &renderPassInfo, nullptr, m_RenderPass.replace()) != VK_SUCCESS) {
		throw std::runtime_error("failed to create render pass!");
	}

}
void RendererVulkan::createDescriptorSetLayout()
{
	VkDescriptorSetLayoutBinding uboLayoutBinding = {};
	uboLayoutBinding.binding = 0;//Has to match binding number in the shader
	uboLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
	uboLayoutBinding.descriptorCount = 1;
	uboLayoutBinding.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;//In which shader is gonna be available
	uboLayoutBinding.pImmutableSamplers = nullptr;


	VkDescriptorSetLayoutBinding uboDynamicLayoutBinding = {};
	uboDynamicLayoutBinding.binding = 1;//Has to match binding number in the shader
	uboDynamicLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC;
	uboDynamicLayoutBinding.descriptorCount = 1;
	uboDynamicLayoutBinding.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;//In which shader is gonna be available
	uboDynamicLayoutBinding.pImmutableSamplers = nullptr;


	VkDescriptorSetLayoutBinding samplerLayoutBinding = {};
	samplerLayoutBinding.binding = 2;
	samplerLayoutBinding.descriptorCount = 1;
	samplerLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
	samplerLayoutBinding.pImmutableSamplers = nullptr;
	samplerLayoutBinding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;


	std::array<VkDescriptorSetLayoutBinding, 3> bindings = { uboLayoutBinding,uboDynamicLayoutBinding, samplerLayoutBinding };
	VkDescriptorSetLayoutCreateInfo layoutInfo = {};
	layoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
	layoutInfo.bindingCount = bindings.size();
	layoutInfo.pBindings = bindings.data();

	if (vkCreateDescriptorSetLayout(m_LogicalDevice, &layoutInfo, nullptr, m_DescriptorSetLayout.replace()) != VK_SUCCESS) {
		throw std::runtime_error("failed to create descriptor set layout!");
	}


}

void RendererVulkan::createGraphicsPipeline()
{

	VkShaderModule vertShaderModule;
	VkShaderModule fragShaderModule;

	createShaderModule("Shaders/vert.spv", vertShaderModule);
	createShaderModule("Shaders/frag.spv", fragShaderModule);


	VkPipelineShaderStageCreateInfo vertShaderStageInfo = {};
	vertShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
	vertShaderStageInfo.stage = VK_SHADER_STAGE_VERTEX_BIT;
	vertShaderStageInfo.module = vertShaderModule;
	vertShaderStageInfo.pName = "main";

	VkPipelineShaderStageCreateInfo fragShaderStageInfo = {};
	fragShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
	fragShaderStageInfo.stage = VK_SHADER_STAGE_FRAGMENT_BIT;
	fragShaderStageInfo.module = fragShaderModule;
	fragShaderStageInfo.pName = "main";


	VkPipelineShaderStageCreateInfo shaderStages[] = { vertShaderStageInfo, fragShaderStageInfo };


	//Fixed functionality setup

	//Vertex input: FROM WHERE VERTICES COME
	



	VkVertexInputBindingDescription bindingDescription = {};
	std::vector<VkVertexInputAttributeDescription> attributeDescriptions = {};
	
	

	//This is specific for this vertex format, has to be improved, one pipeline for each vertexFormat?
	Vertex::GetVertexDescription(&bindingDescription);
	Vertex::GetAttributesDescription(attributeDescriptions);

	VkPipelineVertexInputStateCreateInfo vertexInputInfo = {};
	vertexInputInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;

	vertexInputInfo.vertexBindingDescriptionCount = 1;
	vertexInputInfo.vertexAttributeDescriptionCount = attributeDescriptions.size();
	vertexInputInfo.pVertexBindingDescriptions = &bindingDescription;
	vertexInputInfo.pVertexAttributeDescriptions = attributeDescriptions.data();

	//Input assembly: HOW WE PRESENT VERTICES
	VkPipelineInputAssemblyStateCreateInfo inputAssembly = {};
	inputAssembly.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
	inputAssembly.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
	inputAssembly.primitiveRestartEnable = VK_FALSE;

	//VIEWPORT
	VkViewport viewport = {};
	viewport.x = 0.0f;
	viewport.y = 0.0f;
	viewport.width = (float)m_SwapChainExtent.width;
	viewport.height = (float)m_SwapChainExtent.height;
	viewport.minDepth = 0.0f;
	viewport.maxDepth = 1.0f;

	VkRect2D scissor = {};
	scissor.offset = { 0, 0 };
	scissor.extent = m_SwapChainExtent;

	VkPipelineViewportStateCreateInfo viewportState = {};
	viewportState.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
	viewportState.viewportCount = 1;
	viewportState.pViewports = &viewport;
	viewportState.scissorCount = 1;
	viewportState.pScissors = &scissor;


	//RASTERIZER

	VkPipelineRasterizationStateCreateInfo rasterizer = {};
	rasterizer.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
	rasterizer.depthClampEnable = VK_FALSE;

	rasterizer.rasterizerDiscardEnable = VK_FALSE;

	rasterizer.polygonMode = VK_POLYGON_MODE_FILL;

	rasterizer.lineWidth = 1.0f;

	rasterizer.cullMode = VK_CULL_MODE_NONE;
	rasterizer.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;
	
	rasterizer.depthBiasEnable = VK_FALSE;
	rasterizer.depthBiasConstantFactor = 0.0f; // Optional
	rasterizer.depthBiasClamp = 0.0f; // Optional
	rasterizer.depthBiasSlopeFactor = 0.0f; // Optional


	//MULTISAMPLING (we don't use it for now)

	VkPipelineMultisampleStateCreateInfo multisampling = {};
	multisampling.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
	multisampling.sampleShadingEnable = VK_FALSE;
	multisampling.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;
	multisampling.minSampleShading = 1.0f; // Optional
	multisampling.pSampleMask = nullptr; /// Optional
	multisampling.alphaToCoverageEnable = VK_FALSE; // Optional
	multisampling.alphaToOneEnable = VK_FALSE; // Optional


	//COLOR BLENDING

	VkPipelineColorBlendAttachmentState colorBlendAttachment = {};
	colorBlendAttachment.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
	colorBlendAttachment.blendEnable = VK_FALSE;
	colorBlendAttachment.srcColorBlendFactor = VK_BLEND_FACTOR_ONE; // Optional
	colorBlendAttachment.dstColorBlendFactor = VK_BLEND_FACTOR_ZERO; // Optional
	colorBlendAttachment.colorBlendOp = VK_BLEND_OP_ADD; // Optional
	colorBlendAttachment.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE; // Optional
	colorBlendAttachment.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO; // Optional
	colorBlendAttachment.alphaBlendOp = VK_BLEND_OP_ADD; // Optional

	VkPipelineColorBlendStateCreateInfo colorBlending = {};
	colorBlending.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
	colorBlending.logicOpEnable = VK_FALSE;
	colorBlending.logicOp = VK_LOGIC_OP_COPY; // Optional
	colorBlending.attachmentCount = 1;
	colorBlending.pAttachments = &colorBlendAttachment;
	colorBlending.blendConstants[0] = 0.0f; // Optional
	colorBlending.blendConstants[1] = 0.0f; // Optional
	colorBlending.blendConstants[2] = 0.0f; // Optional
	colorBlending.blendConstants[3] = 0.0f; // Optional

	//DYNAMIC STATE: Stuff you can change without recreating pipeline

	VkDynamicState dynamicStates[] = {
		VK_DYNAMIC_STATE_VIEWPORT,
		VK_DYNAMIC_STATE_LINE_WIDTH
	};

	VkPipelineDynamicStateCreateInfo dynamicState = {};
	dynamicState.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
	dynamicState.dynamicStateCount = 2;
	dynamicState.pDynamicStates = dynamicStates;


	//PIPELINE LAYOUT CREATION
	VkDescriptorSetLayout setLayouts[] = { m_DescriptorSetLayout };
	VkPipelineLayoutCreateInfo pipelineLayoutInfo = {};
	pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
	pipelineLayoutInfo.setLayoutCount = 1; 
	pipelineLayoutInfo.pSetLayouts = setLayouts; // Optional
	pipelineLayoutInfo.pushConstantRangeCount = 0; // Optional
	pipelineLayoutInfo.pPushConstantRanges = 0; // Optional

	if (vkCreatePipelineLayout(m_LogicalDevice, &pipelineLayoutInfo, nullptr,
		m_PipelineLayout.replace()) != VK_SUCCESS) {
		throw std::runtime_error("failed to create pipeline layout!");
	}

	//Depth attachment
	VkPipelineDepthStencilStateCreateInfo depthStencil = {};
	depthStencil.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
	depthStencil.depthTestEnable = VK_TRUE;
	depthStencil.depthWriteEnable = VK_TRUE;
	depthStencil.depthCompareOp = VK_COMPARE_OP_LESS;
	depthStencil.depthBoundsTestEnable = VK_FALSE;
	depthStencil.minDepthBounds = 0.0f; // Optional
	depthStencil.maxDepthBounds = 1.0f; // Optional
	depthStencil.stencilTestEnable = VK_FALSE;
	depthStencil.front = {}; // Optional
	depthStencil.back = {}; // Optional



	//All comes together here to shape our pipeline
	
	VkGraphicsPipelineCreateInfo pipelineInfo = {};
	pipelineInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
	pipelineInfo.stageCount = 2;
	pipelineInfo.pStages = shaderStages;

	pipelineInfo.pVertexInputState = &vertexInputInfo;
	pipelineInfo.pInputAssemblyState = &inputAssembly;
	pipelineInfo.pViewportState = &viewportState;
	pipelineInfo.pRasterizationState = &rasterizer;
	pipelineInfo.pMultisampleState = &multisampling;
	pipelineInfo.pDepthStencilState = &depthStencil; // Optional
	pipelineInfo.pColorBlendState = &colorBlending;
	pipelineInfo.pDynamicState = nullptr; // Optional


	pipelineInfo.layout = m_PipelineLayout;

	pipelineInfo.renderPass = m_RenderPass;
	pipelineInfo.subpass = 0;

	//Pipeline derivatives. This is supposed to be cheaper to swap than completelly unrelated ones
	pipelineInfo.basePipelineHandle = VK_NULL_HANDLE; // Optional
	pipelineInfo.basePipelineIndex = -1; // Optional

	//This call could create more than one pipeline object
	if (vkCreateGraphicsPipelines(m_LogicalDevice, VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, m_GraphicsPipeline.replace()) != VK_SUCCESS) {
		throw std::runtime_error("failed to create graphics pipeline!");
	}

	vkDestroyShaderModule(m_LogicalDevice, vertShaderModule, nullptr);
	vkDestroyShaderModule(m_LogicalDevice, fragShaderModule, nullptr);

}


void  RendererVulkan::createFramebuffers() {
	m_SwapChainFramebuffers.resize(m_SwapChainImageViews.size(), VKHandleWrapper <VkFramebuffer>{m_LogicalDevice, vkDestroyFramebuffer});
	

	for (size_t i = 0; i < m_SwapChainImageViews.size(); i++) {
		std::array<VkImageView, 2> attachments = {
			m_SwapChainImageViews[i],
			m_DepthImage.GetVKImageView()
		};

		VkFramebufferCreateInfo framebufferInfo = {};
		framebufferInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
		framebufferInfo.renderPass = m_RenderPass;
		framebufferInfo.attachmentCount = attachments.size();
		framebufferInfo.pAttachments = attachments.data();
		framebufferInfo.width = m_SwapChainExtent.width;
		framebufferInfo.height = m_SwapChainExtent.height;
		framebufferInfo.layers = 1;

		if (vkCreateFramebuffer(m_LogicalDevice, &framebufferInfo, nullptr, m_SwapChainFramebuffers[i].replace()) != VK_SUCCESS) {
			throw std::runtime_error("failed to create framebuffer!");
		}
	}
}

void RendererVulkan::createCommandPools()
{
	QueueFamilyIndices queueFamilyIndices = findQueueFamilies(m_PhysicalDevice);

	VkCommandPoolCreateInfo poolInfo = {};
	poolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
	poolInfo.queueFamilyIndex = queueFamilyIndices.graphicsFamily;
	poolInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT; // Optional, here we could say how often we are gonna re-record the commands

	if (vkCreateCommandPool(m_LogicalDevice, &poolInfo, nullptr, m_CommandPool.replace()) != VK_SUCCESS) {
		throw std::runtime_error("failed to create command pool!");
	}

	//Special Command pool for short lived command buffers like the ones for mem transfer. They die once their job is done
	VkCommandPoolCreateInfo poolInfoMemTransfers = {};
	poolInfoMemTransfers.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
	poolInfoMemTransfers.queueFamilyIndex = queueFamilyIndices.graphicsFamily;
	poolInfoMemTransfers.flags = VK_COMMAND_POOL_CREATE_TRANSIENT_BIT ;

	if (vkCreateCommandPool(m_LogicalDevice, &poolInfoMemTransfers, nullptr, m_MemTransferCommandPool.replace()) != VK_SUCCESS) {
		throw std::runtime_error("failed to create command pool!");
	}


}

void RendererVulkan::createDepthResources()
{
	m_DepthImage.Init(m_LogicalDevice);
	VkFormat depthFormat = VK_FORMAT_D32_SFLOAT;
	createVulkanImage(m_SwapChainExtent.width, m_SwapChainExtent.height, depthFormat, VK_IMAGE_TILING_OPTIMAL, VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, m_DepthImage.GetVKImage(), m_DepthImage.GetVKImageMemory());
	createImageView(m_DepthImage.GetVKImage(), depthFormat, VK_IMAGE_ASPECT_DEPTH_BIT, m_DepthImage.GetVKImageView());

	transitionImageLayout(m_DepthImage.GetVKImage(), depthFormat, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL);
}


void  RendererVulkan::createVulkanImage(uint32_t width, uint32_t height, VkFormat format, VkImageTiling tiling, VkImageUsageFlags usage, VkMemoryPropertyFlags properties, VKHandleWrapper<VkImage>& image, VKHandleWrapper<VkDeviceMemory>& imageMemory) {
	
	VkImageCreateInfo imageInfo = {};
	imageInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
	imageInfo.imageType = VK_IMAGE_TYPE_2D;
	imageInfo.extent.width = width;
	imageInfo.extent.height = height;
	imageInfo.extent.depth = 1;
	imageInfo.mipLevels = 1;
	imageInfo.arrayLayers = 1;
	imageInfo.format = format;
	imageInfo.tiling = tiling;
	imageInfo.initialLayout = VK_IMAGE_LAYOUT_PREINITIALIZED;
	imageInfo.usage = usage;
	imageInfo.samples = VK_SAMPLE_COUNT_1_BIT;
	imageInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

	if (vkCreateImage(m_LogicalDevice, &imageInfo, nullptr, image.replace()) != VK_SUCCESS) {
		throw std::runtime_error("failed to create image!");
	}

	VkMemoryRequirements memRequirements;
	vkGetImageMemoryRequirements(m_LogicalDevice, image, &memRequirements);

	VkMemoryAllocateInfo allocInfo = {};
	allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
	allocInfo.allocationSize = memRequirements.size;
	allocInfo.memoryTypeIndex = findMemoryType(memRequirements.memoryTypeBits, properties);

	if (vkAllocateMemory(m_LogicalDevice, &allocInfo, nullptr, imageMemory.replace()) != VK_SUCCESS) {
		throw std::runtime_error("failed to allocate image memory!");
	}

	vkBindImageMemory(m_LogicalDevice, image, imageMemory, 0);
}

void RendererVulkan::CreateMaterial(std::string i_MatName,  int* iTexIndices, int iNumTextures)
{

	VKMaterial vk_mat = {};
	VkDescriptorSet descSet = VK_NULL_HANDLE;
	CreateDescriptorSet(descSet,iTexIndices,iNumTextures);
	vk_mat.Init(m_GraphicsPipeline, descSet);

 	m_MaterialsMap[i_MatName] = vk_mat;
	
}
void RendererVulkan::DeleteMaterials()
{
	//Wait for device idle we don't wanna mess here
	vkDeviceWaitIdle(m_LogicalDevice);

	m_MaterialsMap.clear();

	//Reseting descriptor pool automatically delete all descriptor sets associated so no need to go one by one
	createDescriptorPool();


	//Reseting textures pool hopefully calls relevan destructors thru VKHandleWrapper :)
	m_TexturesPool.clear();
	m_TexturesPool.resize(s_TexturePoolSize);
	/*for (int i = 0; i < m_iUsedTextures; i++)
	{

	}*/
	m_iUsedTextures = 0;
}

int RendererVulkan::CreateTexture(void*  i_data, int i_Widht, int i_Height)
{
	
	int iTexIndex = m_iUsedTextures;
	VKTextureWrapper* tTexture = &m_TexturesPool[iTexIndex];
	

	

	tTexture->Init(m_LogicalDevice);

	createTextureImage(i_data, i_Widht, i_Height, tTexture->GetVKImage(), tTexture->GetVKImageMemory());
	createImageView(tTexture->GetVKImage(), VK_FORMAT_R8G8B8A8_UNORM, VK_IMAGE_ASPECT_COLOR_BIT, tTexture->GetVKImageView());
	createTextureSampler(tTexture->GetVKTextureSampler());

	tTexture->UpdateDescriptor();

	
	m_iUsedTextures++;

	return iTexIndex;
}


void RendererVulkan::createTextureSampler(VKHandleWrapper<VkSampler>& i_Sampler)
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

	if (vkCreateSampler(m_LogicalDevice, &samplerInfo, nullptr, i_Sampler.replace()) != VK_SUCCESS) {
		throw std::runtime_error("failed to create texture sampler!");
	}

}

void  RendererVulkan::CreateVertexBuffer(const void*  i_data, size_t iBufferSize)
{

	VkDeviceSize bufferSize = iBufferSize;


	VKHandleWrapper<VkBuffer> stagingBuffer{ m_LogicalDevice, vkDestroyBuffer };
	VKHandleWrapper<VkDeviceMemory> stagingBufferMemory{ m_LogicalDevice, vkFreeMemory };

	createVKBuffer(bufferSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, stagingBuffer, stagingBufferMemory);

	void* data;
	vkMapMemory(m_LogicalDevice, stagingBufferMemory, 0, bufferSize, 0, &data);//Get a pointer to the device memory
	memcpy(data, i_data, iBufferSize);//Copy our vertex data there
	vkUnmapMemory(m_LogicalDevice, stagingBufferMemory);//Unmap the memory

	createVKBuffer(bufferSize, VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, m_MainVertexBuffer, m_MainVertexBufferMemory);

	
	copyVKBuffer(stagingBuffer, m_MainVertexBuffer, bufferSize);
}

void RendererVulkan::CreateIndexBuffer(const void*  i_data, size_t iBufferSize)
{
	VkDeviceSize bufferSize = iBufferSize;

	VKHandleWrapper<VkBuffer> stagingBuffer{ m_LogicalDevice, vkDestroyBuffer };
	VKHandleWrapper<VkDeviceMemory> stagingBufferMemory{ m_LogicalDevice, vkFreeMemory };
	createVKBuffer(bufferSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, stagingBuffer, stagingBufferMemory);

	void* data;
	vkMapMemory(m_LogicalDevice, stagingBufferMemory, 0, bufferSize, 0, &data);//Get a pointer to the device memory
	memcpy(data, i_data, iBufferSize);//Copy our index data there
	vkUnmapMemory(m_LogicalDevice, stagingBufferMemory);//Unmap the memory

	createVKBuffer(bufferSize, VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, m_MainIndexBuffer, m_MainIndexBufferMemory);

	copyVKBuffer(stagingBuffer, m_MainIndexBuffer, bufferSize);

}


void RendererVulkan::DeleteVertexBuffer()
{
	//Wait for device idle we don't wanna mess here
	vkDeviceWaitIdle(m_LogicalDevice);
	//vkDestroyBuffer(m_LogicalDevice, m_MainVertexBuffer, nullptr);
	m_MainVertexBuffer = VK_NULL_HANDLE;
	//vkFreeMemory(m_LogicalDevice, m_MainVertexBufferMemory, nullptr);
	m_MainVertexBufferMemory = VK_NULL_HANDLE;
}
void RendererVulkan::DeleteIndexBuffer()
{
	//Wait for device idle we don't wanna mess here
	vkDeviceWaitIdle(m_LogicalDevice);
	//vkDestroyBuffer(m_LogicalDevice, m_MainIndexBuffer, nullptr);
	m_MainIndexBuffer = VK_NULL_HANDLE;
	//vkFreeMemory(m_LogicalDevice, m_MainIndexBufferMemory, nullptr);
	m_MainIndexBufferMemory = VK_NULL_HANDLE;
}

void RendererVulkan::CreateStaticUniformBuffer(const void*  i_data, size_t iBufferSize)
{

	int nUniformBuffers = m_StaticUniformBuffers.size();
	m_StaticUniformBuffers.resize(m_StaticUniformBuffers.size() + 1);
	m_StaticUniformBuffers[nUniformBuffers].Init(m_LogicalDevice, iBufferSize);

	createVKBuffer(iBufferSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, m_StaticUniformBuffers[nUniformBuffers].GetStagingBuffer(), m_StaticUniformBuffers[nUniformBuffers].GetStagingMemory());
	createVKBuffer(iBufferSize, VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, m_StaticUniformBuffers[nUniformBuffers].GetBuffer(), m_StaticUniformBuffers[nUniformBuffers].GetMemory());
	m_StaticUniformBuffers[nUniformBuffers].UpdateDescriptor();

}
void RendererVulkan::CreateInstancedUniformBuffer(const void*  i_data, size_t iBufferSize)
{
	


	int nUniformBuffers = m_DynamicUniformBuffers.size();
	m_DynamicUniformBuffers.resize(m_DynamicUniformBuffers.size() + 1);
	m_DynamicUniformBuffers[nUniformBuffers].Init(m_LogicalDevice, iBufferSize);

	createVKBuffer(iBufferSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, m_DynamicUniformBuffers[nUniformBuffers].GetStagingBuffer(), m_DynamicUniformBuffers[nUniformBuffers].GetStagingMemory());
	createVKBuffer(iBufferSize, VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, m_DynamicUniformBuffers[nUniformBuffers].GetBuffer(), m_DynamicUniformBuffers[nUniformBuffers].GetMemory());
	m_DynamicUniformBuffers[nUniformBuffers].UpdateDescriptor();

}

void RendererVulkan::DeleteStaticUniformBuffer()
{
	//Wait for device idle we don't wanna mess here
	vkDeviceWaitIdle(m_LogicalDevice);

	m_StaticUniformBuffers.clear();


}
void RendererVulkan::DeleteInstancedUniformBuffer()
{
	//Wait for device idle we don't wanna mess here
	vkDeviceWaitIdle(m_LogicalDevice);
	m_DynamicUniformBuffers.clear();

}



//TODO:: This pool should have at least count > materials/desc sets, remove magic number
void RendererVulkan::createDescriptorPool()
{
	std::array<VkDescriptorPoolSize, 3> poolSizes = {};
	poolSizes[0].type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
	poolSizes[0].descriptorCount = 36;
	poolSizes[1].type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC;
	poolSizes[1].descriptorCount = 36;
	poolSizes[2].type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
	poolSizes[2].descriptorCount = 36;

	VkDescriptorPoolCreateInfo poolInfo = {};
	poolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
	poolInfo.poolSizeCount = poolSizes.size();
	poolInfo.pPoolSizes = poolSizes.data();
	poolInfo.maxSets = 36 +1;

	if (vkCreateDescriptorPool(m_LogicalDevice, &poolInfo, nullptr, m_DescriptorPool.replace()) != VK_SUCCESS) {
		throw std::runtime_error("failed to create descriptor pool!");
	}

}

void RendererVulkan::CreateDescriptorSet(VkDescriptorSet& i_DescSet, int* iTexIndices, int iNumTextures)
{
	VkDescriptorSetLayout layouts[] = { m_DescriptorSetLayout };
	VkDescriptorSetAllocateInfo allocInfo = {};
	allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
	allocInfo.descriptorPool = m_DescriptorPool;
	allocInfo.descriptorSetCount = 1;
	allocInfo.pSetLayouts = layouts;

	if (vkAllocateDescriptorSets(m_LogicalDevice, &allocInfo, &i_DescSet) != VK_SUCCESS) {
		throw std::runtime_error("failed to allocate descriptor set!");
	}

	std::vector<VkDescriptorBufferInfo> staticBufferInfos;
	for (int i = 0; i < m_StaticUniformBuffers.size(); i++)
	{
		m_StaticUniformBuffers[i].UpdateDescriptor();
		staticBufferInfos.push_back(m_StaticUniformBuffers[i].GetDescriptor());
	}

	std::vector<VkDescriptorBufferInfo> dynamicBufferInfos;
	for (int i = 0; i < m_DynamicUniformBuffers.size(); i++)
	{
		m_DynamicUniformBuffers[i].UpdateDescriptor();
		dynamicBufferInfos.push_back(m_DynamicUniformBuffers[i].GetDescriptor());
	}

	
	std::vector<VkDescriptorImageInfo> imageInfos;
	for (int i = 0; i < iNumTextures; i++)
	{

		VKTextureWrapper* pTexture = &m_TexturesPool[iTexIndices[i]];
		pTexture->UpdateDescriptor();
		imageInfos.push_back(pTexture->GetDescriptor());
	}

	

	std::array<VkWriteDescriptorSet, 3> descriptorWrites = {};
	descriptorWrites[0].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
	descriptorWrites[0].dstSet = i_DescSet;
	descriptorWrites[0].dstBinding = 0;
	descriptorWrites[0].dstArrayElement = 0;
	descriptorWrites[0].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
	descriptorWrites[0].descriptorCount = staticBufferInfos.size();

	descriptorWrites[0].pBufferInfo = staticBufferInfos.data();
	

	descriptorWrites[1].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
	descriptorWrites[1].dstSet = i_DescSet;
	descriptorWrites[1].dstBinding = 1;
	descriptorWrites[1].dstArrayElement = 0;
	descriptorWrites[1].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC;
	descriptorWrites[1].descriptorCount = dynamicBufferInfos.size();

	descriptorWrites[1].pBufferInfo = dynamicBufferInfos.data();


	descriptorWrites[2].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
	descriptorWrites[2].dstSet = i_DescSet;
	descriptorWrites[2].dstBinding = 2;
	descriptorWrites[2].dstArrayElement = 0;
	descriptorWrites[2].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
	descriptorWrites[2].descriptorCount = imageInfos.size();
	descriptorWrites[2].pImageInfo = imageInfos.data();


	vkUpdateDescriptorSets(m_LogicalDevice, descriptorWrites.size(), descriptorWrites.data(), 0, nullptr);



}

void RendererVulkan::recordDrawCommandBuffers()
{
	if (m_CommandBuffers.size() == 0)
		return;//No command buffer allocated

	Scene* scene = ServiceLocator::GetSceneManager()->GetScene();

	for (size_t i = 0; i < m_CommandBuffers.size(); i++) {
		
		VkCommandBufferBeginInfo beginInfo = {};
		beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
		beginInfo.flags = VK_COMMAND_BUFFER_USAGE_SIMULTANEOUS_USE_BIT;//Means we can submit whilst still drawing previous frame
		beginInfo.pInheritanceInfo = nullptr; // Optional

		vkBeginCommandBuffer(m_CommandBuffers[i], &beginInfo);//Calling this will delete previous content meaning we cant append to previous content

		VkRenderPassBeginInfo renderPassInfo = {};
		renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
		renderPassInfo.renderPass = m_RenderPass;
		renderPassInfo.framebuffer = m_SwapChainFramebuffers[i];

		renderPassInfo.renderArea.offset = { 0, 0 };
		renderPassInfo.renderArea.extent = m_SwapChainExtent;

		std::array<VkClearValue, 2> clearValues = {};
		clearValues[0].color = { 0.0f, 0.0f, 0.0f, 1.0f };
		clearValues[1].depthStencil = { 1.0f, 0 };

		renderPassInfo.clearValueCount = clearValues.size();
		renderPassInfo.pClearValues = clearValues.data();

		vkCmdBeginRenderPass(m_CommandBuffers[i], &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);

		std::vector <Model> sceneModels = *(scene->GetModels());

		VkBuffer vertexBuffers[] = { m_MainVertexBuffer };
		VkDeviceSize offsets[] = { 0 };

		vkCmdBindVertexBuffers(m_CommandBuffers[i], 0, 1, vertexBuffers, offsets);
		vkCmdBindIndexBuffer(m_CommandBuffers[i], m_MainIndexBuffer, 0, VK_INDEX_TYPE_UINT32);
		vkCmdBindPipeline(m_CommandBuffers[i], VK_PIPELINE_BIND_POINT_GRAPHICS, m_GraphicsPipeline);//TODO: Maybe one pipeline per model?? For Now use the one we have
		for (int iModel = 0; iModel < sceneModels.size(); iModel++)
		{

			int nIndices = sceneModels[iModel].GetMesh()->GetNIndices();
			int indexStart = sceneModels[iModel].GetMesh()->GetIndexStartPosition();
			
			std::string m_sMaterialName = sceneModels[iModel].GetMaterial()->GetMaterialName();

			uint32_t dynamicOffset = iModel * static_cast<uint32_t>(256);

			vkCmdBindDescriptorSets(m_CommandBuffers[i], VK_PIPELINE_BIND_POINT_GRAPHICS, m_PipelineLayout, 0, 1, &m_MaterialsMap[m_sMaterialName].GetDescriptorSet(), 1, &dynamicOffset);//Here we bind textures
			vkCmdDrawIndexed(m_CommandBuffers[i], nIndices, 1, indexStart, sceneModels[iModel].GetMesh()->GetVertexStartPosition(), 0);
		}
		
		

		vkCmdEndRenderPass(m_CommandBuffers[i]);


		if (vkEndCommandBuffer(m_CommandBuffers[i]) != VK_SUCCESS) {
			throw std::runtime_error("failed to record command buffer!");
		}
	}

}
void RendererVulkan::createCommandBuffers()
{
	

	//In case we regenerated the swapchain we need to reset this...
	if (m_CommandBuffers.size() > 0) {
		vkFreeCommandBuffers(m_LogicalDevice, m_CommandPool, m_CommandBuffers.size(), m_CommandBuffers.data());
	}

	//Resize to match framebuffers, we will have one command buffer per framebuffer
	m_CommandBuffers.resize(m_SwapChainFramebuffers.size());

	VkCommandBufferAllocateInfo allocInfo = {};
	allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
	allocInfo.commandPool = m_CommandPool;
	allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
	allocInfo.commandBufferCount = (uint32_t)m_CommandBuffers.size();

	if (vkAllocateCommandBuffers(m_LogicalDevice, &allocInfo, m_CommandBuffers.data()) != VK_SUCCESS) {
		throw std::runtime_error("failed to allocate command buffers!");
	}

	//FILL WITH EMPTY DATA
	for (size_t i = 0; i < m_CommandBuffers.size(); i++) {

		VkCommandBufferBeginInfo beginInfo = {};
		beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
		beginInfo.flags = VK_COMMAND_BUFFER_USAGE_SIMULTANEOUS_USE_BIT;//Means we can submit whilst still drawing previous frame
		beginInfo.pInheritanceInfo = nullptr; // Optional

		vkBeginCommandBuffer(m_CommandBuffers[i], &beginInfo);//Calling this will delete previous content meaning we cant append to previous content

		VkRenderPassBeginInfo renderPassInfo = {};
		renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
		renderPassInfo.renderPass = m_RenderPass;
		renderPassInfo.framebuffer = m_SwapChainFramebuffers[i];

		renderPassInfo.renderArea.offset = { 0, 0 };
		renderPassInfo.renderArea.extent = m_SwapChainExtent;

		std::array<VkClearValue, 2> clearValues = {};
		clearValues[0].color = { 0.0f, 0.0f, 0.0f, 1.0f };
		clearValues[1].depthStencil = { 1.0f, 0 };

		renderPassInfo.clearValueCount = clearValues.size();
		renderPassInfo.pClearValues = clearValues.data();

		vkCmdBeginRenderPass(m_CommandBuffers[i], &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);

		vkCmdEndRenderPass(m_CommandBuffers[i]);


		if (vkEndCommandBuffer(m_CommandBuffers[i]) != VK_SUCCESS) {
			throw std::runtime_error("failed to record command buffer!");
		}
	}
	
	//
}

void RendererVulkan::createSemaphores()
{
	VkSemaphoreCreateInfo semaphoreInfo = {};
	semaphoreInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

	if (vkCreateSemaphore(m_LogicalDevice, &semaphoreInfo, nullptr, m_ImageAvailableSemaphore.replace()) != VK_SUCCESS ||
		vkCreateSemaphore(m_LogicalDevice, &semaphoreInfo, nullptr, m_RenderFinishedSemaphore.replace()) != VK_SUCCESS) {

		throw std::runtime_error("failed to create semaphores!");
	}

}


void RendererVulkan::createShaderModule(const std::string & filename, VkShaderModule & shaderModule)
{

	std::cout << "*Attempting to load" << filename.c_str() <<":" << std::endl;
	auto shaderCode = readFile(filename);

	if (shaderCode.size() != 0)
	{
		std::cout << "SUCCESS" << std::endl;
	}
	else
	{
		std::cout << "FAIL" << std::endl;
		return;
	}

	VkShaderModuleCreateInfo createInfo = {};
	createInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
	createInfo.codeSize = shaderCode.size();

	std::vector<uint32_t> codeAligned(shaderCode.size() / sizeof(uint32_t) + 1);
	memcpy(codeAligned.data(), shaderCode.data(), shaderCode.size());
	createInfo.pCode = codeAligned.data();

	if (vkCreateShaderModule(m_LogicalDevice, &createInfo, nullptr, &shaderModule) != VK_SUCCESS) {
		throw std::runtime_error("failed to create shader module!");
	}


}

void RendererVulkan::createVKBuffer(VkDeviceSize size, VkBufferUsageFlags usage, VkMemoryPropertyFlags properties, VKHandleWrapper<VkBuffer>& buffer, VKHandleWrapper<VkDeviceMemory>& bufferMemory)
{
	VkBufferCreateInfo bufferInfo = {};
	bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
	bufferInfo.size = size;
	bufferInfo.usage = usage;
	bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

	if (vkCreateBuffer(m_LogicalDevice , &bufferInfo, nullptr, buffer.replace()) != VK_SUCCESS) {
		throw std::runtime_error("failed to create buffer!");
	}

	VkMemoryRequirements memRequirements;
	vkGetBufferMemoryRequirements(m_LogicalDevice, buffer, &memRequirements);

	VkMemoryAllocateInfo allocInfo = {};
	allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
	allocInfo.allocationSize = memRequirements.size;
	allocInfo.memoryTypeIndex = findMemoryType(memRequirements.memoryTypeBits, properties);

	if (vkAllocateMemory(m_LogicalDevice, &allocInfo, nullptr, bufferMemory.replace()) != VK_SUCCESS) {
		throw std::runtime_error("failed to allocate buffer memory!");
	}

	vkBindBufferMemory(m_LogicalDevice, buffer, bufferMemory, 0);

}

void RendererVulkan::copyImage(VkImage srcImage, VkImage dstImage, uint32_t width, uint32_t height)
{
	VkCommandBuffer commandBuffer = beginSingleTimeCommands();

	VkImageSubresourceLayers subResource = {};
	subResource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
	subResource.baseArrayLayer = 0;
	subResource.mipLevel = 0;
	subResource.layerCount = 1;

	VkImageCopy region = {};
	region.srcSubresource = subResource;
	region.dstSubresource = subResource;
	region.srcOffset = { 0, 0, 0 };
	region.dstOffset = { 0, 0, 0 };
	region.extent.width = width;
	region.extent.height = height;
	region.extent.depth = 1;

	vkCmdCopyImage(
		commandBuffer,
		srcImage, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
		dstImage, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
		1, &region
	);

	endSingleTimeCommands(commandBuffer);

}

void RendererVulkan::createTextureImage(void*  i_data, int i_Widht, int i_Height, VKHandleWrapper<VkImage>& io_Image, VKHandleWrapper<VkDeviceMemory>& io_Memory)
{
	VkDeviceSize imageSize = i_Widht * i_Height * 4;//Assuming 4 channels for now

	VKHandleWrapper<VkImage> stagingImage{ m_LogicalDevice, vkDestroyImage };
	VKHandleWrapper<VkDeviceMemory> stagingImageMemory{ m_LogicalDevice, vkFreeMemory };
	createVulkanImage(i_Widht, i_Height, VK_FORMAT_R8G8B8A8_UNORM, VK_IMAGE_TILING_LINEAR, VK_IMAGE_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, stagingImage, stagingImageMemory);


	//We need to know if there is any padding per row of the texture so we can us memcpy directly or not
	VkImageSubresource subresource = {};
	subresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
	subresource.mipLevel = 0;
	subresource.arrayLayer = 0;

	VkSubresourceLayout stagingImageLayout;
	vkGetImageSubresourceLayout(m_LogicalDevice, stagingImage, &subresource, &stagingImageLayout);


	void* data;
	vkMapMemory(m_LogicalDevice, stagingImageMemory, 0, imageSize, 0, &data);

	//row matches exactly w*4 so we can just memcpy
	if (stagingImageLayout.rowPitch == i_Widht * 4) {
		memcpy(data, i_data, (size_t)imageSize);
	}
	//There is some padding so we need to go row by row
	else
	{
		uint8_t* dataOrigin = reinterpret_cast<uint8_t*>(i_data);
		uint8_t* dataBytes = reinterpret_cast<uint8_t*>(data);

		for (int y = 0; y < i_Height; y++) {
			memcpy(&dataBytes[y * stagingImageLayout.rowPitch], &dataOrigin[y * i_Widht * 4], i_Widht * 4);
		}
	}

	vkUnmapMemory(m_LogicalDevice, stagingImageMemory);

	createVulkanImage(i_Widht, i_Height, VK_FORMAT_R8G8B8A8_UNORM, VK_IMAGE_TILING_OPTIMAL, VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
		VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, io_Image, io_Memory);

	//Before anything images should transition to its optimal transfer states
	transitionImageLayout(stagingImage, VK_FORMAT_R8G8B8A8_UNORM, VK_IMAGE_LAYOUT_PREINITIALIZED, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL);
	transitionImageLayout(io_Image, VK_FORMAT_R8G8B8A8_UNORM, VK_IMAGE_LAYOUT_PREINITIALIZED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);
	copyImage(stagingImage, io_Image, i_Widht, i_Height);

	transitionImageLayout(io_Image, VK_FORMAT_R8G8B8A8_UNORM, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);



}


void RendererVulkan::createImageView(VkImage image, VkFormat format, VkImageAspectFlags aspectFlags, VKHandleWrapper<VkImageView>& imageView)
{
	VkImageViewCreateInfo viewInfo = {};
	viewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
	viewInfo.image = image;
	viewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
	viewInfo.format = format;
	viewInfo.subresourceRange.aspectMask = aspectFlags;
	viewInfo.subresourceRange.baseMipLevel = 0;
	viewInfo.subresourceRange.levelCount = 1;
	viewInfo.subresourceRange.baseArrayLayer = 0;
	viewInfo.subresourceRange.layerCount = 1;

	if (vkCreateImageView(m_LogicalDevice, &viewInfo, nullptr, imageView.replace()) != VK_SUCCESS) {
		throw std::runtime_error("failed to create texture image view!");
	}
}

void RendererVulkan::copyVKBuffer(VkBuffer srcBuffer, VkBuffer dstBuffer, VkDeviceSize size)
{
	VkCommandBuffer commandBuffer = beginSingleTimeCommands();

	VkBufferCopy copyRegion = {};
	copyRegion.size = size;
	vkCmdCopyBuffer(commandBuffer, srcBuffer, dstBuffer, 1, &copyRegion);

	endSingleTimeCommands(commandBuffer);


}



VkCommandBuffer RendererVulkan::beginSingleTimeCommands()
{
	VkCommandBufferAllocateInfo allocInfo = {};
	allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
	allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
	allocInfo.commandPool = m_CommandPool;
	allocInfo.commandBufferCount = 1;

	VkCommandBuffer commandBuffer;
	vkAllocateCommandBuffers(m_LogicalDevice, &allocInfo, &commandBuffer);

	VkCommandBufferBeginInfo beginInfo = {};
	beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
	beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

	vkBeginCommandBuffer(commandBuffer, &beginInfo);

	return commandBuffer;
}
void RendererVulkan::endSingleTimeCommands(VkCommandBuffer commandBuffer)
{
	vkEndCommandBuffer(commandBuffer);

	VkSubmitInfo submitInfo = {};
	submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
	submitInfo.commandBufferCount = 1;
	submitInfo.pCommandBuffers = &commandBuffer;

	vkQueueSubmit(m_graphicsQueue, 1, &submitInfo, VK_NULL_HANDLE);
	vkQueueWaitIdle(m_graphicsQueue);

	vkFreeCommandBuffers(m_LogicalDevice, m_CommandPool, 1, &commandBuffer);

}

void RendererVulkan::transitionImageLayout(VkImage image, VkFormat format, VkImageLayout oldLayout, VkImageLayout newLayout)
{
	VkCommandBuffer commandBuffer = beginSingleTimeCommands();

  	VkImageMemoryBarrier barrier = {};
	barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
	barrier.oldLayout = oldLayout;
	barrier.newLayout = newLayout;
	barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
	barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
	barrier.image = image;



	if (newLayout == VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL) {
		barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;

		if (hasStencilComponent(format)) {
			barrier.subresourceRange.aspectMask |= VK_IMAGE_ASPECT_STENCIL_BIT;
		}
	}
	else {
		barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
	}


	barrier.subresourceRange.baseMipLevel = 0;
	barrier.subresourceRange.levelCount = 1;
	barrier.subresourceRange.baseArrayLayer = 0;
	barrier.subresourceRange.layerCount = 1;


	if (oldLayout == VK_IMAGE_LAYOUT_PREINITIALIZED && newLayout == VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL) {
		barrier.srcAccessMask = VK_ACCESS_HOST_WRITE_BIT;
		barrier.dstAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
	}
	else if (oldLayout == VK_IMAGE_LAYOUT_PREINITIALIZED && newLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL) {
		barrier.srcAccessMask = VK_ACCESS_HOST_WRITE_BIT;
		barrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
	}
	else if (oldLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL && newLayout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL) {
		barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
		barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
	}
	else if (oldLayout == VK_IMAGE_LAYOUT_UNDEFINED && newLayout == VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL) {
		barrier.srcAccessMask = 0;
		barrier.dstAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
	}
	else {
		throw std::invalid_argument("unsupported layout transition!");
	}

	vkCmdPipelineBarrier(
		commandBuffer,
		VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT,
		0,
		0, nullptr,
		0, nullptr,
		1, &barrier
	);






	endSingleTimeCommands(commandBuffer);
}

VkSurfaceFormatKHR RendererVulkan::chooseSwapSurfaceFormat(const std::vector<VkSurfaceFormatKHR>& availableFormats)
{
	//Best case scenario, if size =1 and format undefined we can choose whatever we want
	if (availableFormats.size() == 1 && availableFormats[0].format == VK_FORMAT_UNDEFINED) {
		return { VK_FORMAT_B8G8R8A8_UNORM, VK_COLOR_SPACE_SRGB_NONLINEAR_KHR };
	}

	for (const auto& availableFormat : availableFormats) {
		if (availableFormat.format == VK_FORMAT_B8G8R8A8_UNORM && availableFormat.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR) {
			return availableFormat;
		}
	}

	//If everything fails we just return the first one available
	return availableFormats[0];

}

VkPresentModeKHR RendererVulkan::chooseSwapPresentMode(const std::vector<VkPresentModeKHR> availablePresentModes)
{
	VkPresentModeKHR bestMode = VK_PRESENT_MODE_FIFO_KHR;//Only this guaranteed to exist

	for (const auto& availablePresentMode : availablePresentModes) {
		if (availablePresentMode == VK_PRESENT_MODE_MAILBOX_KHR) {//Triple buffering with less latency the one we are looking for
			return availablePresentMode;
		}
		else if (availablePresentMode == VK_PRESENT_MODE_IMMEDIATE_KHR) {
			bestMode = availablePresentMode;
		}
	}

	return bestMode;
}

VkExtent2D RendererVulkan::chooseSwapExtent(const VkSurfaceCapabilitiesKHR& capabilities, int width, int height)
{
	if (capabilities.currentExtent.width != std::numeric_limits<uint32_t>::max()) {
		return capabilities.currentExtent;
	}
	else {

		
		VkExtent2D actualExtent = { width, height };


		actualExtent.width = std::max(capabilities.minImageExtent.width, std::min(capabilities.maxImageExtent.width, actualExtent.width));
		actualExtent.height = std::max(capabilities.minImageExtent.height, std::min(capabilities.maxImageExtent.height, actualExtent.height));

		return actualExtent;
	}
}



int32_t RendererVulkan::findMemoryType(uint32_t typeFilter, VkMemoryPropertyFlags properties)
{
	VkPhysicalDeviceMemoryProperties memProperties;
	vkGetPhysicalDeviceMemoryProperties(m_PhysicalDevice, &memProperties);
	
	//So here we have to check if the bit specified in typeFilter is set to 1, and also that the memorytype flag matches the input param properties, if so thats our mem type
	for (uint32_t i = 0; i < memProperties.memoryTypeCount; i++) {
		if ((typeFilter & (1 << i)) && (memProperties.memoryTypes[i].propertyFlags & properties) == properties) {
			return i;
		}
	}
	throw std::runtime_error("failed to find suitable memory type!");

	return -1;
}

std::vector<char> RendererVulkan::readFile(const std::string & filename)
{
	std::ifstream file(filename, std::ios::ate | std::ios::binary); //Start reading EOF so we can now its size

	if (!file.is_open()) {
		throw std::runtime_error("failed to open file!");
	}

	size_t fileSize = (size_t)file.tellg();
	std::vector<char> buffer(fileSize);

	file.seekg(0);
	file.read(buffer.data(), fileSize);

	file.close();

	return buffer;
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


#ifndef NDEBUG
void  RendererVulkan::setupDebugCallback() 
{
	VkDebugReportCallbackCreateInfoEXT createInfo = {};
	createInfo.sType = VK_STRUCTURE_TYPE_DEBUG_REPORT_CALLBACK_CREATE_INFO_EXT;
	createInfo.flags = VK_DEBUG_REPORT_ERROR_BIT_EXT | VK_DEBUG_REPORT_WARNING_BIT_EXT;
	createInfo.pfnCallback = debugCallback;

	auto func = (PFN_vkCreateDebugReportCallbackEXT)vkGetInstanceProcAddr(m_Instance, "vkCreateDebugReportCallbackEXT");
	if (func != nullptr) {
		VkResult  res = func(m_Instance, &createInfo, nullptr, m_Debugcallback.replace());
		if(res != VK_SUCCESS)
			throw std::runtime_error("failed to set up debug callback!");
	}
	else {
		throw std::runtime_error("failed to set up debug callback!");
	}

}



#endif
