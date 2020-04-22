#pragma once
#include "../Renderer/Vulkan/Common.h"
#include "../Renderer/Vulkan/VulkanImage.h"
#include "../Renderer/Vulkan/VulkanImageView.h"
#include "../Renderer/Vulkan/VulkanSampler.h"
#include "../Renderer/Vulkan/Buffer.h"
#include "../Renderer/Vulkan/resources/Shader.h"

#include <memory>

class VulkanContext;
class PipelineLayout;
class CommandBuffer;
class PersistentCommandsPerFrame;


struct ImFont;
class Font
{
public:
    /**
     * @brief Constructor
     * @param name The name of the font file that exists within 'assets/fonts' (without extension)
     * @param size The font size, scaled by DPI
     */
    Font(const std::string& name, float size);

    ImFont* handle{ nullptr };

    std::string name;

    std::vector<uint32_t> data;

    float size{};
};

struct GLFWwindow;

class VulkanImGUI
{
public:
	~VulkanImGUI();

  VulkanImGUI(VulkanContext& i_context);
	void Init(GLFWwindow* i_window);
	void DoUI(bool i_FirstCall = false);
	void Draw(CommandBuffer& m_Command);
	void OnWindowResize();
private:
	
  void recordCommandBuffers(CommandBuffer* command_buffer, CommandBuffer* primary_commandBuffer);
  void newFrame();
	
	void UpdateDrawBuffers();


	void RenderStatsWindow(bool* pOpen);

	float m_UpdateTimer = 0.0f;

  VulkanContext& m_VulkanContext;
  std::unique_ptr<VulkanImage> m_FontImage;
  std::unique_ptr<VulkanImageView> m_FontImageView;

  std::unique_ptr<VulkanSampler> m_Sampler{ nullptr };

  PipelineLayout* m_PipelineLayout{ nullptr };

  std::unique_ptr<Buffer> m_VertexBuffer;
  std::unique_ptr<Buffer> m_IndexBuffer;

  size_t m_LastVertexBufferSize;
  size_t m_LastIndexBufferSize;

  ShaderSource m_VertexShader;
  ShaderSource m_FragmentShader;

  std::vector<Font> m_Fonts;

  bool m_ForceUpdateGeometryBuffers{ false };
  PersistentCommandsPerFrame* m_PersistentCommandsPerFrame;
};
