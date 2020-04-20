#pragma once
#include "Common.h"
#include "VulkanImage.h"

class Device;
class VulkanImageView
{
public:
    VulkanImageView(VulkanImage& image, VkImageViewType view_type, VkFormat format = VK_FORMAT_UNDEFINED);
    VulkanImageView(VulkanImageView&) = delete;
    VulkanImageView(VulkanImageView&& other);
    ~VulkanImageView();

   VulkanImageView& operator=(const VulkanImageView&) = delete;
   VulkanImageView& operator=(VulkanImageView&&) = delete;


   void setImage(VulkanImage& img);

    inline VkImageSubresourceRange getSubResourceRange() { return m_SubresourceRange; }
    inline VkFormat getFormat() const { return m_Format; }
    inline VulkanImage* getImage() { return m_Image; }
    inline VkImageView getHandle() const { return m_ImageView; }

    VkImageSubresourceLayers getSubresourceLayers() const;
   
   
private:
    VkImageView m_ImageView{ VK_NULL_HANDLE };
    VulkanImage* m_Image{ nullptr };
    VkFormat m_Format{};
    VkImageSubresourceRange m_SubresourceRange{};
    const Device& m_Device;
};