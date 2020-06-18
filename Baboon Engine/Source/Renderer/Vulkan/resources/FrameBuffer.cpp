#include "FrameBuffer.h"
#include "../Device.h"
#include "RenderTarget.h"
#include "RenderPass.h"
#include <cassert>

FrameBuffer::FrameBuffer(const Device& device, const RenderTarget& render_target, const RenderPass& render_pass):
    m_Device(device),
    m_Extent{render_target.getExtent()}
{

    std::vector<VkImageView> attachments;
    size_t layers = 1;
    for (auto& view : render_target.getViews())
    {
        size_t viewLayers = view.getSubresourceLayers().layerCount;
        layers = viewLayers > layers ? viewLayers : layers;
        attachments.emplace_back(view.getHandle());
    }


    VkFramebufferCreateInfo create_info{ VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO };

    create_info.renderPass = render_pass.getHandle();
    create_info.attachmentCount = attachments.size();
    create_info.pAttachments = attachments.data();
    create_info.width = m_Extent.width;
    create_info.height = m_Extent.height;
    create_info.layers = layers;

    auto result = vkCreateFramebuffer(m_Device.get_handle(), &create_info, nullptr, &m_Framebuffer);

    assert(result == VK_SUCCESS, "Cant create framebuffer");
}

FrameBuffer::FrameBuffer(FrameBuffer&& other):
    m_Device{ other.m_Device },
    m_Framebuffer{ other.m_Framebuffer },
    m_Extent{ other.m_Extent }
{
    other.m_Framebuffer = VK_NULL_HANDLE;
}

FrameBuffer::~FrameBuffer()
{
    if (m_Framebuffer != VK_NULL_HANDLE)
    {
        vkDestroyFramebuffer(m_Device.get_handle(), m_Framebuffer, nullptr);
    }
}
