#pragma once
#include "Common.h"
#include <vector>
#include <limits>

class Device;
class FencePool
{
public:
    FencePool(const Device& device);
    ~FencePool();

    FencePool(const FencePool&) = delete;
    FencePool(FencePool&& other) = default;
    FencePool& operator=(const FencePool&) = delete;
    FencePool& operator=(FencePool&&) = delete;

    VkResult reset();
    uint32_t get_active_fence_count() const;
    VkFence request_fence();
    VkResult wait(uint32_t timeout = (std::numeric_limits<uint32_t>::max)()) const;

private:
  
    const Device& m_Device;
    std::vector<VkFence> m_Fences;

    uint32_t m_Active_Fence_Count{ 0 };

};