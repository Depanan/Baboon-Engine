#pragma once

#include "Common.h"
#include "resources/Shader.h"
#include "Core/Model.h"

class CommandBuffer;

//Here is where the drawing actually happens!
class VulkanContext;
class Subpass
{
public:
    Subpass(VulkanContext& render_context, ShaderSource&& vertex_shader, ShaderSource&& fragment_shader);
    virtual void prepare() = 0;
    virtual void draw(CommandBuffer& command_buffer) = 0;

    const ShaderSource& getVertexShader() const { return m_VertexShader; }
    const ShaderSource& getFragmentShader() const{ return m_FragmentShader;}

    bool getDisableDepthAttachment() { return m_DisableDepthAttachment; }
    const  std::vector<uint32_t>& getInputAttachments()const { return m_InputAttachments; }
    const  std::vector<uint32_t>& getOutputAttachments()const { return m_OutputAttachments; }

protected:
    ShaderSource m_VertexShader;
    ShaderSource m_FragmentShader;
    VulkanContext& m_RenderContext;

    
    bool m_DisableDepthAttachment{ false };


    /// Default to no input attachments
    std::vector<uint32_t> m_InputAttachments = {};

    /// Default to swapchain output attachment
    std::vector<uint32_t> m_OutputAttachments = { 0 };
};



class Buffer;
class VulkanImage;
class VulkanSampler;
class VulkanImageView;
class Camera;
struct Texture{
    VulkanImage* m_Image;
    VulkanSampler* m_Sampler;
    VulkanImageView* m_View;
};
class TestTriangleSubPass: public Subpass
{
public:
    TestTriangleSubPass(VulkanContext& render_context, ShaderSource&& vertex_shader, ShaderSource&& fragment_shader, const Camera* p_Camera);
    void prepare() override;
    void draw(CommandBuffer& command_buffer) override;

private:
    Buffer* m_TrianglePos;
    Buffer* m_TriangleIndices;
    Buffer* m_UniformBuffer;
    UBO m_TestUBO;
    const Camera* m_Camera;
    Texture m_TestTexture;

};






