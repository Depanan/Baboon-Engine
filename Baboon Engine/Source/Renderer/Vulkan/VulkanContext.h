#pragma once
#include "Common.h"
#include <memory>
#include "SwapChain.h"
#include "RenderFrame.h"
#include "CommandBuffer.h"

class Device;


class VulkanContext{

public:
    VulkanContext(Device& device, VkSurfaceKHR surface, uint32_t window_width, uint32_t window_height);
    void checkForSurfaceChanges();
    void prepare(size_t nThreads, RenderTarget::CreateFunc createRenderTargetfunc = RenderTarget::DEFAULT_CREATE_FUNC);
    CommandBuffer& begin(CommandBuffer::ResetMode reset_mode = CommandBuffer::ResetMode::ResetPool);
    void submit(const CommandBuffer& command_buffer);
    void end(VkSemaphore semaphore);
    RenderFrame& getActiveFrame()const;
    bool isFrameActive() { return m_FrameActive; }
    Device& getDevice() const{ return m_DeviceRef; }
    VkExtent2D getSurfaceExtent()const { return m_Surface_extent; }
    RenderFrame& getCurrentFrame() const{ return *m_Frames[m_FrameIndex]; }
    const std::vector<std::unique_ptr<RenderFrame>>& getRenderFrames()const { return m_Frames; }
private:
    VkSemaphore submit(const Queue& queue, const CommandBuffer& command_buffer, VkSemaphore wait_semaphore, VkPipelineStageFlags wait_pipeline_stage);
    std::unique_ptr<SwapChain> m_SwapChain{ nullptr };
    std::vector<std::unique_ptr<RenderFrame>> m_Frames;
    Device& m_DeviceRef;
    const Queue& m_Queue;
    VkExtent2D m_Surface_extent;
    bool m_FrameActive{ false };
    uint32_t m_FrameIndex{ 0 };
    VkSemaphore m_FrameSemaphore;
    RenderTarget::CreateFunc m_CreateRenderTargetFunction = RenderTarget::DEFAULT_CREATE_FUNC;
    void recreate(uint32_t window_width, uint32_t window_height);
    void waitFrame();

};