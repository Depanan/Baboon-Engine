#pragma once
#include <vk_mem_alloc.h>
#include "Common.h"
#include <vector>
#include "CommandPool.h"
#include "FencePool.h"
#include "Queue.h"
#include <memory>
#include "VulkanResources.h"


class Device
{
public:
    Device(VkPhysicalDevice physDevice, VkSurfaceKHR surface, const std::vector<const char*> validationLayers, const std::vector<const char*> deviceExtensions, std::chrono::duration<int, std::milli> garbageCollectorInterval = std::chrono::milliseconds(60000));
    VkDevice get_handle() const { return m_Handle; }
    const Queue& getQueueByFlags(VkQueueFlags requiredFlags, uint32_t index) const;
    ~Device();

    Device& operator=(const Device&) = delete;
    Device& operator=(Device&&) = delete;
    Device(const Device&) = delete;
    Device(Device&&) = delete;
    const Queue& getGraphicsQueue() const;
    inline VkPhysicalDevice Device::get_physical_device() const{ return m_PhysDevice; }
    VkResult wait_idle() const;

    inline VulkanResources& getResourcesCache() { return m_ResourcesCache; }
    const inline VmaAllocator& getMemoryAllocator() const { return m_MemoryAllocator; }

    CommandBuffer& Device::requestCommandBuffer() { return m_CommandPool->request_command_buffer(); }
    CommandPool& Device::getCommandPool() { return *m_CommandPool; }
    VkFence Device::requestFence() { return m_FencePool->request_fence(); }
    FencePool& Device::getFencePool(){return *m_FencePool;}

private:
    VkPhysicalDevice m_PhysDevice{ VK_NULL_HANDLE }; //The physical device this logical device belongs to (GPU most likely)
    VkDevice m_Handle{ VK_NULL_HANDLE };

    //VkQueue m_graphicsQueue{ VK_NULL_HANDLE };//This 2 queues will likely be the same in common gpus, TODO: Make Queue class to wrap their functionality (Submit and stuff)
    //VkQueue m_PresentQueue{ VK_NULL_HANDLE };

   // QueueFamilyIndices m_QueueFamilyIndices;

    std::vector<VkQueueFamilyProperties> m_QueueFamilyProperties;
    std::vector<std::vector<Queue>> m_Queues;

    std::unique_ptr<CommandPool> m_CommandPool;
    std::unique_ptr<FencePool> m_FencePool;

    VulkanResources m_ResourcesCache;
    VmaAllocator m_MemoryAllocator{ VK_NULL_HANDLE };

};