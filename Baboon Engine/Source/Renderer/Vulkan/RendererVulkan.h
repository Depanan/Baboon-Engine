#pragma once
#define NOMINMAX //To avoid windows.h name collision
#include "Renderer\RendererAbstract.h"
#include "vulkan\vulkan.h"
#include <vector>
#include <iostream>
#include <functional>
#include "Renderer\Common\Mesh.h"
#include "UI\VulkanIMGUI.h"

template <typename T>
class VKHandleWrapper {

public:
	VKHandleWrapper() : VKHandleWrapper([](T, VkAllocationCallbacks*) {}) {}

	VKHandleWrapper(std::function<void(T, VkAllocationCallbacks*)> deletef) {
		m_DeleterFunction = [=](T handle) { deletef(handle, nullptr); };
	}

	VKHandleWrapper(const VKHandleWrapper<VkInstance>& instance, std::function<void(VkInstance, T, VkAllocationCallbacks*)> deletef) {
		m_DeleterFunction = [&instance, deletef](T handle) { deletef(instance, handle, nullptr); };
	}

	VKHandleWrapper(const VKHandleWrapper<VkDevice>& device, std::function<void(VkDevice, T, VkAllocationCallbacks*)> deletef) {
		m_DeleterFunction = [&device, deletef](T handle) { deletef(device, handle, nullptr); };
	}

	~VKHandleWrapper() {
		cleanup();
	}

	const T* operator &() const {
		return &m_VKHandle;
	}

	T* replace() {
		cleanup();
		return &m_VKHandle;
	}

	operator T() const {
		return m_VKHandle;
	}

	void operator=(T handle) {
		if (handle != m_VKHandle) {
			cleanup();
			m_VKHandle = handle;
		}
	}

	template<typename V>
	bool operator==(V handle) {
		return m_VKHandle == T(handle);
	}

private:
	T m_VKHandle{ VK_NULL_HANDLE };
	std::function<void(T)> m_DeleterFunction;

	void cleanup() {
		if (m_VKHandle != VK_NULL_HANDLE) {
				m_DeleterFunction(m_VKHandle);
		}
		m_VKHandle = VK_NULL_HANDLE;
	}
};

class VKUniformBufferWrapper
{
public:
	VKUniformBufferWrapper(){}
	void Init(VKHandleWrapper <VkDevice>& i_LogicalDevice, VkDeviceSize i_BufferSize)
	{
		m_UniformStagingBuffer= { i_LogicalDevice, vkDestroyBuffer };
		m_UniformStagingBufferMemory= { i_LogicalDevice, vkFreeMemory };
		m_UniformBuffer= { i_LogicalDevice, vkDestroyBuffer };
		m_UniformBufferMemory={ i_LogicalDevice, vkFreeMemory };
		m_BufferSize = i_BufferSize;
	}

	void UpdateDescriptor()
	{
		m_Descriptor.buffer = m_UniformBuffer;
		m_Descriptor.offset = 0;
		m_Descriptor.range = VK_WHOLE_SIZE;


	}

	VKHandleWrapper<VkBuffer>& GetStagingBuffer() { return m_UniformStagingBuffer; }
	VKHandleWrapper<VkDeviceMemory>& GetStagingMemory(){ return m_UniformStagingBufferMemory; }
	VKHandleWrapper<VkBuffer>&GetBuffer(){ return m_UniformBuffer; }
	VKHandleWrapper<VkDeviceMemory>&GetMemory(){ return m_UniformBufferMemory; }
	VkDescriptorBufferInfo& GetDescriptor() { return m_Descriptor; }
	VkDeviceSize GetBufferSize() { return m_BufferSize; }
private:

	VKHandleWrapper<VkBuffer>		m_UniformStagingBuffer = VK_NULL_HANDLE;
	VKHandleWrapper<VkDeviceMemory> m_UniformStagingBufferMemory = VK_NULL_HANDLE;
	VKHandleWrapper<VkBuffer>		m_UniformBuffer = VK_NULL_HANDLE;
	VKHandleWrapper<VkDeviceMemory> m_UniformBufferMemory = VK_NULL_HANDLE;
	VkDescriptorBufferInfo m_Descriptor;
	VkDeviceSize m_BufferSize;
};


class VKImageWrapper
{
public:
	VKImageWrapper() {};
	void Init(VKHandleWrapper <VkDevice>& i_LogicalDevice)
	{
		m_Image = { i_LogicalDevice, vkDestroyImage };
		m_ImageMemory = { i_LogicalDevice, vkFreeMemory };
		m_TextureImageView = { i_LogicalDevice, vkDestroyImageView };
	}

	
	VKHandleWrapper<VkImage>& GetVKImage() { return m_Image; };
	VKHandleWrapper<VkDeviceMemory>& GetVKImageMemory() { return m_ImageMemory; };
	VKHandleWrapper<VkImageView>& GetVKImageView() { return m_TextureImageView; };

private:
	VKHandleWrapper<VkImage> m_Image = VK_NULL_HANDLE;
	VKHandleWrapper<VkDeviceMemory> m_ImageMemory = VK_NULL_HANDLE;
	VKHandleWrapper<VkImageView> m_TextureImageView = VK_NULL_HANDLE;
};
class VKTextureWrapper
{
public:
	VKTextureWrapper(){};
	void Init(VKHandleWrapper <VkDevice>& i_LogicalDevice)
	{
		m_Image.Init(i_LogicalDevice);
		m_TextureSampler = { i_LogicalDevice, vkDestroySampler };
	}
	void UpdateDescriptor()
	{
		m_Descriptor.imageView = m_Image.GetVKImageView();
		m_Descriptor.imageLayout = m_Layout;
		m_Descriptor.sampler = m_TextureSampler;
		m_Layout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;//TODO: Make this a parameter?
	}

	VKHandleWrapper<VkImage>& GetVKImage() { return m_Image.GetVKImage(); };
	VKHandleWrapper<VkDeviceMemory>& GetVKImageMemory() { return m_Image.GetVKImageMemory(); };
	VKHandleWrapper<VkImageView>& GetVKImageView() { return  m_Image.GetVKImageView(); };
	VKHandleWrapper<VkSampler>& GetVKTextureSampler() { return  m_TextureSampler; };
	VkDescriptorImageInfo& GetDescriptor() { return m_Descriptor; }

private:
	VKImageWrapper m_Image;
	VKHandleWrapper<VkSampler> m_TextureSampler = VK_NULL_HANDLE;
	VkDescriptorImageInfo m_Descriptor;
	VkImageLayout  m_Layout;
};

class VulkanImGUI;
class RendererVulkan : public RendererAbstract
{
	friend class VulkanImGUI;
public:
	

	
	int Init(const char** i_requiredExtensions, const unsigned int i_nExtensions,  GLFWwindow* i_window) override;
	void Destroy() override;
	void DrawFrame() override;
	void OnWindowResize(int i_NewW, int i_NewH) override;
	void WaitToDestroy() override;

	float GetMainRTAspectRatio() override;
	float GetMainRTWidth() override;
	float GetMainRTHeight() override;
	void createTexture(void*  i_data, int i_Widht, int i_Height) override;
	void createVertexBuffer(const void*  i_data, size_t iBufferSize) override;;
	void createIndexBuffer(const void*  i_data, size_t iBufferSize) override;;
	void createStaticUniformBuffer(const void*  i_data, size_t iBufferSize) override;;
	void createInstancedUniformBuffer(const void*  i_data, size_t iBufferSize) override;;
	void createDescriptorSet();
	void SetupRenderCalls() override;

	void UpdateTimesAndFPS(std::chrono::time_point<std::chrono::high_resolution_clock>  i_tStartTime) override;
	
	VkCommandBuffer beginSingleTimeCommands();
	void endSingleTimeCommands(VkCommandBuffer commandBuffer);
	VkFormat GetMainRTColorFormat() {return m_SwapChainImageFormat;}
	VkFormat GetMainRTDepthFormat() { return m_SwapChainDepthFormat; }
private:


	struct QueueFamilyIndices {
		int graphicsFamily = -1;
		int presentFamily = -1;

		bool isComplete() {
			return graphicsFamily >= 0 && presentFamily >= 0;
		}
	};

	struct SwapChainSupportDetails {
		VkSurfaceCapabilitiesKHR capabilities;
		std::vector<VkSurfaceFormatKHR> formats;
		std::vector<VkPresentModeKHR> presentModes;
	};
	VKHandleWrapper<VkInstance> m_Instance{ vkDestroyInstance };

	VkApplicationInfo m_applicationInfo;
	VkInstanceCreateInfo m_instanceInfo;
	VkPhysicalDevice m_PhysicalDevice = VK_NULL_HANDLE;
	VKHandleWrapper <VkDevice> m_LogicalDevice{ vkDestroyDevice };
	
	VkQueue m_graphicsQueue;
	VkQueue m_PresentQueue;
	VKHandleWrapper <VkSurfaceKHR> m_Surface  { m_Instance,vkDestroySurfaceKHR };
	VKHandleWrapper <VkSwapchainKHR> m_SwapChain  { m_LogicalDevice,vkDestroySwapchainKHR };
	VKHandleWrapper<VkRenderPass> m_RenderPass  { m_LogicalDevice,vkDestroyRenderPass };
	VKHandleWrapper<VkCommandPool> m_CommandPool  { m_LogicalDevice,vkDestroyCommandPool };
	VKHandleWrapper<VkCommandPool> m_MemTransferCommandPool{ m_LogicalDevice,vkDestroyCommandPool };

	std::vector<VkImage> m_SwapChainImages;
	std::vector<VKHandleWrapper<VkImageView>> m_SwapChainImageViews; //views to the images above
	std::vector<VKHandleWrapper<VkFramebuffer>> m_SwapChainFramebuffers;
	std::vector<VkCommandBuffer> m_CommandBuffers;

	VKImageWrapper m_DepthImage;

	VKHandleWrapper<VkBuffer> m_MainVertexBuffer{ m_LogicalDevice, vkDestroyBuffer };
	VKHandleWrapper<VkDeviceMemory>  m_MainVertexBufferMemory{ m_LogicalDevice, vkFreeMemory };
	VKHandleWrapper<VkBuffer> m_MainIndexBuffer{ m_LogicalDevice, vkDestroyBuffer };
	VKHandleWrapper<VkDeviceMemory> m_MainIndexBufferMemory{ m_LogicalDevice, vkFreeMemory };

	//Uniform buffer stuff, we keep the staging buffer as we will update uniforms every frame
	std::vector<VKUniformBufferWrapper> m_StaticUniformBuffers;
	std::vector<VKUniformBufferWrapper> m_DynamicUniformBuffers;


	VkFormat m_SwapChainImageFormat;
	VkFormat m_SwapChainDepthFormat;
	VkExtent2D m_SwapChainExtent;

	//Used to send uniforms to shaders
	VKHandleWrapper<VkDescriptorSetLayout> m_DescriptorSetLayout{ m_LogicalDevice, vkDestroyDescriptorSetLayout };
	VKHandleWrapper<VkPipelineLayout> m_PipelineLayout  { m_LogicalDevice,vkDestroyPipelineLayout };
	VKHandleWrapper<VkPipeline> m_GraphicsPipeline  { m_LogicalDevice,vkDestroyPipeline };

	VKHandleWrapper<VkDescriptorPool> m_DescriptorPool{ m_LogicalDevice, vkDestroyDescriptorPool };
	VkDescriptorSet m_DescriptorSet;


	//Images/Textures stuff, this is likely to evolve into a POOL of images...
	std::vector<VKTextureWrapper> m_Textures;


	//Sync stuff
	VKHandleWrapper <VkSemaphore> m_ImageAvailableSemaphore  { m_LogicalDevice,vkDestroySemaphore };
	VKHandleWrapper <VkSemaphore> m_RenderFinishedSemaphore  { m_LogicalDevice,vkDestroySemaphore };
	

	VulkanImGUI m_GUI;
	

	
	float m_UpdateUniformsTime = 0.0f;
	

	const std::vector<const char*> m_VvalidationLayers = {
		"VK_LAYER_LUNARG_standard_validation"
	};

	const std::vector<const char*> deviceExtensions = {
		VK_KHR_SWAPCHAIN_EXTENSION_NAME
	};

	#ifdef NDEBUG
		const bool m_bEnableValidationLayers = false;
	#else
		const bool m_bEnableValidationLayers = true;


		static VKAPI_ATTR VkBool32 VKAPI_CALL debugCallback(
			VkDebugReportFlagsEXT flags,
			VkDebugReportObjectTypeEXT objType,
			uint64_t obj,
			size_t location,
			int32_t code,
			const char* layerPrefix,
			const char* msg,
			void* userData) {

			std::cerr << "validation layer: " << msg << std::endl;

			return VK_FALSE;
		}

		static void DestroyDebugReportCallbackEXT(VkInstance instance, VkDebugReportCallbackEXT callback, const VkAllocationCallbacks* pAllocator) {
			auto func = (PFN_vkDestroyDebugReportCallbackEXT)vkGetInstanceProcAddr(instance, "vkDestroyDebugReportCallbackEXT");
			if (func != nullptr) {
				func(instance, callback, pAllocator);
			}
		}

		VKHandleWrapper<VkDebugReportCallbackEXT> m_Debugcallback{m_Instance,DestroyDebugReportCallbackEXT };


		void setupDebugCallback();

	#endif

	

	VkResult createInstance(const char** i_requiredExtensions, const unsigned int i_nExtensions);
	void createSurface( GLFWwindow* i_window);
	void pickPhysicalDevice();
	void createLogicalDevice();
	void createSwapChain(int width, int height);
	void createImageViews();
	void createRenderPass();
	void createDescriptorSetLayout();
	void createGraphicsPipeline();
	void createFramebuffers();
	void createCommandPools();
	void createDepthResources();
	
	
	void createTextureImage(void*  i_data, int i_Widht, int i_Height, VKHandleWrapper<VkImage>& io_Image, VKHandleWrapper<VkDeviceMemory>& io_Memory);
	void createTextureSampler(VKHandleWrapper<VkSampler>& i_Sampler);

	void createDescriptorPool();
	

	void createCommandBuffers();//Hack here with nindices, try to figure out something better
	void recordDrawCommandBuffers();//For the scene
	void createSemaphores();


	void UploadSceneUniforms();//Sends to the device updated scene uniforms

	bool isDeviceSuitable(VkPhysicalDevice device);
	bool checkDeviceExtensionSupport(VkPhysicalDevice device);

	QueueFamilyIndices  findQueueFamilies(VkPhysicalDevice device);
	SwapChainSupportDetails querySwapChainSupport(VkPhysicalDevice device);

	

	VkSurfaceFormatKHR chooseSwapSurfaceFormat(const std::vector<VkSurfaceFormatKHR>& availableFormats);
	VkPresentModeKHR chooseSwapPresentMode(const std::vector<VkPresentModeKHR> availablePresentModes);
	VkExtent2D chooseSwapExtent(const VkSurfaceCapabilitiesKHR& capabilities, int width, int height);//Basically window resolution

	void createShaderModule(const std::string& filename, VkShaderModule& shaderModule);
	void createVulkanImage(uint32_t width, uint32_t height, VkFormat format, VkImageTiling tiling, VkImageUsageFlags usage, VkMemoryPropertyFlags properties, VKHandleWrapper<VkImage>& image, VKHandleWrapper<VkDeviceMemory>& imageMemory);
	void createImageView(VkImage image, VkFormat format, VkImageAspectFlags aspectFlags, VKHandleWrapper<VkImageView>& imageView);
	void createVKBuffer(VkDeviceSize size, VkBufferUsageFlags usage, VkMemoryPropertyFlags properties, VKHandleWrapper<VkBuffer>& buffer, VKHandleWrapper<VkDeviceMemory>& bufferMemory);
	void copyVKBuffer(VkBuffer srcBuffer, VkBuffer dstBuffer, VkDeviceSize size);
	void copyImage(VkImage srcImage, VkImage dstImage, uint32_t width, uint32_t height);

	

	void transitionImageLayout(VkImage image, VkFormat format, VkImageLayout oldLayout, VkImageLayout newLayout);

	int32_t findMemoryType(uint32_t typeFilter, VkMemoryPropertyFlags properties);

	static std::vector<char> readFile(const std::string& filename);
	
};	
