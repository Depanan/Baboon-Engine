#pragma once
#include "../Common.h"
#include <unordered_map>
#include <memory>

class Device;
class DescriptorSetLayout;

/**
 * @brief Manages an array of fixed size VkDescriptorPool and is able to allocate descriptor sets
 */
class DescriptorPool
{
public:
    static const uint32_t MAX_SETS_PER_POOL = 16;

    DescriptorPool(Device& device,
        const DescriptorSetLayout& descriptor_set_layout,
        uint32_t                   pool_size = MAX_SETS_PER_POOL);

    DescriptorPool(DescriptorPool&&) = default;
    ~DescriptorPool();

    DescriptorPool(const DescriptorPool&) = delete;
    DescriptorPool& operator=(const DescriptorPool&) = delete;
    DescriptorPool& operator=(DescriptorPool&&) = delete;

    void reset();
    const DescriptorSetLayout& get_descriptor_set_layout() const;
    void set_descriptor_set_layout(const DescriptorSetLayout& set_layout);
    VkDescriptorSet allocate();
    VkResult free(VkDescriptorSet descriptor_set);

private:
    Device& m_Device;

    const DescriptorSetLayout* m_DescriptorSetLayout{ nullptr };

    // Descriptor pool size
    std::vector<VkDescriptorPoolSize> m_PoolSizes;

    // Number of sets to allocate for each pool
    uint32_t m_PoolMaxSets{ 0 };

    // Total descriptor pools created
    std::vector<VkDescriptorPool> m_Pools;

    // Count sets for each pool
    std::vector<uint32_t> m_PoolSetsCount;

    // Current pool index to allocate descriptor set
    uint32_t m_PoolIndex{ 0 };

    // Map between descriptor set and pool index
    std::unordered_map<VkDescriptorSet, uint32_t> m_SetPoolMapping;

    // Find next pool index or create new pool
    uint32_t find_available_pool(uint32_t pool_index);
};
