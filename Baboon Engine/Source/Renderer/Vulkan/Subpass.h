#pragma once

#include "Common.h"
#include "resources/Shader.h"
#include "Core/Model.h"
#include <unordered_map>
#include <memory>
#include "PersistentCommand.h"
#include <Core/Scene.h>
#include <mutex>

//#include "Core/ThreadPool.hpp"
class CommandBuffer;

//Here is where the drawing actually happens!
class VulkanContext;
class PersistentCommandsPerFrame;
class ThreadPool;
class Subpass
{
public:
    Subpass(VulkanContext& render_context, std::string vertex_shader, std::string fragment_shader);
    virtual ~Subpass();
    virtual void prepare() = 0;
    virtual void draw(CommandBuffer& command_buffer) = 0;

    //const ShaderSource& getVertexShader() const { return *m_VertexShader; }
    //const ShaderSource& getFragmentShader() const{ return *m_FragmentShader;}

    bool getDisableDepthAttachment() { return m_DisableDepthAttachment; }
    void setDisableDepthAttachment() { m_DisableDepthAttachment = true; }
    const  std::vector<uint32_t>& getInputAttachments()const { return m_InputAttachments; }
    const  std::vector<uint32_t>& getOutputAttachments()const { return m_OutputAttachments; }
    void setOutputAttachments(std::vector<uint32_t> attachments) { m_OutputAttachments = attachments; }
    void setInputAttachments(std::vector<uint32_t> attachments) { m_InputAttachments = attachments; }

    void updateRenderTargetAttachments();

    void invalidatePersistentCommands();
    void setReRecordCommands();


   
protected:
    std::weak_ptr<ShaderSource> m_VertexShader;
    std::string m_VertexShaderPath;
    std::weak_ptr<ShaderSource> m_FragmentShader;
    std::string m_FragmentShaderPath;



    VulkanContext& m_RenderContext;

    PersistentCommandsPerFrame m_PersistentCommandsPerFrame;

    bool m_DisableDepthAttachment{ false };


    /// Default to no input attachments
    std::vector<uint32_t> m_InputAttachments = {};

    /// Default to swapchain output attachment
    std::vector<uint32_t> m_OutputAttachments = { 0 };

    ThreadPool* m_ThreadPool;

    std::shared_ptr<ShaderSource> getVertexShader();
    std::shared_ptr<ShaderSource> getFragmentShader();


    void drawBatchList(std::vector<RenderBatch>& batches, CommandBuffer* primary_command_buffer, std::vector<CommandBuffer*>& commands);
    void recordBatches(CommandBuffer* commandBuffer, CommandBuffer* primary_command_buffer, std::vector<RenderBatch>& batches, size_t beginIndex, size_t endIndex);
    void recordCommandBuffers(std::vector<CommandBuffer*> commandBuffers, CommandBuffer* primary_command_buffer, std::vector<RenderBatch>& batches, size_t beginIndex, size_t endIndex);
    void drawModel(const Model& model, CommandBuffer* commandBuffer);
};



class VulkanBuffer;
class VulkanTexture;
class Camera;
class CommandPool;




class GeometrySubpass: public Subpass
{
public:
    GeometrySubpass(VulkanContext& render_context, std::string vertex_shader, std::string fragment_shader, size_t nThreads = 1);
    void prepare() override;
    void draw(CommandBuffer& command_buffer) override;

    
    
};

class LightSubpass : public Subpass
{
public:
    LightSubpass(VulkanContext& render_context,  std::string vertex_shader, std::string fragment_shader);
    void prepare() override {}
    void draw(CommandBuffer& command_buffer) override;
};

class TransparentSubpass : public Subpass
{
public:
    TransparentSubpass(VulkanContext& render_context, std::string vertex_shader, std::string fragment_shader, size_t nThreads = 1);
    void prepare() override;
    void draw(CommandBuffer& command_buffer) override;


};







