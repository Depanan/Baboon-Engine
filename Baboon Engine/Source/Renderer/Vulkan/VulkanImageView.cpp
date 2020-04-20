#include "VulkanImageView.h"
#include "Core\ServiceLocator.h"
#include "Device.h"
#include <unordered_set>




VulkanImageView::VulkanImageView(VulkanImage& image, VkImageViewType view_type, VkFormat format) :
    m_Image(&image),
    m_Format(format),
    m_Device(image.getDevice())
{
    if (format == VK_FORMAT_UNDEFINED)
    {
        m_Format = format = m_Image->getFormat();
    }

    m_SubresourceRange.levelCount = m_Image->getSubresource().mipLevel;
    m_SubresourceRange.layerCount = m_Image->getSubresource().arrayLayer;

    if (is_depth_only_format(format))
    {
        m_SubresourceRange.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;
    }
    else if (is_depth_stencil_format(format))
    {
        m_SubresourceRange.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT | VK_IMAGE_ASPECT_STENCIL_BIT;
    }
    else
    {
        m_SubresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    }

    VkImageViewCreateInfo view_info{ VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO };
    view_info.image = m_Image->getHandle();
    view_info.viewType = view_type;
    view_info.format = format;
    view_info.subresourceRange = m_SubresourceRange;

    auto result = vkCreateImageView(m_Device.get_handle(), &view_info, nullptr, &m_ImageView);

    if (result != VK_SUCCESS)
    {
        LOGERROR("Cannot create image view!!");
    }

    m_Image->getViews().emplace(this);
}



VulkanImageView::VulkanImageView(VulkanImageView&& other) :
    m_Device{ other.m_Device },
    m_Image{ other.m_Image },
    m_ImageView{ other.m_ImageView },
    m_Format{ other.m_Format },
    m_SubresourceRange{ other.m_SubresourceRange }
{
    // Remove old view from image set and add this new one
    auto& views = m_Image->getViews();
    views.erase(&other);
    views.emplace(this);

    other.m_ImageView = VK_NULL_HANDLE;
}

VulkanImageView::~VulkanImageView()
{
    if (m_ImageView != VK_NULL_HANDLE)
    {
        vkDestroyImageView(m_Device.get_handle(), m_ImageView, nullptr);
    }
}

void VulkanImageView::setImage(VulkanImage& img)
{
    m_Image = &img;
}

VkImageSubresourceLayers VulkanImageView::getSubresourceLayers() const
{
    VkImageSubresourceLayers subresource{};
    subresource.aspectMask = m_SubresourceRange.aspectMask;
    subresource.baseArrayLayer = m_SubresourceRange.baseArrayLayer;
    subresource.layerCount = m_SubresourceRange.layerCount;
    subresource.mipLevel = m_SubresourceRange.baseMipLevel;
    return subresource;
}

