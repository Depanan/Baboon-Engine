#include "VulkanIMGUI.h"
#include "UI\imgui\imgui.h"
#include "UI\imgui\imgui_impl_glfw_vulkan.h"
#include "defines.h"
#include "Cameras\CameraManager.h"

#include <windows.h>
#include <iostream>


VulkanImGUI::~VulkanImGUI()
{
	ImGui_ImplGlfwVulkan_Shutdown();
	RendererVulkan* pRenderer = (RendererVulkan*)ServiceLocator::GetRenderer();
	vkDestroyFence(pRenderer->m_LogicalDevice, m_ReSubmitFence, VK_NULL_HANDLE);
	vkDestroyCommandPool(pRenderer->m_LogicalDevice, m_CommandPool, VK_NULL_HANDLE);
	vkDestroyRenderPass(pRenderer->m_LogicalDevice, m_RenderPass,VK_NULL_HANDLE);
	vkDestroyDescriptorPool(pRenderer->m_LogicalDevice, m_DescriptorPool, VK_NULL_HANDLE);
}

void VulkanImGUI::Init(GLFWwindow* i_window)
{
	//Init Fence
	CreateReSubmitFence();

	//Init renderpass
	CreateRenderPass();
	//Init descriptor pool
	CreateDescriptorPool();

	RendererVulkan* pRenderer = (RendererVulkan*)ServiceLocator::GetRenderer();

	ImGui_ImplGlfwVulkan_Init_Data init_data = {};
	init_data.allocator = VK_NULL_HANDLE;
	init_data.gpu = pRenderer->m_PhysicalDevice;
	init_data.device = pRenderer->m_LogicalDevice;
	init_data.render_pass = m_RenderPass;
	init_data.pipeline_cache = VK_NULL_HANDLE;
	init_data.descriptor_pool = m_DescriptorPool;
	init_data.check_vk_result = VK_NULL_HANDLE;
	ImGui_ImplGlfwVulkan_Init(i_window, true, &init_data);//This function creates descriptor sets and all the stuff needed with the provided init data struct

	VkCommandBuffer createFontsCommand = pRenderer->beginSingleTimeCommands();
	ImGui_ImplGlfwVulkan_CreateFontsTexture(createFontsCommand);
	pRenderer->endSingleTimeCommands(createFontsCommand);
	ImGui_ImplGlfwVulkan_InvalidateFontUploadObjects();

	
	CreateCommandPool();
	CreateCommandBuffers();
}
void VulkanImGUI::OnWindowResize()
{
	RendererVulkan* pRenderer = (RendererVulkan*)ServiceLocator::GetRenderer();
	vkDestroyRenderPass(pRenderer->m_LogicalDevice, m_RenderPass, VK_NULL_HANDLE);
	CreateRenderPass();
	CreateCommandBuffers();
	UpdateCommandBuffers();
	
}

void VulkanImGUI::Draw(VkQueue i_queue, uint32_t i_bufferindex, const std::vector<VkSemaphore>& i_SignalSemaphores, const std::vector<VkSemaphore>& i_WaitSemaphores, const std::vector<VkPipelineStageFlags>& i_WaitSemaphoresStages)
{
	RendererVulkan* pRenderer = (RendererVulkan*)ServiceLocator::GetRenderer();

	VkSubmitInfo submitInfo = {};
	submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
	submitInfo.commandBufferCount = 1;
	submitInfo.pCommandBuffers = &m_CmdBuffers[i_bufferindex];
	submitInfo.pWaitSemaphores = i_WaitSemaphores.data();
	submitInfo.waitSemaphoreCount = i_WaitSemaphores.size();
	submitInfo.pWaitDstStageMask = i_WaitSemaphoresStages.data();
	submitInfo.pSignalSemaphores = i_SignalSemaphores.data();
	submitInfo.signalSemaphoreCount = i_SignalSemaphores.size();

	
	if (vkQueueSubmit(i_queue, 1, &submitInfo, VK_NULL_HANDLE) != VK_SUCCESS) {
		throw std::runtime_error("failed to submit draw command buffer!");
	}

	vkQueueWaitIdle(i_queue);
}


void VulkanImGUI::RenderStatsWindow(bool* pOpen)
{
	RendererVulkan* pRenderer = (RendererVulkan*)ServiceLocator::GetRenderer();
	ImGui::SetNextWindowSize(ImVec2(200, 100), ImGuiSetCond_FirstUseEver);
	if (ImGui::Begin("App stats", pOpen, ImGuiWindowFlags_MenuBar))
	{
		ImGui::Text("FPS: %d", pRenderer->m_LastFPS);
		const Camera* cam = ServiceLocator::GetCameraManager()->GetCamera(CameraManager::eCameraType_Main);
		const glm::vec3 camPos = cam->GetPosition();
		ImGui::Text("Scene cam Pos = (%.2f,%.2f,%.2f)",camPos.x,camPos.y,camPos.z);


	
	}
	ImGui::End();
}

std::string BasicFileOpen()
{
	std::string fileOpen = "";
	char filename[MAX_PATH];

	OPENFILENAME ofn;
	ZeroMemory(&filename, sizeof(filename));
	ZeroMemory(&ofn, sizeof(ofn));
	ofn.lStructSize = sizeof(ofn);
	ofn.hwndOwner = NULL;  // If you have a window to center over, put its HANDLE here
	ofn.lpstrFilter = "OBJ Files\0*.obj\0Any File\0*.*\0";
	ofn.lpstrFile = filename;
	ofn.nMaxFile = MAX_PATH;
	ofn.lpstrTitle = "Select a File, yo!";
	ofn.Flags = OFN_DONTADDTORECENT | OFN_FILEMUSTEXIST | OFN_NOCHANGEDIR;

	if (GetOpenFileNameA(&ofn))
	{
		fileOpen = std::string(filename);
	}

	return fileOpen;
}

void VulkanImGUI::DoUI(bool i_FirstCall)
{
	ServiceLocator::GetRenderer();

	m_UpdateTimer += ServiceLocator::GetRenderer()->GetDeltaTime();
	
	if (m_UpdateTimer < 1.0f/60.0f) {
		
		return;
	}
		
	m_UpdateTimer = 0.0f;

	ImGui_ImplGlfwVulkan_NewFrame();
	
	static bool bStatsWindow = false;
	static bool bLoadScene = false;

	if (bStatsWindow)
	{
		RenderStatsWindow(&bStatsWindow);
	}
	if (bLoadScene)
	{
		
		
		std::string filePath = BasicFileOpen();
		if (filePath.size() > 0)
		{
			ServiceLocator::GetSceneManager()->GetScene()->Init(filePath);
		}
		

		bLoadScene = false;
	}

	if (ImGui::BeginMainMenuBar())
	{
		if (ImGui::BeginMenu("Options"))
		{
			ImGui::MenuItem("App stats", NULL, &bStatsWindow);
			ImGui::MenuItem("Load Scene", NULL, &bLoadScene);
			ImGui::EndMenu();
		}

		ImGui::EndMainMenuBar();
	}
	

	

	//bool bOpen;
	//ImGui::ShowTestWindow(&bOpen);
	///////////////////////////////////////////////////////

	UpdateCommandBuffers(i_FirstCall);
}
void VulkanImGUI::CreateReSubmitFence()
{
	RendererVulkan* pRenderer = (RendererVulkan*)ServiceLocator::GetRenderer();
	VkFenceCreateInfo fenceCreateInfo;
	fenceCreateInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
	fenceCreateInfo.flags = 0;
	fenceCreateInfo.pNext = VK_NULL_HANDLE;
	vkCreateFence(pRenderer->m_LogicalDevice, &fenceCreateInfo, NULL, &m_ReSubmitFence);
}
void VulkanImGUI::CreateCommandPool()
{
	RendererVulkan* pRenderer = (RendererVulkan*)ServiceLocator::GetRenderer();
	RendererVulkan::QueueFamilyIndices queueFamilyIndices = pRenderer->findQueueFamilies(pRenderer->m_PhysicalDevice);

	VkCommandPoolCreateInfo cmdPoolInfo = {};
	cmdPoolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
	cmdPoolInfo.queueFamilyIndex = queueFamilyIndices.graphicsFamily;
	cmdPoolInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
	vkCreateCommandPool(pRenderer->m_LogicalDevice, &cmdPoolInfo, nullptr, &m_CommandPool);
	

}

void VulkanImGUI::CreateCommandBuffers()
{
	RendererVulkan* pRenderer = (RendererVulkan*)ServiceLocator::GetRenderer();
	
	//In case we regenerated the swapchain we need to reset this...
	if (m_CmdBuffers.size() > 0) {
		vkFreeCommandBuffers(pRenderer->m_LogicalDevice, m_CommandPool, m_CmdBuffers.size(), m_CmdBuffers.data());
	}
	//Resize to match framebuffers, we will have one command buffer per framebuffer
	m_CmdBuffers.resize(pRenderer->m_SwapChainFramebuffers.size());

	VkCommandBufferAllocateInfo allocInfo = {};
	allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
	allocInfo.commandPool = m_CommandPool;
	allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
	allocInfo.commandBufferCount = (uint32_t)m_CmdBuffers.size();

	if (vkAllocateCommandBuffers(pRenderer->m_LogicalDevice, &allocInfo, m_CmdBuffers.data()) != VK_SUCCESS) {
		throw std::runtime_error("failed to allocate command buffers!");
	}
}

void VulkanImGUI::CreateDescriptorPool()
{
	m_DescriptorPool = VK_NULL_HANDLE;
	RendererVulkan* pRenderer = (RendererVulkan*)ServiceLocator::GetRenderer();


	std::array<VkDescriptorPoolSize, 1> poolSizes = {};
	poolSizes[0].type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
	poolSizes[0].descriptorCount = 1;
	

	VkDescriptorPoolCreateInfo poolInfo = {};
	poolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
	poolInfo.poolSizeCount = poolSizes.size();
	poolInfo.pPoolSizes = poolSizes.data();
	poolInfo.maxSets = 2;

	if (vkCreateDescriptorPool(pRenderer->m_LogicalDevice, &poolInfo, nullptr, &m_DescriptorPool) != VK_SUCCESS) {
		throw std::runtime_error("failed to create descriptor pool!");
	}

}
void VulkanImGUI::CreateRenderPass()
{
	RendererVulkan* pRenderer = (RendererVulkan*)ServiceLocator::GetRenderer();

	VkAttachmentDescription attachments[2] = {};

	// Color attachment
	attachments[0].format = pRenderer->GetMainRTColorFormat();
	attachments[0].samples = VK_SAMPLE_COUNT_1_BIT;
	// Don't clear the framebuffer (like the renderpass from the example does)
	attachments[0].loadOp = VK_ATTACHMENT_LOAD_OP_LOAD;
	attachments[0].storeOp = VK_ATTACHMENT_STORE_OP_STORE;
	attachments[0].stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
	attachments[0].stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
	attachments[0].initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
	attachments[0].finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

	// Depth attachment
	attachments[1].format = pRenderer->GetMainRTDepthFormat();
	attachments[1].samples = VK_SAMPLE_COUNT_1_BIT;
	attachments[1].loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
	attachments[1].storeOp = VK_ATTACHMENT_STORE_OP_STORE;
	attachments[1].stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
	attachments[1].stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
	attachments[1].initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
	attachments[1].finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

	VkAttachmentReference colorReference = {};
	colorReference.attachment = 0;
	colorReference.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

	VkAttachmentReference depthReference = {};
	depthReference.attachment = 1;
	depthReference.layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

	// Use subpass dependencies for image layout transitions
	VkSubpassDependency subpassDependencies[2] = {};

	// Transition from final to initial (VK_SUBPASS_EXTERNAL refers to all commmands executed outside of the actual renderpass)
	subpassDependencies[0].srcSubpass = VK_SUBPASS_EXTERNAL;
	subpassDependencies[0].dstSubpass = 0;
	subpassDependencies[0].srcStageMask = VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT;
	subpassDependencies[0].dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
	subpassDependencies[0].srcAccessMask = VK_ACCESS_MEMORY_READ_BIT;
	subpassDependencies[0].dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
	subpassDependencies[0].dependencyFlags = VK_DEPENDENCY_BY_REGION_BIT;

	// Transition from initial to final
	subpassDependencies[1].srcSubpass = 0;
	subpassDependencies[1].dstSubpass = VK_SUBPASS_EXTERNAL;
	subpassDependencies[1].srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
	subpassDependencies[1].dstStageMask = VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT;
	subpassDependencies[1].srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
	subpassDependencies[1].dstAccessMask = VK_ACCESS_MEMORY_READ_BIT;
	subpassDependencies[1].dependencyFlags = VK_DEPENDENCY_BY_REGION_BIT;

	VkSubpassDescription subpassDescription = {};
	subpassDescription.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
	subpassDescription.flags = 0;
	subpassDescription.inputAttachmentCount = 0;
	subpassDescription.pInputAttachments = NULL;
	subpassDescription.colorAttachmentCount = 1;
	subpassDescription.pColorAttachments = &colorReference;
	subpassDescription.pResolveAttachments = NULL;
	subpassDescription.pDepthStencilAttachment = &depthReference;
	subpassDescription.preserveAttachmentCount = 0;
	subpassDescription.pPreserveAttachments = NULL;

	VkRenderPassCreateInfo renderPassInfo = {};
	renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
	renderPassInfo.pNext = NULL;
	renderPassInfo.attachmentCount = 2;
	renderPassInfo.pAttachments = attachments;
	renderPassInfo.subpassCount = 1;
	renderPassInfo.pSubpasses = &subpassDescription;
	renderPassInfo.dependencyCount = 2;
	renderPassInfo.pDependencies = subpassDependencies;

	vkCreateRenderPass(pRenderer->m_LogicalDevice, &renderPassInfo, nullptr, &m_RenderPass);
}

void VulkanImGUI::UpdateCommandBuffers(bool i_ForceSkipFence)
{

	
	RendererVulkan* pRenderer = (RendererVulkan*)ServiceLocator::GetRenderer();

	
	vkResetCommandPool(pRenderer->m_LogicalDevice, m_CommandPool, 0);

	VkCommandBufferBeginInfo cmdBufferBeginInfo{};
	cmdBufferBeginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
	cmdBufferBeginInfo.flags = VK_COMMAND_BUFFER_USAGE_SIMULTANEOUS_USE_BIT;//Means we can submit whilst still drawing previous frame
	VkClearValue clearValues[2];
	clearValues[0].color = { { 0.0f, 0.0f, 0.0f, 0.0f } };
	clearValues[1].depthStencil = { 1.0f, 0 };

	VkRenderPassBeginInfo renderPassBeginInfo{};
	renderPassBeginInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
	renderPassBeginInfo.renderPass = m_RenderPass;
	renderPassBeginInfo.renderArea.extent.width = pRenderer->GetMainRTWidth();
	renderPassBeginInfo.renderArea.extent.height = pRenderer->GetMainRTHeight();
	renderPassBeginInfo.clearValueCount = 2;
	renderPassBeginInfo.pClearValues = clearValues;

	for (int32_t i = 0; i < m_CmdBuffers.size(); ++i)
	{
		vkBeginCommandBuffer(m_CmdBuffers[i], &cmdBufferBeginInfo);
		renderPassBeginInfo.framebuffer = pRenderer->m_SwapChainFramebuffers[i];
		vkCmdBeginRenderPass(m_CmdBuffers[i], &renderPassBeginInfo, VK_SUBPASS_CONTENTS_INLINE);

		//Record IMGUI commands
		ImGui_ImplGlfwVulkan_Render(m_CmdBuffers[i]);

		vkCmdEndRenderPass(m_CmdBuffers[i]);
		vkEndCommandBuffer(m_CmdBuffers[i]);
	}

}