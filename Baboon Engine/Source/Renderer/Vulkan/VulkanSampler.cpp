#include "VulkanSampler.h"
#include "Core\ServiceLocator.h"
#include "Device.h"


VulkanSampler::VulkanSampler(Device& d, const VkSamplerCreateInfo& info) :
    m_Device{ d }
{
    VkResult res = vkCreateSampler(m_Device.get_handle(), &info, nullptr, &m_Sampler);
    if (res != VK_SUCCESS)
    {
        LOGERROR("Cannot create sampler!");
    }
}

VulkanSampler::VulkanSampler(VulkanSampler&& other) :
    m_Device{ other.m_Device },
    m_Sampler{ other.m_Sampler }
{
    other.m_Sampler = VK_NULL_HANDLE;
}

VulkanSampler::~VulkanSampler()
{
    if (m_Sampler != VK_NULL_HANDLE)
    {
        vkDestroySampler(m_Device.get_handle(), m_Sampler, nullptr);
    }
}

