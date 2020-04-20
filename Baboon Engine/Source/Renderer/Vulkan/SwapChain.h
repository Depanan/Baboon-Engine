#pragma once
#include "Common.h"
#include <vector>

class Device;
class SwapChain
{
public:
    SwapChain(const Device& device, VkSurfaceKHR surface, uint32_t window_width, uint32_t window_height);
    SwapChain(SwapChain& old_swapchain, const Device& device, VkSurfaceKHR surface, uint32_t window_width, uint32_t window_height);

    VkResult acquire_next_image(uint32_t& image_index, VkSemaphore image_acquired_semaphore, VkFence fence);

    SwapChain(SwapChain&& other);
    ~SwapChain();

    SwapChain& operator=(const SwapChain&) = delete;
    SwapChain& operator=(SwapChain&&) = delete;
    SwapChain(const SwapChain&) = delete;

    VkSurfaceKHR get_surface() { return m_SurfaceReference; }
    const std::vector<VkImage>& get_images() const;


    VkFormat get_format() const{return m_Surface_format.format;}
    VkImageUsageFlags get_image_usage() const { return m_Image_usage;}

    const VkSwapchainKHR& getHandle() const { return m_Handle; }
private:
    VkSwapchainKHR m_Handle{ VK_NULL_HANDLE };
    const Device& m_DeviceReference;
    VkSurfaceKHR m_SurfaceReference;
    std::vector<VkImage> m_SwapChainImages;

    VkSurfaceFormatKHR m_Surface_format{};
    VkImageUsageFlags  m_Image_usage;


};