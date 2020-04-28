#pragma once

#include "Common.h"
#include "resources/Shader.h"
#include "Core/Model.h"
#include <unordered_map>
#include <memory>

class CommandBuffer;

//Here is where the drawing actually happens!
class VulkanContext;
class PersistentCommandsPerFrame;

class Subpass
{
public:
    Subpass(VulkanContext& render_context, std::weak_ptr<ShaderSource> vertex_shader, std::weak_ptr <ShaderSource> fragment_shader);
    virtual void prepare() = 0;
    virtual void draw(CommandBuffer& command_buffer) = 0;

    //const ShaderSource& getVertexShader() const { return *m_VertexShader; }
    //const ShaderSource& getFragmentShader() const{ return *m_FragmentShader;}

    bool getDisableDepthAttachment() { return m_DisableDepthAttachment; }
    const  std::vector<uint32_t>& getInputAttachments()const { return m_InputAttachments; }
    const  std::vector<uint32_t>& getOutputAttachments()const { return m_OutputAttachments; }


    void invalidatePersistentCommands();
    void setReRecordCommands();
protected:
    std::weak_ptr<ShaderSource> m_VertexShader;
    std::weak_ptr<ShaderSource> m_FragmentShader;
    VulkanContext& m_RenderContext;

    PersistentCommandsPerFrame* m_PersistentCommandsPerFrame;

    bool m_DisableDepthAttachment{ false };


    /// Default to no input attachments
    std::vector<uint32_t> m_InputAttachments = {};

    /// Default to swapchain output attachment
    std::vector<uint32_t> m_OutputAttachments = { 0 };
};



class VulkanBuffer;
class VulkanTexture;
class Camera;
class CommandPool;




class TestTriangleSubPass: public Subpass
{
public:
    TestTriangleSubPass(VulkanContext& render_context, std::weak_ptr<ShaderSource> vertex_shader, std::weak_ptr<ShaderSource> fragment_shader, const Camera* p_Camera);
    void prepare() override;
    void draw(CommandBuffer& command_buffer) override;

private:
    
    const Camera* m_Camera;

    //VulkanTexture* m_TestTexture;
    
    void recordCommandBuffers(CommandBuffer* commandBuffer, CommandBuffer* primary_command_buffer);
    void drawModel(Model& model, CommandBuffer* commandBuffer);
};






