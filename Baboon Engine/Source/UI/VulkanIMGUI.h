#pragma once
#include "vulkan\vulkan.h"
#include <GLFW/glfw3.h>
#include <vector>
class VulkanImGUI
{
public:
	~VulkanImGUI();
	void Init(GLFWwindow* i_window);
	void DoUI(bool i_FirstCall = false);
	void Draw(VkQueue i_queue, uint32_t i_bufferindex, const std::vector<VkSemaphore>& i_SignalSemaphores,const std::vector<VkSemaphore>& i_WaitSemaphores , const std::vector<VkPipelineStageFlags>& i_WaitSemaphoresStages);
	void OnWindowResize();
private:
	
	VkCommandPool m_CommandPool;
	std::vector<VkCommandBuffer> m_CmdBuffers;
	VkRenderPass m_RenderPass;
	VkFence m_ReSubmitFence;
	VkDescriptorPool m_DescriptorPool = VK_NULL_HANDLE;
	void CreateRenderPass();
	void CreateDescriptorPool();
	void CreateCommandPool();
	void CreateCommandBuffers();
	void CreateReSubmitFence();
	void UpdateCommandBuffers(bool i_ForceSkipFence = false);


	void RenderStatsWindow(bool* pOpen);

	float m_UpdateTimer = 0.0f;

};
