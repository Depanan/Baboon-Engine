#include "RenderTarget.h"
#include "../CommandBuffer.h"
#include <cassert>

Attachment::Attachment(VkFormat format, VkSampleCountFlagBits samples, VkImageUsageFlags usage)
    :m_Format(format),m_Samples(samples),m_Usage(usage)
{

}

RenderTarget::RenderTarget(std::vector<VulkanImage>&& images) :
    m_Images(std::move(images))
{
    VkExtent3D extent3d = m_Images.front().getExtent();
    m_Extent = VkExtent2D{ extent3d.width,extent3d.height };
    for (auto& image : m_Images)
    {
        m_ImageViews.emplace_back(image, VK_IMAGE_VIEW_TYPE_2D);
        m_Attachments.emplace_back(Attachment{ image.getFormat(), image.getSampleCount(), image.getUsage() });
        VkExtent3D currentExtent = image.getExtent();
        assert(m_Extent.width == currentExtent.width && m_Extent.height == currentExtent.height, "Different extents on RT not supported!");
    }

    
}


void RenderTarget::startOfFrameMemoryBarrier(CommandBuffer& commandBuffer)
{
    //COLOR
    {
    ImageMemoryBarrier memory_barrier{};
    memory_barrier.old_layout = VK_IMAGE_LAYOUT_UNDEFINED;
    memory_barrier.new_layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
    memory_barrier.src_stage_mask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    memory_barrier.dst_stage_mask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    memory_barrier.src_access_mask = 0;
    memory_barrier.dst_access_mask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
    commandBuffer.imageBarrier(m_ImageViews.at(0), memory_barrier);
    }
    //DEPTH
    {
    ImageMemoryBarrier memory_barrier{};
    memory_barrier.old_layout = VK_IMAGE_LAYOUT_UNDEFINED;
    memory_barrier.new_layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
    memory_barrier.src_access_mask = 0;
    memory_barrier.dst_access_mask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
    memory_barrier.src_stage_mask = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
    memory_barrier.dst_stage_mask = VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT | VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT;

    commandBuffer.imageBarrier(m_ImageViews.at(1), memory_barrier);
    }
}

void RenderTarget::presentFrameMemoryBarrier(CommandBuffer& commandBuffer)
{
    //WE ONLY CARE ABOUT COLOR TO PRESENT
    ImageMemoryBarrier memory_barrier{};
    memory_barrier.old_layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
    memory_barrier.new_layout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
    memory_barrier.src_access_mask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
    memory_barrier.src_stage_mask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    memory_barrier.dst_stage_mask = VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT;

    commandBuffer.imageBarrier(m_ImageViews.at(0), memory_barrier);
}