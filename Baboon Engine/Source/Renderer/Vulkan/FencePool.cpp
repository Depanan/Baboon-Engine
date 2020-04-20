#include "FencePool.h"
#include "Device.h"
#include "Core\ServiceLocator.h"

FencePool::FencePool(const Device& device) :
    m_Device(device)
   
{

}

VkFence FencePool::request_fence()
{
    // Check if there is an available Fence
    if (m_Active_Fence_Count < m_Fences.size())
    {
        return m_Fences.at(m_Active_Fence_Count++);
    }
    //Otherwise create one and add it to the list

    VkFence fence{ VK_NULL_HANDLE };

    VkFenceCreateInfo create_info{ VK_STRUCTURE_TYPE_FENCE_CREATE_INFO };

    VkResult result = vkCreateFence(m_Device.get_handle(), &create_info, nullptr, &fence);

    if (result != VK_SUCCESS)
    {
        LOGERROR("Cant create Fence!!");
    }

    m_Fences.push_back(fence);

    m_Active_Fence_Count++;

    return fence;
}

VkResult FencePool::wait(uint32_t timeOut) const
{
    if (m_Active_Fence_Count < 1 || m_Fences.empty())
    {
        return VK_SUCCESS;
    }

    return vkWaitForFences(m_Device.get_handle(), m_Active_Fence_Count, m_Fences.data(), true, timeOut);

}
FencePool::~FencePool()
{
    wait();
    reset();

    // Destroy all Fences
    for (VkFence Fence : m_Fences)
    {
        vkDestroyFence(m_Device.get_handle(), Fence, nullptr);
    }

    m_Fences.clear();
}


VkResult FencePool::reset()
{
    if (m_Active_Fence_Count < 1 || m_Fences.empty())
    {
        return VK_SUCCESS;
    }

    VkResult result = vkResetFences(m_Device.get_handle(), m_Active_Fence_Count, m_Fences.data());

    if (result != VK_SUCCESS)
    {
        return result;
    }

    m_Active_Fence_Count = 0;

    return VK_SUCCESS;
}
uint32_t FencePool::get_active_fence_count() const
{
    return m_Active_Fence_Count;
}

