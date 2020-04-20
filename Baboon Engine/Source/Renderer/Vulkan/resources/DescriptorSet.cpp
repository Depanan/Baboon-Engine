
#include "DescriptorSet.h"
#include "DescriptorPool.h"
#include "Core/ServiceLocator.h"
#include "../Device.h"
#include "DescriptorSetLayout.h"

DescriptorSet::DescriptorSet(Device& device,
    DescriptorSetLayout& descriptor_set_layout,
    DescriptorPool& descriptor_pool,
    const BindingMap<VkDescriptorBufferInfo>& buffer_infos,
    const BindingMap<VkDescriptorImageInfo>& image_infos) :
    m_Device{ device },
    m_DescriptorSetLayout{ descriptor_set_layout },
    m_DescriptorPool{ descriptor_pool },
    m_BufferInfos{ buffer_infos },
    m_ImageInfos{ image_infos },
    m_DescriptorSet{ descriptor_pool.allocate() }
{
    prepare();
}

void DescriptorSet::reset(const BindingMap<VkDescriptorBufferInfo>& new_buffer_infos, const BindingMap<VkDescriptorImageInfo>& new_image_infos)
{
    if (!new_buffer_infos.empty() || !new_image_infos.empty())
    {
        m_BufferInfos = new_buffer_infos;
        m_ImageInfos = new_image_infos;
    }
    else
    {
        LOGERROR("Calling reset on Descriptor Set with no new buffer infos and no new image infos.");
    }

    this->m_WriteDescriptorSets.clear();
    this->m_UpdatedBindings.clear();

    prepare();
}

void DescriptorSet::prepare()
{
    // We don't want to prepare twice during the life cycle of a Descriptor Set
    if (!m_WriteDescriptorSets.empty())
    {
        LOGERROR("Trying to prepare a Descriptor Set that has already been prepared, skipping.");
        return;
    }

    // Iterate over all buffer bindings
    for (auto& binding_it : m_BufferInfos)
    {
        auto  binding_index = binding_it.first;
        auto& buffer_bindings = binding_it.second;

        if (auto binding_info = m_DescriptorSetLayout.getLayoutBinding(binding_index))
        {
            // Iterate over all binding buffers in array
            for (auto& element_it : buffer_bindings)
            {
                auto  array_element = element_it.first;
                auto& buffer_info = element_it.second;

                VkWriteDescriptorSet write_descriptor_set{ VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET };

                write_descriptor_set.dstBinding = binding_index;
                write_descriptor_set.descriptorType = binding_info->descriptorType;
                write_descriptor_set.pBufferInfo = &buffer_info;
                write_descriptor_set.dstSet = m_DescriptorSet;
                write_descriptor_set.dstArrayElement = array_element;
                write_descriptor_set.descriptorCount = 1;

                m_WriteDescriptorSets.push_back(write_descriptor_set);
            }
        }
        else
        {
            LOGERROR("Shader layout set does not use buffer binding at #{}", binding_index);
        }
    }

    // Iterate over all image bindings
    for (auto& binding_it : m_ImageInfos)
    {
        auto  binding_index = binding_it.first;
        auto& binding_resources = binding_it.second;

        if (auto binding_info = m_DescriptorSetLayout.getLayoutBinding(binding_index))
        {
            // Iterate over all binding images in array
            for (auto& element_it : binding_resources)
            {
                auto  array_element = element_it.first;
                auto& image_info = element_it.second;

                VkWriteDescriptorSet write_descriptor_set{ VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET };

                write_descriptor_set.dstBinding = binding_index;
                write_descriptor_set.descriptorType = binding_info->descriptorType;
                write_descriptor_set.pImageInfo = &image_info;
                write_descriptor_set.dstSet = m_DescriptorSet;
                write_descriptor_set.dstArrayElement = array_element;
                write_descriptor_set.descriptorCount = 1;

                m_WriteDescriptorSets.push_back(write_descriptor_set);
            }
        }
        else
        {
            LOGERROR("Shader layout set does not use image binding at #{}", binding_index);
        }
    }
}

void DescriptorSet::update(const std::vector<uint32_t>& bindings_to_update)
{
    std::vector<VkWriteDescriptorSet> write_operations;

    // If the 'bindings_to_update' vector is empty, we want to write to all the bindings (skipping those that haven't already been written)
    if (bindings_to_update.empty())
    {
        for (auto& write_operation : m_WriteDescriptorSets)
        {
            if (std::find(m_UpdatedBindings.begin(), m_UpdatedBindings.end(), write_operation.dstBinding) == m_UpdatedBindings.end())
            {
                write_operations.push_back(write_operation);
            }
        }
    }
    else
    {
        // Otherwise we want to update the binding indices present in the 'bindings_to_update' vector.
        // (Again skipping those that have already been written)
        for (auto& write_operation : m_WriteDescriptorSets)
        {
            if (std::find(bindings_to_update.begin(), bindings_to_update.end(), write_operation.dstBinding) != bindings_to_update.end() &&
                std::find(m_UpdatedBindings.begin(), m_UpdatedBindings.end(), write_operation.dstBinding) == m_UpdatedBindings.end())
            {
                write_operations.push_back(write_operation);
            }
        }
    }

    // Perform the Vulkan call to update the DescriptorSet by executing the write operations
    if (!write_operations.empty())
    {
        vkUpdateDescriptorSets(m_Device.get_handle(),
            write_operations.size(),
            write_operations.data(),
            0,
            nullptr);
    }

    // Store the bindings from the write operations that were executed by vkUpdateDescriptorSets to prevent overwriting by future calls to "update()"
    for (auto& write_op : write_operations)
    {
        m_UpdatedBindings.push_back(write_op.dstBinding);
    }
}

DescriptorSet::DescriptorSet(DescriptorSet&& other) :
    m_Device{ other.m_Device },
    m_DescriptorSetLayout{ other.m_DescriptorSetLayout },
    m_DescriptorPool{ other.m_DescriptorPool },
    m_BufferInfos{ std::move(other.m_BufferInfos) },
    m_ImageInfos{ std::move(other.m_ImageInfos) },
    m_DescriptorSet{ other.m_DescriptorSet },
    m_WriteDescriptorSets{ std::move(other.m_WriteDescriptorSets) },
    m_UpdatedBindings{ std::move(other.m_UpdatedBindings) }
{
    other.m_DescriptorSet = VK_NULL_HANDLE;
}


