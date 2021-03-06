#pragma once
#define NOMINMAX //To avoid windows.h name collision
#include "Renderer\RendererAbstract.h"
#include "Common.h"
#include <memory>
#include "UI\VulkanIMGUI.h"
#include "Instance.h"
#include "Device.h"
#include "VulkanContext.h"
#include "RenderPath.h"
#include <list>
#include "Core/Observer.h"

class VulkanImGUI;
class RendererVulkan : public RendererAbstract, public Observer
{
    friend class VulkanImGUI;
public:
	virtual ~RendererVulkan(){}
	int Init(std::vector<const char*>& required_extensions,  GLFWwindow* i_window, const Camera* p_Camera) override;
	void Destroy() override;
	void DrawFrame() override;
	void OnWindowResize(int i_NewW, int i_NewH) override;
	void WaitToDestroy() override;
  void Update() override;
	float GetMainRTAspectRatio() override;
	float GetMainRTWidth() override;
	float GetMainRTHeight() override;
 

	Buffer* CreateVertexBuffer( void*  i_data, size_t iBufferSize) override;
  Buffer* CreateIndexBuffer( void*  i_data, size_t iBufferSize) override;
	void DeleteBuffer(Buffer*) override;
  Buffer* CreateStaticUniformBuffer( void*  i_data, size_t iBufferSize) override;
  Buffer* CreateInstancedUniformBuffer( void*  i_data, size_t iBufferSize) override;
	void DeleteStaticUniformBuffer() override;
	void DeleteInstancedUniformBuffer() override;
  void ReloadShader(std::string) override;
	void UpdateTimesAndFPS(std::chrono::time_point<std::chrono::high_resolution_clock>  i_tStartTime) override;


  void ObserverUpdate(int message, void* data)override;
  

  //This 3 to be implemented
  virtual Texture* CreateTexture(void* i_data, int i_Widht, int i_Height) override;
  virtual void DeleteTexture(Texture*) override;

  virtual void CreateMaterial(std::string i_MatName, int* iTexIndices, int iNumTextures) override{ }

	VkFormat GetMainRTColorFormat() {return m_SwapChainImageFormat;}
	VkFormat GetMainRTDepthFormat() { return m_SwapChainDepthFormat; }

  ShaderSourcePool& getShaderSourcePool() {
      return m_ShaderSourcePool;
  }
private:

    size_t m_ThreadCount = 1;
    bool m_SceneLoaded = false;
    bool m_Dirty = false;

  std::unique_ptr<Instance> m_Instance{ nullptr };
  VkSurfaceKHR m_Surface{ VK_NULL_HANDLE };
	VkPhysicalDevice m_PhysicalDevice = VK_NULL_HANDLE;
  std::unique_ptr<Device> m_LogicalDevice{ nullptr };
  std::unique_ptr<VulkanContext> m_RenderContext{ nullptr };
  std::unique_ptr<RenderPath> m_RenderPath{ nullptr };


#define SHADOWMAP_RESOLUTION 1024
#define SHADOWMAP_FORMAT VK_FORMAT_D32_SFLOAT_S8_UINT
  std::unique_ptr<RenderPath> m_ShadowPath{ nullptr };
  std::unique_ptr<RenderTarget> m_ShadowRT{ nullptr };

  std::list<VulkanImage> m_Images;//Using lists here so pointers remain valid!
  std::list<VulkanImageView> m_ImageViews;
  std::list <VulkanSampler> m_Samplers;
  std::list<VulkanBuffer> m_Buffers;

  //std::unique_ptr <VulkanImGUI> m_GUI{ nullptr };

	VkFormat m_SwapChainImageFormat;
	VkFormat m_SwapChainDepthFormat;
	VkExtent2D m_SwapChainExtent;

  ShaderSourcePool m_ShaderSourcePool;

  void createSurface(GLFWwindow* i_window);
  bool isDeviceSuitable(VkPhysicalDevice device);
  void pickPhysicalDevice();
  void reRecordCommands();

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
	#endif

	
};	
