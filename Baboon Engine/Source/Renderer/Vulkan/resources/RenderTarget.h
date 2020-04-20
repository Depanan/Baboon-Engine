#pragma once
#include "../Common.h"
#include "../VulkanImage.h"
#include "../VulkanImageView.h"
#include <vector>

class CommandBuffer;
struct Attachment
{
    VkFormat m_Format{ VK_FORMAT_UNDEFINED };

    VkSampleCountFlagBits m_Samples { VK_SAMPLE_COUNT_1_BIT };

    VkImageUsageFlags m_Usage{ VK_IMAGE_USAGE_SAMPLED_BIT };

    VkImageLayout m_InitialLayout{ VK_IMAGE_LAYOUT_UNDEFINED };

    Attachment() = default;

    Attachment(VkFormat format, VkSampleCountFlagBits samples, VkImageUsageFlags usage);
};


class RenderTarget
{
public:
    RenderTarget(std::vector<VulkanImage>&& images);


    RenderTarget(const RenderTarget&) = delete;

    RenderTarget(RenderTarget&&) = delete;

    RenderTarget& operator=(const RenderTarget& other) noexcept = delete;

    RenderTarget& operator=(RenderTarget&& other) noexcept = delete;


    inline const std::vector<Attachment>& getAttachments() const { return m_Attachments; }
    inline const std::vector<VulkanImageView>& getViews() const { return m_ImageViews; }

    void startOfFrameMemoryBarrier(CommandBuffer& commandBuffer);
    void presentFrameMemoryBarrier(CommandBuffer& commandBuffer);

    VkExtent2D getExtent() const { return m_Extent; }
private:
    std::vector<VulkanImage> m_Images;
    std::vector<VulkanImageView> m_ImageViews;
    std::vector<Attachment> m_Attachments;
    VkExtent2D m_Extent;

};