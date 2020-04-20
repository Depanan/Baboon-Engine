#include "Device.h"
#define VMA_IMPLEMENTATION
#include <vk_mem_alloc.h>
#include <set>
#include "Core\ServiceLocator.h"
#include "CommandPool.h"



const Queue& Device::getQueueByFlags(VkQueueFlags requiredFlags, uint32_t index) const
{
    for (uint32_t famIndex = 0; famIndex < m_Queues.size(); ++famIndex)
    {
        const Queue& firstQueue = m_Queues[famIndex][0];

        VkQueueFlags flags = firstQueue.getProperties().queueFlags;
        uint32_t     count = firstQueue.getProperties().queueCount;

        if (((flags & requiredFlags) == requiredFlags) && index < count)
        {
            return m_Queues[famIndex][index];
        }
    }

    throw std::runtime_error("Queue not found");
}



Device::Device(VkPhysicalDevice physDevice, VkSurfaceKHR surface, const std::vector<const char*> validationLayers, const std::vector<const char*> deviceExtensions)
    :
    m_ResourcesCache(*this)
{
    m_PhysDevice = physDevice;
  


    uint32_t queueFamilyPropertiesCount = 0;//Number of families available in physdevice
    vkGetPhysicalDeviceQueueFamilyProperties(physDevice, &queueFamilyPropertiesCount, nullptr);

    m_QueueFamilyProperties = std::vector<VkQueueFamilyProperties>(queueFamilyPropertiesCount);//Vector to hold per family properties
    vkGetPhysicalDeviceQueueFamilyProperties(physDevice, &queueFamilyPropertiesCount, m_QueueFamilyProperties.data());//fetch each family property and store in vector

    std::vector<VkDeviceQueueCreateInfo> queueCreateInfos(queueFamilyPropertiesCount);//Info to create a queue in the logical device
    std::vector<std::vector<float>>      queuePriorities(queueFamilyPropertiesCount);

    for (uint32_t familyIndex = 0; familyIndex < queueFamilyPropertiesCount; ++familyIndex)
    {
        const VkQueueFamilyProperties& familyProperty = m_QueueFamilyProperties[familyIndex];
        queuePriorities[familyIndex].resize(familyProperty.queueCount, 1.0f);//resize and set priority to 1 to every element

        VkDeviceQueueCreateInfo& createInfo = queueCreateInfos[familyIndex];

        createInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
        createInfo.queueFamilyIndex = familyIndex;
        createInfo.queueCount = familyProperty.queueCount;
        createInfo.pQueuePriorities = queuePriorities[familyIndex].data();
    }

    VkPhysicalDeviceFeatures deviceFeatures = {};

    VkDeviceCreateInfo createInfo = {};
    createInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
    createInfo.pQueueCreateInfos = queueCreateInfos.data();
    createInfo.queueCreateInfoCount = (uint32_t)queueCreateInfos.size();
    createInfo.pEnabledFeatures = &deviceFeatures;

    createInfo.enabledExtensionCount = deviceExtensions.size();
    createInfo.ppEnabledExtensionNames = deviceExtensions.data();

    if (validationLayers.size()) {
        createInfo.enabledLayerCount = validationLayers.size();
        createInfo.ppEnabledLayerNames = validationLayers.data();
    }
    else {
        createInfo.enabledLayerCount = 0;
    }


    if (vkCreateDevice(m_PhysDevice, &createInfo, nullptr, &m_Handle) != VK_SUCCESS) {
        LOGERROR("Cant create Vulkan logical device!");
    }

   

    m_Queues.resize(queueFamilyPropertiesCount);

    for (uint32_t familyIndex = 0; familyIndex < queueFamilyPropertiesCount; ++familyIndex)
    {
        const VkQueueFamilyProperties& familyProperty = m_QueueFamilyProperties[familyIndex];

        VkBool32 present_supported{ VK_FALSE };

        // Only check if surface is valid to allow for headless applications ej compute?
        if (surface != VK_NULL_HANDLE)
        {
           VkResult res = vkGetPhysicalDeviceSurfaceSupportKHR(physDevice, familyIndex, surface, &present_supported);
           if (res != VK_SUCCESS)
               LOGERROR("Device doesn't support surface, error!");
        }

        for (uint32_t queueIndex = 0; queueIndex < familyProperty.queueCount; ++queueIndex)//Create queue instances for this family 
        {
            m_Queues[familyIndex].emplace_back(*this, familyIndex, familyProperty, present_supported, queueIndex);
        }
    }

    VmaAllocatorCreateInfo allocator_info{};
    allocator_info.physicalDevice = m_PhysDevice;
    allocator_info.device = m_Handle;

   

    VkResult result  = vmaCreateAllocator(&allocator_info, &m_MemoryAllocator);
    if (result != VK_SUCCESS)
    {
        LOGERROR("Error creating memory allocator");
    }

    m_CommandPool = std::make_unique<CommandPool>(*this, getQueueByFlags(VK_QUEUE_GRAPHICS_BIT | VK_QUEUE_COMPUTE_BIT, 0).getFamilyIndex()); //We get the first queue with graphics and compute
    m_FencePool = std::make_unique<FencePool>(*this);
}



Device::~Device()
{


  
    m_ResourcesCache.clear();

    m_CommandPool.reset();//Manually reseting the pointer here
    m_FencePool.reset();


    if (m_MemoryAllocator != VK_NULL_HANDLE)
    {
        VmaStats stats;
        vmaCalculateStats(m_MemoryAllocator, &stats);

        LOGINFO("Total device memory leaked: {} bytes.", stats.total.usedBytes);

        vmaDestroyAllocator(m_MemoryAllocator);
    }


    //Finally destroy the logical device
    if (m_Handle != VK_NULL_HANDLE)
    {
        vkDestroyDevice(m_Handle, nullptr);
    }
}

const Queue& Device::getGraphicsQueue() const
{
    for (uint32_t queue_family_index = 0U; queue_family_index < m_Queues.size(); ++queue_family_index)
    {
        const Queue& first_queue = m_Queues[queue_family_index][0];

        uint32_t queue_count = first_queue.getProperties().queueCount;

        if (first_queue.canPresent() && 0 < queue_count)
        {
            return m_Queues[queue_family_index][0];
        }
    }

    return getQueueByFlags(VK_QUEUE_GRAPHICS_BIT, 0);
}

VkResult Device::wait_idle()const
{
  return vkDeviceWaitIdle(m_Handle);
}