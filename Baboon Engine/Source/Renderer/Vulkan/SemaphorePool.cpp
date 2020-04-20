#include "SemaphorePool.h"
#include "Device.h"
#include "Core\ServiceLocator.h"

SemaphorePool::SemaphorePool(const Device& device) :
    m_Device(device)
   
{

}

VkSemaphore SemaphorePool::request_semaphore()
{
    // Check if there is an available semaphore
    if (m_Active_Semaphore_Count < m_Semaphores.size())
    {
        return m_Semaphores.at(m_Active_Semaphore_Count++);
    }
    //Otherwise create one and add it to the list

    VkSemaphore semaphore{ VK_NULL_HANDLE };

    VkSemaphoreCreateInfo create_info{ VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO };

    VkResult result = vkCreateSemaphore(m_Device.get_handle(), &create_info, nullptr, &semaphore);

    if (result != VK_SUCCESS)
    {
        LOGERROR("Cant create semaphore!!");
    }

    m_Semaphores.push_back(semaphore);

    m_Active_Semaphore_Count++;

    return semaphore;
}
SemaphorePool::~SemaphorePool()
{
    reset();

    // Destroy all semaphores
    for (VkSemaphore semaphore : m_Semaphores)
    {
        vkDestroySemaphore(m_Device.get_handle(), semaphore, nullptr);
    }

    m_Semaphores.clear();
}


void SemaphorePool::reset()
{
    m_Active_Semaphore_Count = 0;
}
uint32_t SemaphorePool::get_active_semaphore_count() const
{
    return m_Active_Semaphore_Count;
}

