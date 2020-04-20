#define NOMINMAX //To avoid windows.h name collision

#include "SwapChain.h"
#include <algorithm>
#include <limits>
#include "Core/ServiceLocator.h"
#include "Core/Logger.h"
#include "Device.h"

struct SwapChainSupportDetails {
    VkSurfaceCapabilitiesKHR capabilities;
    std::vector<VkSurfaceFormatKHR> formats;
    std::vector<VkPresentModeKHR> presentModes;
};

static SwapChainSupportDetails querySwapChainSupport(VkPhysicalDevice device, VkSurfaceKHR surface)
{
    SwapChainSupportDetails details;

    //Capabilities
    vkGetPhysicalDeviceSurfaceCapabilitiesKHR(device, surface, &details.capabilities);

    //Formats
    uint32_t formatCount;
    vkGetPhysicalDeviceSurfaceFormatsKHR(device, surface, &formatCount, nullptr);

    if (formatCount != 0) {
        details.formats.resize(formatCount);
        vkGetPhysicalDeviceSurfaceFormatsKHR(device, surface, &formatCount, details.formats.data());
    }

    //Present modes
    uint32_t presentModeCount;
    vkGetPhysicalDeviceSurfacePresentModesKHR(device, surface, &presentModeCount, nullptr);

    if (presentModeCount != 0) {
        details.presentModes.resize(presentModeCount);
        vkGetPhysicalDeviceSurfacePresentModesKHR(device, surface, &presentModeCount, details.presentModes.data());
    }


    return details;
}
static VkSurfaceFormatKHR chooseSwapSurfaceFormat(const std::vector<VkSurfaceFormatKHR>& availableFormats)
{
    //Best case scenario, if size =1 and format undefined we can choose whatever we want
    if (availableFormats.size() == 1 && availableFormats[0].format == VK_FORMAT_UNDEFINED) {
        return { VK_FORMAT_B8G8R8A8_UNORM, VK_COLOR_SPACE_SRGB_NONLINEAR_KHR };
    }

    for (const auto& availableFormat : availableFormats) {
        if (availableFormat.format == VK_FORMAT_B8G8R8A8_UNORM && availableFormat.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR) {
            return availableFormat;
        }
    }

    //If everything fails we just return the first one available
    return availableFormats[0];

}

static VkPresentModeKHR chooseSwapPresentMode(const std::vector<VkPresentModeKHR> availablePresentModes)
{
    VkPresentModeKHR bestMode = VK_PRESENT_MODE_FIFO_KHR;//Only this guaranteed to exist

    for (const auto& availablePresentMode : availablePresentModes) {
        if (availablePresentMode == VK_PRESENT_MODE_MAILBOX_KHR) {//Triple buffering with less latency the one we are looking for
            return availablePresentMode;
        }
        else if (availablePresentMode == VK_PRESENT_MODE_IMMEDIATE_KHR) {
            bestMode = availablePresentMode;
        }
    }

    return bestMode;
}

static VkExtent2D chooseSwapExtent(const VkSurfaceCapabilitiesKHR& capabilities, int width, int height)
{
    if (capabilities.currentExtent.width != std::numeric_limits<uint32_t>::max()) {
        return capabilities.currentExtent;
    }
    else {


        VkExtent2D actualExtent = { width, height };


        actualExtent.width = std::max(capabilities.minImageExtent.width, std::min(capabilities.maxImageExtent.width, actualExtent.width));
        actualExtent.height = std::max(capabilities.minImageExtent.height, std::min(capabilities.maxImageExtent.height, actualExtent.height));

        return actualExtent;
    }
}
SwapChain::SwapChain(SwapChain&& other) :
    m_DeviceReference{ other.m_DeviceReference },
    m_SurfaceReference{ other.m_SurfaceReference },
    m_Handle{ other.m_Handle },
    m_Image_usage{ std::move(other.m_Image_usage) },
    m_SwapChainImages{ std::move(other.m_SwapChainImages) }
{
    other.m_Handle = VK_NULL_HANDLE;
    other.m_SurfaceReference = VK_NULL_HANDLE;
}

SwapChain::~SwapChain() {
    LOGDEBUG("Deleting Swapchain");
    if (m_Handle != VK_NULL_HANDLE)
    {
        vkDestroySwapchainKHR(m_DeviceReference.get_handle(), m_Handle, nullptr);
    }

}
const std::vector<VkImage>& SwapChain::get_images() const
{
    return m_SwapChainImages;
}
SwapChain::SwapChain(const Device& device, VkSurfaceKHR surface, uint32_t window_width, uint32_t window_height) :
    SwapChain(*this,device,surface,window_width,window_height)
{

}

SwapChain::SwapChain(SwapChain& old_swapchain, const Device& device, VkSurfaceKHR surface, uint32_t window_width, uint32_t window_height):
    m_DeviceReference(device),
    m_SurfaceReference(surface)
{
    LOGDEBUG("Creating Swapchain");

    SwapChainSupportDetails swapChainSupport = querySwapChainSupport(m_DeviceReference.get_physical_device(),m_SurfaceReference);
    VkSurfaceFormatKHR surfaceFormat = chooseSwapSurfaceFormat(swapChainSupport.formats);
    VkPresentModeKHR presentMode = chooseSwapPresentMode(swapChainSupport.presentModes);
    VkExtent2D extent = chooseSwapExtent(swapChainSupport.capabilities, window_width, window_height);



    m_Surface_format = surfaceFormat;
    m_Image_usage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT; //Final render target, might change if doing post processing

    //Number of images in the swapchain, we will try to have +1 the minimum for triple buffering

    uint32_t imageCount = swapChainSupport.capabilities.minImageCount + 1;
    //We clamp if our imagecount is above capabilities
    if (swapChainSupport.capabilities.maxImageCount > 0 && imageCount > swapChainSupport.capabilities.maxImageCount) {
        imageCount = swapChainSupport.capabilities.maxImageCount;
    }
    VkSwapchainCreateInfoKHR createInfo = {};
    createInfo.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
    createInfo.surface = m_SurfaceReference;
    createInfo.minImageCount = imageCount;
    createInfo.imageFormat = surfaceFormat.format;
    createInfo.imageColorSpace = surfaceFormat.colorSpace;
    createInfo.imageExtent = extent;
    createInfo.imageArrayLayers = 1;//One unless doing stereoscopic stuff 
    createInfo.imageUsage = m_Image_usage;




    //The example doesn't seem to need this bit
    /*QueueFamilyIndices indices = findQueueFamilies(m_PhysicalDevice);
    uint32_t queueFamilyIndices[] = { (uint32_t)indices.graphicsFamily, (uint32_t)indices.presentFamily };

    if (indices.graphicsFamily != indices.presentFamily) { //If they are different we use concurrent. Less performance but more simple to handle
        createInfo.imageSharingMode = VK_SHARING_MODE_CONCURRENT;
        createInfo.queueFamilyIndexCount = 2;
        createInfo.pQueueFamilyIndices = queueFamilyIndices;
    }
    else {
        createInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
        createInfo.queueFamilyIndexCount = 0; // Optional
        createInfo.pQueueFamilyIndices = nullptr; // Optional
    }
    */
    createInfo.preTransform = swapChainSupport.capabilities.currentTransform;//Pixel transformation for the whole render target. Current transform means none 
    createInfo.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;//No alpha blending
    createInfo.presentMode = presentMode;
    createInfo.clipped = VK_TRUE;


    createInfo.oldSwapchain = old_swapchain.m_Handle;

    if (vkCreateSwapchainKHR(m_DeviceReference.get_handle(), &createInfo, nullptr, &m_Handle) != VK_SUCCESS) {
        LOGERROR("Cannot create swap chain");
    }



    //Retrieving images from swapchain for use during rendering
    vkGetSwapchainImagesKHR(m_DeviceReference.get_handle(), m_Handle, &imageCount, nullptr);
    m_SwapChainImages.resize(imageCount);
    vkGetSwapchainImagesKHR(m_DeviceReference.get_handle(), m_Handle, &imageCount, m_SwapChainImages.data());
    

    //m_SwapChainImageFormat = surfaceFormat.format;
    //m_SwapChainExtent = extent;



}


VkResult SwapChain::acquire_next_image(uint32_t& image_index, VkSemaphore image_acquired_semaphore, VkFence fence)
{
    return vkAcquireNextImageKHR(m_DeviceReference.get_handle(), m_Handle, std::numeric_limits<uint64_t>::max(), image_acquired_semaphore, fence, &image_index);
}
