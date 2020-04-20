#include "ResourceBindingState.h"
#include "Buffer.h"
#include "VulkanImageView.h"
#include "VulkanSampler.h"

void ResourceBindingState::reset()
{
    clear_dirty();

    m_Resource_Sets.clear();
}
bool ResourceBindingState::is_dirty()
{
    return m_Dirty;
}

void ResourceBindingState::clear_dirty()
{
    m_Dirty = false;
}

void ResourceBindingState::clear_dirty(uint32_t set)
{
    m_Resource_Sets[set].clear_dirty();
}
void ResourceBindingState::bind_buffer(const Buffer& buffer, VkDeviceSize offset, VkDeviceSize range, uint32_t set, uint32_t binding, uint32_t array_element)
{
    m_Resource_Sets[set].bind_buffer(buffer, offset, range, binding, array_element);

    m_Dirty = true;
}

void ResourceBindingState::bind_image(const VulkanImageView& image_view, const VulkanSampler& sampler, uint32_t set, uint32_t binding, uint32_t array_element)
{
    m_Resource_Sets[set].bind_image(image_view, sampler, binding, array_element);

    m_Dirty = true;
}

void ResourceBindingState::bind_input(const VulkanImageView& image_view, uint32_t set, uint32_t binding, uint32_t array_element)
{
    m_Resource_Sets[set].bind_input(image_view, binding, array_element);

    m_Dirty = true;
}



void ResourceSet::reset()
{
    clear_dirty();

    m_Resource_Bindings.clear();
}

bool ResourceSet::is_dirty() const
{
    return m_Dirty;
}

void ResourceSet::clear_dirty()
{
    m_Dirty = false;
}

void ResourceSet::clear_dirty(uint32_t binding, uint32_t array_element)
{
    m_Resource_Bindings[binding][array_element].m_Dirty = false;
}

void ResourceSet::bind_buffer(const Buffer& buffer, VkDeviceSize offset, VkDeviceSize range, uint32_t binding, uint32_t array_element)
{
    m_Resource_Bindings[binding][array_element].m_Dirty = true;
    m_Resource_Bindings[binding][array_element].m_Buffer = &buffer;
    m_Resource_Bindings[binding][array_element].m_Offset = offset;
    m_Resource_Bindings[binding][array_element].m_Range = range;

    m_Dirty = true;
}

void ResourceSet::bind_image(const VulkanImageView& image_view, const VulkanSampler& sampler, uint32_t binding, uint32_t array_element)
{
    m_Resource_Bindings[binding][array_element].m_Dirty = true;
    m_Resource_Bindings[binding][array_element].m_ImageView = &image_view;
    m_Resource_Bindings[binding][array_element].m_Sampler = &sampler;

    m_Dirty = true;
}

void ResourceSet::bind_input(const VulkanImageView& image_view, const uint32_t binding, const uint32_t array_element)
{
    m_Resource_Bindings[binding][array_element].m_Dirty = true;
    m_Resource_Bindings[binding][array_element].m_ImageView = &image_view;

    m_Dirty = true;
}

