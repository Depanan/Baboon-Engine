#pragma once
#include "../Common.h"
#include "../VulkanImage.h"
#include "../VulkanImageView.h"
#include <vector>
#include <functional>

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


    using CreateFunc = std::function<std::unique_ptr<RenderTarget>(VulkanImage&&)>;
    static const CreateFunc DEFAULT_CREATE_FUNC;
    static const CreateFunc DEFERRED_CREATE_FUNC;
    static const CreateFunc SHADOWMAP_CREATE_FUNC;

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
    void setOutputAttachments(std::vector<uint32_t>& output);
  
    const std::vector<uint32_t>& getOutputAttachments() const;

    void setInputAttachments(std::vector<uint32_t>& input);

    const std::vector<uint32_t>& getInputAttachments() const;
    
private:
    std::vector<VulkanImage> m_Images;
    std::vector<VulkanImageView> m_ImageViews;
    std::vector<Attachment> m_Attachments;
    VkExtent2D m_Extent;
    std::vector<uint32_t> m_OutputAttachments = { 0 };
    std::vector<uint32_t> m_InputAttachments = { 0 };

};