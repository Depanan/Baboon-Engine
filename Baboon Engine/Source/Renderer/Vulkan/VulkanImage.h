#pragma once
#include "Common.h"
#include <unordered_set>
#include <vk_mem_alloc.h>

class Device;
class VulkanImageView;
class VulkanImage
{
public:
    VulkanImage(const Device& device, 
        const VkExtent3D& extent,
        VkFormat format, 
        VkImageUsageFlags image_usage,
        VmaMemoryUsage memUsage, 
        VkSampleCountFlagBits sample_count = VK_SAMPLE_COUNT_1_BIT, 
        uint32_t mip_levels = 1, 
        uint32_t array_layers = 1,
        VkImageTiling tiling = VK_IMAGE_TILING_OPTIMAL,
        VkImageCreateFlags    flags = 0);
    
    
    VulkanImage(const Device& device, VkImage handle, const VkExtent3D& extent, VkFormat format, VkImageUsageFlags image_usage);


    VulkanImage(const VulkanImage&) = delete;
    VulkanImage(VulkanImage&& other);
    ~VulkanImage();
    VulkanImage& operator=(VulkanImage &) = delete;
    VulkanImage& operator=(VulkanImage&&) = delete;


    uint8_t* map();
    void unmap();

    inline const Device& getDevice() { return m_Device; }
    inline VkImage getHandle() const{ return m_Handle; }
    inline VkImageSubresource getSubresource() { return m_Subresource; }
    inline VkFormat getFormat() { return m_Format; }
    inline VkImageUsageFlags getUsage() { return m_Usage; }
    inline VkSampleCountFlagBits getSampleCount() { return m_SampleCount; }
    inline VkExtent3D getExtent(){ return m_Extent; }
    std::unordered_set<VulkanImageView*>& getViews();
private:
  
    VkImage m_Handle{ VK_NULL_HANDLE };
    VmaAllocation m_Memory{ VK_NULL_HANDLE };

    uint8_t* m_MappedData{ nullptr };
    bool m_Mapped{ false };

    const Device& m_Device;
    VkImageType m_Type{};
    VkExtent3D m_Extent{};
    VkFormat m_Format{};
    VkImageUsageFlags m_Usage{};
    VkImageSubresource m_Subresource{};
    VkSampleCountFlagBits m_SampleCount{};
    VkImageTiling m_Tiling{};

    /// Image views referring to this image
    std::unordered_set<VulkanImageView*> m_Views;
};