#pragma once
#include "Common.h"
#include <unordered_map>

class Device;
class VulkanBuffer;
class VulkanSampler;
class VulkanImageView;

struct ResourceInfo
{
    bool m_Dirty{ false };
    const VulkanBuffer* m_Buffer{ nullptr };
    VkDeviceSize m_Offset{ 0 };
    VkDeviceSize m_Range{ 0 };
    const VulkanImageView* m_ImageView{ nullptr };
    const VulkanSampler* m_Sampler{ nullptr };
};

/**
 * @brief A resource set is a set of bindings containing resources that were bound
 *        by a command buffer.
 *
 * The ResourceSet has a one to one mapping with a DescriptorSet.
 */
class ResourceSet
{
public:
    void reset();

    bool is_dirty() const;

    void clear_dirty();

    void clear_dirty(uint32_t binding, uint32_t array_element);

    void bind_buffer(const VulkanBuffer& buffer, VkDeviceSize offset, VkDeviceSize range, uint32_t binding, uint32_t array_element);

    void bind_image(const VulkanImageView& image_view, const VulkanSampler& sampler, uint32_t binding, uint32_t array_element);

    void bind_input(const VulkanImageView& image_view, uint32_t binding, uint32_t array_element);

    inline const BindingMap<ResourceInfo>& get_resource_bindings() const { return m_Resource_Bindings; }

    void forceDirty();

private:
    bool m_Dirty{ false };

    BindingMap<ResourceInfo> m_Resource_Bindings;
};

class ResourceBindingState
{
public:
    void reset();

    bool is_dirty();

    void clear_dirty();

    void clear_dirty(uint32_t set);

    void bind_buffer(const VulkanBuffer& buffer, VkDeviceSize offset, VkDeviceSize range, uint32_t set, uint32_t binding, uint32_t array_element);

    void bind_image(const VulkanImageView& image_view, const VulkanSampler& sampler, uint32_t set, uint32_t binding, uint32_t array_element);

    void bind_input(const VulkanImageView& image_view, uint32_t set, uint32_t binding, uint32_t array_element);

    inline const std::unordered_map<uint32_t, ResourceSet>& get_resource_sets() { return m_Resource_Sets; }

    void forceDirty();

private:
    bool m_Dirty{ false };
    std::unordered_map<uint32_t, ResourceSet> m_Resource_Sets;

};