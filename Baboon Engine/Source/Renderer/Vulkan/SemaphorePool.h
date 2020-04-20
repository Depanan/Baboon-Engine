#pragma once
#include "Common.h"
#include <vector>
class Device;
class SemaphorePool
{
public:
    SemaphorePool(const Device& device);
    ~SemaphorePool();

    SemaphorePool(const SemaphorePool&) = delete;
    SemaphorePool(SemaphorePool&& other) = default;
    SemaphorePool& operator=(const SemaphorePool&) = delete;
    SemaphorePool& operator=(SemaphorePool&&) = delete;

    void reset();
    uint32_t get_active_semaphore_count() const;
    VkSemaphore request_semaphore();

   
private:
  
    const Device& m_Device;
    std::vector<VkSemaphore> m_Semaphores;

    uint32_t m_Active_Semaphore_Count{ 0 };

};