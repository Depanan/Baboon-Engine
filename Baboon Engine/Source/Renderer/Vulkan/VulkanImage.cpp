#include "VulkanImage.h"
#include "Core\ServiceLocator.h"
#include "Device.h"
#include "VulkanImageView.h"

int32_t findMemoryType(VkPhysicalDevice physDevicce, uint32_t typeFilter, VkMemoryPropertyFlags properties)
{
    VkPhysicalDeviceMemoryProperties memProperties;
    vkGetPhysicalDeviceMemoryProperties(physDevicce, &memProperties);

    //So here we have to check if the bit specified in typeFilter is set to 1, and also that the memorytype flag matches the input param properties, if so thats our mem type
    for (uint32_t i = 0; i < memProperties.memoryTypeCount; i++) {
        if ((typeFilter & (1 << i)) && (memProperties.memoryTypes[i].propertyFlags & properties) == properties) {
            return i;
        }
    }
    throw std::runtime_error("failed to find suitable memory type!");

    return -1;
}

VulkanImage::VulkanImage(const Device& device, VkImage handle, const VkExtent3D& extent, VkFormat format, VkImageUsageFlags image_usage) :
    m_Device{ device },
    m_Handle{ handle },
    m_Extent{ extent},
    m_Format{ format},
    m_Usage{ image_usage}
{
    m_Type = VK_IMAGE_TYPE_2D;
    m_SampleCount = VK_SAMPLE_COUNT_1_BIT;
    m_Subresource.mipLevel = 1;
    m_Subresource.arrayLayer = 1;

}

VulkanImage::VulkanImage(VulkanImage&& other)
    :
    m_Device{ other.m_Device },
    m_Handle{ other.m_Handle },
    m_Extent{ other.m_Extent },
    m_Format{ other.m_Format },
    m_Usage{ other.m_Usage },
    m_Memory{other.m_Memory},
    m_Tiling(other.m_Tiling),
    m_Subresource(other.m_Subresource),
    m_SampleCount(other.m_SampleCount),
    m_Mapped(other.m_Mapped),
    m_MappedData(other.m_MappedData)

{
    other.m_Handle = VK_NULL_HANDLE;
    other.m_Memory = VK_NULL_HANDLE;
    other.m_MappedData = nullptr;
    other.m_Mapped = false;
    for (auto& view : m_Views)
    {
        view->setImage(*this);
    }
}

VulkanImage::~VulkanImage()
{
    if (m_Handle != VK_NULL_HANDLE && m_Memory != VK_NULL_HANDLE)
    {
        unmap();
        vmaDestroyImage(m_Device.getMemoryAllocator(), m_Handle, m_Memory);
    }
    


}

uint8_t* VulkanImage::map()
{
    if (!m_MappedData)
    {
        if (m_Tiling != VK_IMAGE_TILING_LINEAR)
        {
            LOGINFO("Mapping image memory that is not linear");
        }
        VkResult result = vmaMapMemory(m_Device.getMemoryAllocator(), m_Memory, reinterpret_cast<void**>(&m_MappedData));
        if (result != VK_SUCCESS)
        {
            LOGERROR("Error mapping memory for image!!");
        }
        m_Mapped = true;
    }
    return m_MappedData;
}

void VulkanImage::unmap()
{
    if (m_Mapped)
    {
        vmaUnmapMemory(m_Device.getMemoryAllocator(), m_Memory);
        m_MappedData = nullptr;
        m_Mapped = false;
    }
}



VulkanImage::VulkanImage(const Device& device,
                        const VkExtent3D& extent,
                        VkFormat format,
                        VkImageUsageFlags image_usage,
                        VmaMemoryUsage memUsage,
                        VkSampleCountFlagBits sample_count,
                        uint32_t mip_levels,
                        uint32_t array_layers,
                        VkImageTiling tiling,
                        VkImageCreateFlags flags) :
    m_Device{ device },
    m_Extent{ extent },
    m_Format{ format },
    m_Usage{ image_usage },
    m_SampleCount{sample_count},
    m_Tiling(tiling)
{

    m_Type = VK_IMAGE_TYPE_2D;//TODO: Support non 2d as well

    VkImageCreateInfo imageInfo = {};
    imageInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
    imageInfo.imageType = m_Type;
    imageInfo.extent = extent;
    imageInfo.mipLevels = mip_levels;
    imageInfo.arrayLayers = array_layers;
    imageInfo.format = format;
    imageInfo.tiling = tiling;
    imageInfo.usage = image_usage;
    imageInfo.samples = sample_count;


    m_Subresource.mipLevel = imageInfo.mipLevels;
    m_Subresource.arrayLayer = imageInfo.arrayLayers;

    VmaAllocationCreateInfo memory_info{};
    memory_info.usage = memUsage;

    if (image_usage & VK_IMAGE_USAGE_TRANSIENT_ATTACHMENT_BIT)
    {
        memory_info.preferredFlags = VK_MEMORY_PROPERTY_LAZILY_ALLOCATED_BIT;
    }

    auto result = vmaCreateImage(m_Device.getMemoryAllocator(),
        &imageInfo, &memory_info,
        &m_Handle, &m_Memory,
        nullptr);

}


std::unordered_set<VulkanImageView*>& VulkanImage::getViews()
{
    return m_Views;
}