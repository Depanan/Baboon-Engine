#include "Queue.h"
#include "Device.h"

Queue::Queue(const Device& device, uint32_t family_index, VkQueueFamilyProperties properties, VkBool32 can_present, uint32_t index):
    m_Device(device),
    m_FamilyIndex(family_index),
    m_Properties(properties),
    m_CanPresent(can_present),
    m_Index(index)
{
    vkGetDeviceQueue(m_Device.get_handle(), m_FamilyIndex, m_Index, &m_Queue);
    
}

VkResult Queue::submit(const std::vector<VkSubmitInfo>& submit_infos, VkFence fence) const
{
    return vkQueueSubmit(m_Queue, submit_infos.size(), submit_infos.data(), fence);
}

VkResult Queue::submit(const CommandBuffer& command_buffer, VkFence fence) const
{
    VkSubmitInfo submit_info{ VK_STRUCTURE_TYPE_SUBMIT_INFO };
    submit_info.commandBufferCount = 1;
    submit_info.pCommandBuffers = &command_buffer.getHandle();

    return submit({ submit_info }, fence);
}

VkResult Queue::present(const VkPresentInfoKHR& present_info) const
{
    if (!m_CanPresent)
    {
        return VK_ERROR_INCOMPATIBLE_DISPLAY_KHR;
    }

    return vkQueuePresentKHR(m_Queue, &present_info);
}


