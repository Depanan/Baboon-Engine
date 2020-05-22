#include "RenderTarget.h"
#include "../CommandBuffer.h"
#include <cassert>


const RenderTarget::CreateFunc RenderTarget::DEFAULT_CREATE_FUNC = [](VulkanImage&& swapChainImage) -> std::unique_ptr<RenderTarget> {
   

    VulkanImage depth_image{ swapChainImage.getDevice(), swapChainImage.getExtent(),
                         VK_FORMAT_D32_SFLOAT,
                         VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT | VK_IMAGE_USAGE_TRANSIENT_ATTACHMENT_BIT,
                         VMA_MEMORY_USAGE_GPU_ONLY };

    std::vector<VulkanImage> images;
    images.push_back(std::move(swapChainImage));
    images.push_back(std::move(depth_image));

    return  std::make_unique<RenderTarget>(std::move(images));

   
};
const RenderTarget::CreateFunc RenderTarget::DEFERRED_CREATE_FUNC = [](VulkanImage&& swapChainImage) -> std::unique_ptr<RenderTarget> {


    const VkFormat          albedo_format{ VK_FORMAT_R8G8B8A8_UNORM };
    const VkFormat          normal_format{ VK_FORMAT_A2B10G10R10_UNORM_PACK32 };
    const VkImageUsageFlags rt_usage_flags{ VK_IMAGE_USAGE_INPUT_ATTACHMENT_BIT | VK_IMAGE_USAGE_TRANSIENT_ATTACHMENT_BIT };


    auto& device = swapChainImage.getDevice();
    auto& extent = swapChainImage.getExtent();

    // G-Buffer should fit 128-bit budget for buffer color storage
    // in order to enable subpasses merging by the driver
    // Light (swapchain_image) RGBA8_UNORM   (32-bit)
    // Albedo                  RGBA8_UNORM   (32-bit)
    // Normal                  RGB10A2_UNORM (32-bit)

    VulkanImage depth_image{ device,
                                 extent,
                                 VK_FORMAT_D32_SFLOAT,
                                 VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT | rt_usage_flags,
                                 VMA_MEMORY_USAGE_GPU_ONLY };

    VulkanImage albedo_image{ device,
                                  extent,
                                  albedo_format,
                                  VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | rt_usage_flags,
                                  VMA_MEMORY_USAGE_GPU_ONLY };

    VulkanImage normal_image{ device,
                                  extent,
                                  normal_format,
                                  VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | rt_usage_flags,
                                  VMA_MEMORY_USAGE_GPU_ONLY };

    std::vector<VulkanImage> images;

    // Attachment 0
    images.push_back(std::move(swapChainImage));

    // Attachment 1
    images.push_back(std::move(depth_image));

    // Attachment 2
    images.push_back(std::move(albedo_image));

    // Attachment 3
    images.push_back(std::move(normal_image));

    return std::make_unique<RenderTarget>(std::move(images));


};



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

    //Attachments other than depth
    {
        ImageMemoryBarrier memory_barrier{};
        memory_barrier.old_layout = VK_IMAGE_LAYOUT_UNDEFINED;
        memory_barrier.new_layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
        memory_barrier.src_stage_mask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
        memory_barrier.dst_stage_mask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
        memory_barrier.src_access_mask = 0;
        memory_barrier.dst_access_mask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
        commandBuffer.imageBarrier(m_ImageViews.at(0), memory_barrier);
        for (size_t i = 2; i < m_ImageViews.size(); ++i)
        {
            commandBuffer.imageBarrier(m_ImageViews.at(i), memory_barrier);
        }
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


void RenderTarget::setOutputAttachments(std::vector<uint32_t>& output)
{
    m_OutputAttachments = output;
}

const std::vector<uint32_t>& RenderTarget::getOutputAttachments() const
{
    return m_OutputAttachments;
}
void RenderTarget::setInputAttachments(std::vector<uint32_t>& input)
{
    m_InputAttachments = input;
}

const std::vector<uint32_t>& RenderTarget::getInputAttachments() const
{
    return m_InputAttachments;
}