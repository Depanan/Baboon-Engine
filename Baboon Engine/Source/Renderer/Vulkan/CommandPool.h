#pragma once
#include "Common.h"
#include "CommandBuffer.h"
#include <memory>
#include <vector>
class Device;
class RenderFrame;

class CommandPool
{
public:
    CommandPool(Device& device, uint32_t queue_family_index, RenderFrame* render_frame = nullptr,
        size_t                   thread_index = 0
        ,CommandBuffer::ResetMode reset_mode = CommandBuffer::ResetMode::ResetPool);
    ~CommandPool();
    CommandPool(CommandPool&& other);

    CommandPool(const CommandPool&) = delete;
    CommandPool& operator=(const CommandPool&) = delete;
    CommandPool& operator=(CommandPool&&) = delete;


    inline   Device& getDevice() const{ return m_Device; }
    inline const VkCommandPool& getHandle() const{ return m_CommandPool; }
    VkResult reset_pool();
    VkResult resetCommandBuffers();
    inline CommandBuffer::ResetMode getResetMode() const { return m_ResetMode; }
    CommandBuffer& request_command_buffer(VkCommandBufferLevel level = VK_COMMAND_BUFFER_LEVEL_PRIMARY);
    size_t getThreadIndex() { return m_threadIndex; }
    RenderFrame* getRenderFrame() { return m_RenderFrame; }
    void setRenderFrame(RenderFrame* frame) { m_RenderFrame = frame; }

private:
    CommandBuffer::ResetMode m_ResetMode;
    size_t m_threadIndex;

    std::vector<std::unique_ptr<CommandBuffer>> m_PrimaryCommandBuffers;
    uint32_t m_ActivePrimaryCBCount{ 0 };

    std::vector<std::unique_ptr<CommandBuffer>> m_SecondaryCommandBuffers;
    uint32_t m_ActiveSecondaryCBCount{ 0 };

    Device& m_Device;
    VkCommandPool m_CommandPool{ VK_NULL_HANDLE };

    RenderFrame* m_RenderFrame{ nullptr };
};



struct PersistentCommands {
    bool m_NeedsSecondaryCommandsRecording{ true };
    CommandPool* m_PersistentCommandPoolsPerFrame{ nullptr };
    CommandBuffer* m_PersistentCommandsPerFrame{ nullptr };
};
class PersistentCommandsPerFrame
{
public:
    PersistentCommands* getPersistentCommands(const char* frameId, Device& device, RenderFrame& renderFrame);
    void setDirty();
    void getDirty(const char* frameId);

private:
    std::unordered_map < const char*, PersistentCommands*> m_PersistentCommandsPerFrame;
};