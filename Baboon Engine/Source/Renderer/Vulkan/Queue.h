#pragma once
#include "Common.h"
#include <vector>

class Device;
class CommandBuffer;
class Queue
{
public:
    Queue(const Device& device, uint32_t family_index, VkQueueFamilyProperties properties, VkBool32 can_present, uint32_t index);
    inline const VkQueueFamilyProperties& getProperties() const { return m_Properties; }
    inline  uint32_t getFamilyIndex() const { return m_FamilyIndex; } 
    VkResult submit(const std::vector<VkSubmitInfo>& submit_infos, VkFence fence) const;
    VkResult submit(const CommandBuffer& command_buffer, VkFence fence) const;
    VkResult present(const VkPresentInfoKHR& present_info) const;

    inline bool canPresent()const { return m_CanPresent; }
private:
  
    const Device& m_Device;
    uint32_t m_Index;
    uint32_t m_FamilyIndex;
    VkQueueFamilyProperties m_Properties; 
    VkBool32 m_CanPresent; 
    
    VkQueue m_Queue{ VK_NULL_HANDLE };
};