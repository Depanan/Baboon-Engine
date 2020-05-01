#include "VulkanIMGUI.h"
#include <imgui/imgui.h>
#include "defines.h"
#include "Cameras\CameraManager.h"
#include <GLFW/glfw3.h>
#include <iostream>


#include "../Renderer/Vulkan/Device.h"
#include "../Renderer/Vulkan/resources/PipelineLayout.h"
#include "../Renderer/Vulkan/VulkanContext.h"
#include "../Renderer/Vulkan/VulkanBuffer.h"
#include "../Renderer/Vulkan/CommandBuffer.h"
#include <glm/gtc/matrix_transform.hpp>

//glfW stuff to deal with input
static GLFWwindow* g_Window = NULL;
static double       g_Time = 0.0f;
static bool         g_MousePressed[3] = { false, false, false };
static float        g_MouseWheel = 0.0f;


void VulkanIMGUI_MouseButtonCallback(GLFWwindow*, int button, int action, int /*mods*/)
{
    if (action == GLFW_PRESS && button >= 0 && button < 3)
        g_MousePressed[button] = true;
}

void VulkanIMGUI_ScrollCallback(GLFWwindow*, double /*xoffset*/, double yoffset)
{
    g_MouseWheel += (float)yoffset; // Use fractional mouse wheel, 1.0 unit 5 lines.
}

void VulkanIMGUI_KeyCallback(GLFWwindow*, int key, int, int action, int mods)
{
    ImGuiIO& io = ImGui::GetIO();
    if (action == GLFW_PRESS)
        io.KeysDown[key] = true;
    if (action == GLFW_RELEASE)
        io.KeysDown[key] = false;

    (void)mods; // Modifiers are not reliable across systems
    io.KeyCtrl = io.KeysDown[GLFW_KEY_LEFT_CONTROL] || io.KeysDown[GLFW_KEY_RIGHT_CONTROL];
    io.KeyShift = io.KeysDown[GLFW_KEY_LEFT_SHIFT] || io.KeysDown[GLFW_KEY_RIGHT_SHIFT];
    io.KeyAlt = io.KeysDown[GLFW_KEY_LEFT_ALT] || io.KeysDown[GLFW_KEY_RIGHT_ALT];
    io.KeySuper = io.KeysDown[GLFW_KEY_LEFT_SUPER] || io.KeysDown[GLFW_KEY_RIGHT_SUPER];
}


Font::Font(const std::string& name, float size) :
    name{ name },
    data{ readFile("Fonts/" + name + ".ttf") },
    size{ size }
{
    // Keep ownership of the font data to avoid a double delete
    ImFontConfig font_config{};
    font_config.FontDataOwnedByAtlas = false;

    ImGuiIO& io = ImGui::GetIO();
    handle = io.Fonts->AddFontFromMemoryTTF(data.data(), static_cast<int>(data.size()), size, &font_config);
}


VulkanImGUI::VulkanImGUI(VulkanContext& i_context, RendererVulkan* renderer) :
    m_VulkanContext(i_context),
    m_VulkanRenderer(renderer),
    m_VertexShader {renderer->getShaderSourcePool().getShaderSource("./Shaders/imgui.vert")},
    m_FragmentShader{ renderer->getShaderSourcePool().getShaderSource("./Shaders/imgui.frag") }

{
 
}

VulkanImGUI::~VulkanImGUI()
{
	
}

void VulkanImGUI::Init(GLFWwindow* i_window)
{
	
    g_Window = i_window;
    glfwSetMouseButtonCallback(g_Window, VulkanIMGUI_MouseButtonCallback);
    glfwSetScrollCallback(g_Window, VulkanIMGUI_ScrollCallback);
    glfwSetKeyCallback(g_Window, VulkanIMGUI_KeyCallback);


    auto& device = m_VulkanContext.getDevice();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO();

    // Dimensions
    auto     extent = m_VulkanContext.getSurfaceExtent();
    io.DisplaySize.x = static_cast<float>(extent.width);
    io.DisplaySize.y = static_cast<float>(extent.height);
    io.FontGlobalScale = 1.0f;
    io.DisplayFramebufferScale = ImVec2(1.0f, 1.0f);
    io.ImeWindowHandle = glfwGetWin32Window(i_window);

    //m_Fonts.emplace_back("Roboto-Regular", 1.0f);

    // Create font texture
    unsigned char* font_data;
    int            tex_width, tex_height;
    io.Fonts->GetTexDataAsRGBA32(&font_data, &tex_width, &tex_height);
    size_t upload_size = tex_width * tex_height * 4 * sizeof(char);
    std::vector<glm::highp_uvec4> testVec(tex_height * tex_width, glm::highp_uvec4(0, 0, 1, 1));
   // font_data = (unsigned char*)&testVec[0].x;

    // Create target image for copy
    VkExtent3D font_extent{ tex_width, tex_height, 1u };
    m_FontImage = std::make_unique<VulkanImage>(device, font_extent, VK_FORMAT_R8G8B8A8_UNORM,
        VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT,
        VMA_MEMORY_USAGE_GPU_ONLY);
    m_FontImageView = std::make_unique<VulkanImageView>(*m_FontImage, VK_IMAGE_VIEW_TYPE_2D);


    // Upload font data into the vulkan image memory
    {
        VulkanBuffer stage_buffer{ device, upload_size, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VMA_MEMORY_USAGE_CPU_ONLY, 0 };
        stage_buffer.update({ font_data, font_data + upload_size });

        auto& command_buffer = device.requestCommandBuffer();

        FencePool fence_pool{ device };

        // Begin recording
        command_buffer.begin(VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT, 0);

        {
            // Prepare for transfer
            ImageMemoryBarrier memory_barrier{};
            memory_barrier.old_layout = VK_IMAGE_LAYOUT_UNDEFINED;
            memory_barrier.new_layout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
            memory_barrier.src_access_mask = 0;
            memory_barrier.dst_access_mask = VK_ACCESS_TRANSFER_WRITE_BIT;
            memory_barrier.src_stage_mask = VK_PIPELINE_STAGE_HOST_BIT;
            memory_barrier.dst_stage_mask = VK_PIPELINE_STAGE_TRANSFER_BIT;

            command_buffer.imageBarrier(*m_FontImageView, memory_barrier);
        }

        // Copy
        VkBufferImageCopy buffer_copy_region{};
        buffer_copy_region.imageSubresource.layerCount = m_FontImageView->getSubResourceRange().layerCount;
        buffer_copy_region.imageSubresource.aspectMask = m_FontImageView->getSubResourceRange().aspectMask;
        buffer_copy_region.imageExtent = m_FontImage->getExtent();

        command_buffer.copy_buffer_to_image(stage_buffer, *m_FontImage, { buffer_copy_region });

        {
            // Prepare for fragmen shader
            ImageMemoryBarrier memory_barrier{};
            memory_barrier.old_layout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
            memory_barrier.new_layout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
            memory_barrier.src_access_mask = 0;
            memory_barrier.dst_access_mask = VK_ACCESS_SHADER_READ_BIT;
            memory_barrier.src_stage_mask = VK_PIPELINE_STAGE_TRANSFER_BIT;
            memory_barrier.dst_stage_mask = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;

            command_buffer.imageBarrier(*m_FontImageView, memory_barrier);
        }

        // End recording
        command_buffer.end();

        auto& queue = device.getQueueByFlags(VK_QUEUE_GRAPHICS_BIT, 0);

        queue.submit(command_buffer, device.requestFence());

        // Wait for the command buffer to finish its work before destroying the staging buffer
        device.getFencePool().wait();
        device.getFencePool().reset();
        device.getCommandPool().reset_pool();
    }

    // Create texture sampler
    VkSamplerCreateInfo sampler_info{ VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO };
    sampler_info.maxAnisotropy = 1.0f;
    sampler_info.magFilter = VK_FILTER_LINEAR;
    sampler_info.minFilter = VK_FILTER_LINEAR;
    sampler_info.mipmapMode = VK_SAMPLER_MIPMAP_MODE_NEAREST;
    sampler_info.addressModeU = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
    sampler_info.addressModeV = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
    sampler_info.addressModeW = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
    sampler_info.borderColor = VK_BORDER_COLOR_FLOAT_OPAQUE_WHITE;

   

    

    m_Sampler = std::make_unique<VulkanSampler>(device, sampler_info);

    m_VertexBuffer = std::make_unique<VulkanBuffer>(device, 1, VK_BUFFER_USAGE_VERTEX_BUFFER_BIT, VMA_MEMORY_USAGE_GPU_TO_CPU);
    m_IndexBuffer = std::make_unique<VulkanBuffer>(device, 1, VK_BUFFER_USAGE_INDEX_BUFFER_BIT, VMA_MEMORY_USAGE_GPU_TO_CPU);



}
void VulkanImGUI::OnWindowResize()
{
    m_PersistentCommandsPerFrame.setDirty();
    m_ForceUpdateGeometryBuffers = true;
}


void VulkanImGUI::recordCommandBuffers(CommandBuffer* command_buffer, CommandBuffer* primary_commandBuffer)
{


    auto& device = m_VulkanContext.getDevice();
    auto& renderTarget = m_VulkanContext.getActiveFrame().getRenderTarget();//Grab the render target

    //Set viewport and scissors
    auto& extent = renderTarget.getExtent();
    VkViewport viewport{};
    viewport.width = static_cast<float>(extent.width);
    viewport.height = static_cast<float>(extent.height);
    viewport.minDepth = 0.0f;
    viewport.maxDepth = 1.0f;
    VkRect2D scissor{};
    scissor.extent = extent;



    command_buffer->begin(VK_COMMAND_BUFFER_USAGE_RENDER_PASS_CONTINUE_BIT, primary_commandBuffer);
    command_buffer->setViewport(0, { viewport });
    command_buffer->setScissor(0, { scissor });

    // Vertex input state
    VkVertexInputBindingDescription vertex_input_binding{};
    vertex_input_binding.stride = sizeof(ImDrawVert);

    // Location 0: Position
    VkVertexInputAttributeDescription pos_attr{};
    pos_attr.format = VK_FORMAT_R32G32_SFLOAT;
    pos_attr.offset = offsetof(ImDrawVert, pos);

    // Location 1: UV
    VkVertexInputAttributeDescription uv_attr{};
    uv_attr.location = 1;
    uv_attr.format = VK_FORMAT_R32G32_SFLOAT;
    uv_attr.offset = offsetof(ImDrawVert, uv);

    // Location 2: Color
    VkVertexInputAttributeDescription col_attr{};
    col_attr.location = 2;
    col_attr.format = VK_FORMAT_R8G8B8A8_UNORM;
    col_attr.offset = offsetof(ImDrawVert, col);

    VertexInputState vertex_input_state{};
    vertex_input_state.m_Bindings = { vertex_input_binding };
    vertex_input_state.m_Attributes = { pos_attr, uv_attr, col_attr };

    command_buffer->setVertexInputState(vertex_input_state);

    // Blend state
    ColorBlendAttachmentState color_attachment{};
    color_attachment.m_BlendEnable = VK_TRUE;
    color_attachment.m_ColorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT;
    color_attachment.m_SrcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA;
    color_attachment.m_DstColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
    color_attachment.m_SrcAlphaBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;

    ColorBlendState blend_state;
    blend_state.m_Attachments.push_back(color_attachment);

    command_buffer->setColorBlendState(blend_state);

    RasterizationState rasterization_state{};
    rasterization_state.m_CullMode = VK_CULL_MODE_NONE;
    command_buffer->setRasterState(rasterization_state);

    DepthStencilState depth_state{};
    depth_state.m_DepthTestEnable = VK_FALSE;
    depth_state.m_DepthWriteEnable = VK_FALSE;
    command_buffer->setDepthStencilState(depth_state);

    // Bind pipeline layout
    std::vector<ShaderModule*> shader_modules;
    auto pVertexShader = m_VertexShader.lock();
    if (!pVertexShader)
    {
        m_VertexShader = m_VulkanRenderer->getShaderSourcePool().getShaderSource("./Shaders/imgui.vert");
        pVertexShader = m_VertexShader.lock();
    }
    auto pFragmentShader = m_FragmentShader.lock();
    if (!pFragmentShader)
    {
        m_FragmentShader = m_VulkanRenderer->getShaderSourcePool().getShaderSource("./Shaders/imgui.frag");
        pFragmentShader = m_FragmentShader.lock();
    }
    //TODO: Fix this shader mess
    
    shader_modules.push_back(&device.getResourcesCache().request_shader_module(VK_SHADER_STAGE_VERTEX_BIT, pVertexShader, m_ShaderVariant));
    shader_modules.push_back(&device.getResourcesCache().request_shader_module(VK_SHADER_STAGE_FRAGMENT_BIT, pFragmentShader, m_ShaderVariant));

    auto& pipelineLayout = device.getResourcesCache().request_pipeline_layout(shader_modules);
    command_buffer->bindPipelineLayout(pipelineLayout)   ;

    command_buffer->bind_image(*m_FontImageView, *m_Sampler, 0, 0, 0);

    // Pre-rotation
    auto& io = ImGui::GetIO();
    auto  push_transform = glm::mat4(1.0f);


    // GUI coordinate space to screen space
    push_transform = glm::translate(push_transform, glm::vec3(-1.0f, -1.0f, 0.0f));
    push_transform = glm::scale(push_transform, glm::vec3(2.0f / io.DisplaySize.x, 2.0f / io.DisplaySize.y, 0.0f));

    // Push constants
    command_buffer->pushConstants(0, push_transform);


    std::vector<std::reference_wrapper<VulkanBuffer>> buffers;
    buffers.push_back(*m_VertexBuffer);
    command_buffer->bind_vertex_buffers(0, buffers, { 0 });

    command_buffer->bind_index_buffer(*m_IndexBuffer, 0, VK_INDEX_TYPE_UINT16);


    // Render commands
    ImDrawData* draw_data = ImGui::GetDrawData();
    int32_t     vertex_offset = 0;
    uint32_t    index_offset = 0;

    if (!draw_data || draw_data->CmdListsCount == 0)
    {
        return;
    }

    for (int32_t i = 0; i < draw_data->CmdListsCount; i++)
    {
        const ImDrawList* cmd_list = draw_data->CmdLists[i];
        for (int32_t j = 0; j < cmd_list->CmdBuffer.Size; j++)
        {
            const ImDrawCmd* cmd = &cmd_list->CmdBuffer[j];
            VkRect2D         scissor_rect;
            scissor_rect.offset.x = std::max(static_cast<int32_t>(cmd->ClipRect.x), 0);
            scissor_rect.offset.y = std::max(static_cast<int32_t>(cmd->ClipRect.y), 0);
            scissor_rect.extent.width = static_cast<uint32_t>(cmd->ClipRect.z - cmd->ClipRect.x);
            scissor_rect.extent.height = static_cast<uint32_t>(cmd->ClipRect.w - cmd->ClipRect.y);


            command_buffer->setScissor(0, { scissor_rect });
            command_buffer->draw_indexed(cmd->ElemCount, 1, index_offset, vertex_offset, 0);
            index_offset += cmd->ElemCount;
        }
        vertex_offset += cmd_list->VtxBuffer.Size;
    }

    command_buffer->end();
}

void VulkanImGUI::Draw(CommandBuffer& primary_commandBuffer)
{
    auto& device = m_VulkanContext.getDevice();
    auto& activeFrame = m_VulkanContext.getActiveFrame();

    auto persistentCommands = m_PersistentCommandsPerFrame.getPersistentCommands(activeFrame.getHashId(), device, activeFrame);
    CommandBuffer* command_buffer = persistentCommands->getCommandBuffer();

    if (persistentCommands->getDirty())
    {

        recordCommandBuffers(command_buffer, &primary_commandBuffer);
        persistentCommands->clearDirty();

    }
    primary_commandBuffer.execute_commands(*command_buffer);


    
	
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

  newFrame();
	
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
			ServiceLocator::GetSceneManager()->LoadScene(filePath);
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
	

	

	//ImGui::ShowTestWindow();
	///////////////////////////////////////////////////////


  // Render to generate draw buffers
  ImGui::Render();

	UpdateDrawBuffers();
}




void VulkanImGUI::newFrame()
{
    ImGuiIO& io = ImGui::GetIO();
    // Setup display size (every frame to accommodate for window resizing)
    int w, h;
    int display_w, display_h;
    glfwGetWindowSize(g_Window, &w, &h);
    glfwGetFramebufferSize(g_Window, &display_w, &display_h);
    io.DisplaySize = ImVec2((float)w, (float)h);
    io.DisplayFramebufferScale = ImVec2(w > 0 ? ((float)display_w / w) : 0, h > 0 ? ((float)display_h / h) : 0);

    // Setup time step
    double current_time =  glfwGetTime();
    io.DeltaTime = g_Time > 0.0 ? (float)(current_time - g_Time) : (float)(1.0f/60.0f);
    g_Time = current_time;

    // Setup inputs
    // (we already got mouse wheel, keyboard keys & characters from glfw callbacks polled in glfwPollEvents())
    if (glfwGetWindowAttrib(g_Window, GLFW_FOCUSED))
    {
        double mouse_x, mouse_y;
        glfwGetCursorPos(g_Window, &mouse_x, &mouse_y);
        io.MousePos = ImVec2((float)mouse_x, (float)mouse_y);   // Mouse position in screen coordinates (set to -1,-1 if no mouse / on another screen, etc.)
    }
    else
    {
        io.MousePos = ImVec2(-1,-1);
    }

    for (int i = 0; i < 3; i++)
    {
        io.MouseDown[i] = g_MousePressed[i] || glfwGetMouseButton(g_Window, i) != 0;    // If a mouse press event came, always pass it as "mouse held this frame", so we don't miss click-release events that are shorter than 1 frame.
        g_MousePressed[i] = false;
    }

    io.MouseWheel = g_MouseWheel;
    g_MouseWheel = 0.0f;

    // Hide OS mouse cursor if ImGui is drawing it
    glfwSetInputMode(g_Window, GLFW_CURSOR, io.MouseDrawCursor ? GLFW_CURSOR_HIDDEN : GLFW_CURSOR_NORMAL);

    // Start the frame
    ImGui::NewFrame();
}

void VulkanImGUI::UpdateDrawBuffers()
{
    ImDrawData* draw_data = ImGui::GetDrawData();
    bool        updated = false;

    if (!draw_data)
    {
        return ;
    }

    size_t vertex_buffer_size = draw_data->TotalVtxCount * sizeof(ImDrawVert);
    size_t index_buffer_size = draw_data->TotalIdxCount * sizeof(ImDrawIdx);

    if ((vertex_buffer_size == 0) || (index_buffer_size == 0))
    {
        return ;
    }

    bool bNeedsUpdate = (m_VertexBuffer->getHandle() == VK_NULL_HANDLE) || (vertex_buffer_size != m_LastVertexBufferSize) ||
        (m_IndexBuffer->getHandle() == VK_NULL_HANDLE) || (index_buffer_size != m_LastIndexBufferSize);
    if (bNeedsUpdate)
    {
        auto& device = m_VulkanContext.getDevice();
        m_VulkanContext.getCurrentFrame().getFencePool().wait();//We need to wait to make sure that the Buffers we are about to destroy are not in use by the previous frame command buffer!
        //vkDeviceWaitIdle(device.get_handle());
    }


    if ((m_VertexBuffer->getHandle() == VK_NULL_HANDLE) || (vertex_buffer_size != m_LastVertexBufferSize))
    {
        m_LastVertexBufferSize = vertex_buffer_size;
        updated = true;

        m_VertexBuffer.reset();
        m_VertexBuffer = std::make_unique<VulkanBuffer>(m_VulkanContext.getDevice(), vertex_buffer_size,
            VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
            VMA_MEMORY_USAGE_GPU_TO_CPU);
    }

    if ((m_IndexBuffer->getHandle() == VK_NULL_HANDLE) || (index_buffer_size != m_LastIndexBufferSize))
    {
        m_LastIndexBufferSize = index_buffer_size;
        updated = true;

        m_IndexBuffer.reset();
        m_IndexBuffer = std::make_unique<VulkanBuffer>(m_VulkanContext.getDevice(), index_buffer_size,
            VK_BUFFER_USAGE_INDEX_BUFFER_BIT,
            VMA_MEMORY_USAGE_GPU_TO_CPU);
    }

    if(true)//bNeedsUpdate || m_ForceUpdateGeometryBuffers)//TODO: There is no way to know if IMGUI has changed from frame to frame apparently so for now we need to redo this
    {
        // Upload data
        ImDrawVert* vtx_dst = (ImDrawVert*)m_VertexBuffer->map();
        ImDrawIdx* idx_dst = (ImDrawIdx*)m_IndexBuffer->map();

        for (int n = 0; n < draw_data->CmdListsCount; n++)
        {
            const ImDrawList* cmd_list = draw_data->CmdLists[n];
            memcpy(vtx_dst, cmd_list->VtxBuffer.Data, cmd_list->VtxBuffer.Size * sizeof(ImDrawVert));
            memcpy(idx_dst, cmd_list->IdxBuffer.Data, cmd_list->IdxBuffer.Size * sizeof(ImDrawIdx));
            vtx_dst += cmd_list->VtxBuffer.Size;
            idx_dst += cmd_list->IdxBuffer.Size;
        }

        m_VertexBuffer->flush();
        m_IndexBuffer->flush();

        m_VertexBuffer->unmap();
        m_IndexBuffer->unmap();

        m_PersistentCommandsPerFrame.setDirty();
        m_ForceUpdateGeometryBuffers = false;
    }

}