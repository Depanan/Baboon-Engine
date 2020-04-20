
#include "DescriptorPool.h"
#include "../Device.h"
#include "Shader.h"
#include "Core/ServiceLocator.h"
#include "DescriptorSetLayout.h"

DescriptorPool::DescriptorPool(Device& device,
    const DescriptorSetLayout& descriptor_set_layout,
    uint32_t                   pool_size) :
    m_Device{ device },
    m_DescriptorSetLayout{ &descriptor_set_layout }
{
    const auto& bindings = descriptor_set_layout.getBindings();

    std::map<VkDescriptorType, std::uint32_t> descriptor_type_counts;

    // Count each type of descriptor set
    for (auto& binding : bindings)
    {
        descriptor_type_counts[binding.descriptorType] += binding.descriptorCount;
    }

    // Allocate pool sizes array
    m_PoolSizes.resize(descriptor_type_counts.size());

    auto pool_size_it = m_PoolSizes.begin();

    // Fill pool size for each descriptor type count multiplied by the pool size
    for (auto& it : descriptor_type_counts)
    {
        pool_size_it->type = it.first;

        pool_size_it->descriptorCount = it.second * pool_size;

        ++pool_size_it;
    }

    m_PoolMaxSets = pool_size;
}

DescriptorPool::~DescriptorPool()
{
    // Destroy all descriptor pools
    for (auto pool : m_Pools)
    {
        vkDestroyDescriptorPool(m_Device.get_handle(), pool, nullptr);
    }
}

void DescriptorPool::reset()
{
    // Reset all descriptor pools
    for (auto pool : m_Pools)
    {
        vkResetDescriptorPool(m_Device.get_handle(), pool, 0);
    }

    // Clear internal tracking of descriptor set allocations
    std::fill(m_PoolSetsCount.begin(), m_PoolSetsCount.end(), 0);
    m_SetPoolMapping.clear();

    // Reset the pool index from which descriptor sets are allocated
    m_PoolIndex = 0;
}

const DescriptorSetLayout& DescriptorPool::get_descriptor_set_layout() const
{
    assert(m_DescriptorSetLayout && "Descriptor set layout is invalid");
    return *m_DescriptorSetLayout;
}

void DescriptorPool::set_descriptor_set_layout(const DescriptorSetLayout& set_layout)
{
    m_DescriptorSetLayout = &set_layout;
}

VkDescriptorSet DescriptorPool::allocate()
{
    m_PoolIndex = find_available_pool(m_PoolIndex);

    // Increment allocated set count for the current pool
    ++m_PoolSetsCount[m_PoolIndex];

    VkDescriptorSetLayout set_layout = get_descriptor_set_layout().getHandle();

    VkDescriptorSetAllocateInfo alloc_info{ VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO };
    alloc_info.descriptorPool = m_Pools[m_PoolIndex];
    alloc_info.descriptorSetCount = 1;
    alloc_info.pSetLayouts = &set_layout;

    VkDescriptorSet handle = VK_NULL_HANDLE;

    // Allocate a new descriptor set from the current pool
    auto result = vkAllocateDescriptorSets(m_Device.get_handle(), &alloc_info, &handle);

    if (result != VK_SUCCESS)
    {
        // Decrement allocated set count for the current pool
        --m_PoolSetsCount[m_PoolIndex];

        return VK_NULL_HANDLE;
    }

    // Store mapping between the descriptor set and the pool
    m_SetPoolMapping.emplace(handle, m_PoolIndex);

    return handle;
}

VkResult DescriptorPool::free(VkDescriptorSet descriptor_set)
{
    // Get the pool index of the descriptor set
    auto it = m_SetPoolMapping.find(descriptor_set);

    if (it == m_SetPoolMapping.end())
    {
        return VK_INCOMPLETE;
    }

    auto desc_pool_index = it->second;

    // Free descriptor set from the pool
    vkFreeDescriptorSets(m_Device.get_handle(), m_Pools[desc_pool_index], 1, &descriptor_set);

    // Remove descriptor set mapping to the pool
    m_SetPoolMapping.erase(it);

    // Decrement allocated set count for the pool
    --m_PoolSetsCount[desc_pool_index];

    // Change the current pool index to use the available pool
    m_PoolIndex = desc_pool_index;

    return VK_SUCCESS;
}

std::uint32_t DescriptorPool::find_available_pool(std::uint32_t search_index)
{
    // Create a new pool
    if (m_Pools.size() <= search_index)
    {
        VkDescriptorPoolCreateInfo create_info{ VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO };

        create_info.poolSizeCount = m_PoolSizes.size();
        create_info.pPoolSizes = m_PoolSizes.data();
        create_info.maxSets = m_PoolMaxSets;

        // We do not set FREE_DESCRIPTOR_SET_BIT as we do not need to free individual descriptor sets
        create_info.flags = 0;

        // Check descriptor set layout and enable the required flags
        /*auto& binding_flags = descriptor_set_layout->get_binding_flags();
        for (auto binding_flag : binding_flags)
        {
            if (binding_flag & VK_DESCRIPTOR_BINDING_UPDATE_AFTER_BIND_BIT_EXT)
            {
                create_info.flags |= VK_DESCRIPTOR_POOL_CREATE_UPDATE_AFTER_BIND_BIT_EXT;
            }
        }*/

        VkDescriptorPool handle = VK_NULL_HANDLE;

        // Create the Vulkan descriptor pool
        auto result = vkCreateDescriptorPool(m_Device.get_handle(), &create_info, nullptr, &handle);

        if (result != VK_SUCCESS)
        {
            return VK_NULL_HANDLE;
        }

        // Store internally the Vulkan handle
        m_Pools.push_back(handle);

        // Add set count for the descriptor pool
        m_PoolSetsCount.push_back(0);

        return search_index;
    }
    else if (m_PoolSetsCount[search_index] < m_PoolMaxSets)
    {
        return search_index;
    }

    // Increment pool index
    return find_available_pool(++search_index);
}