#pragma once
#include "Common.h"
#include "VulkanImage.h"

class Device;
class VulkanSampler
{
public:
    VulkanSampler(Device& d, const VkSamplerCreateInfo& info);
    VulkanSampler(const VulkanSampler&) = delete;
    VulkanSampler(VulkanSampler&& sampler);
    ~VulkanSampler();
    VulkanSampler& operator=(const VulkanSampler&) = delete;
    VulkanSampler& operator=(VulkanSampler&&) = delete;

    
    inline VkSampler getHandle() const {return m_Sampler;}

private:
    Device& m_Device;

    VkSampler m_Sampler{ VK_NULL_HANDLE };
};