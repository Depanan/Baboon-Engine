#pragma once
#include "../Common.h"

class Device;
class RenderTarget;
class RenderPass;

class FrameBuffer
{
public:
    FrameBuffer(const Device& device, const RenderTarget& render_target, const RenderPass& render_pass);

    FrameBuffer(const FrameBuffer&) = delete;

    FrameBuffer(FrameBuffer&& other);

    ~FrameBuffer();

    FrameBuffer& operator=(const FrameBuffer&) = delete;

    FrameBuffer& operator=(FrameBuffer&&) = delete;


    const VkFramebuffer getHandle()const { return m_Framebuffer; }

private:
    const Device& m_Device;
    VkExtent2D m_Extent{};
    VkFramebuffer m_Framebuffer{ VK_NULL_HANDLE };
};