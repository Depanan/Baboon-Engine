#pragma once
#include "../Common.h"
#include <unordered_map>
#include <memory>

struct ShaderResource;
class Device;
class DescriptorSetLayout
{
public:
    DescriptorSetLayout(const Device& device, const std::vector<ShaderResource>& resource_set);

    DescriptorSetLayout(const DescriptorSetLayout&) = delete;
    DescriptorSetLayout& operator=(const DescriptorSetLayout&) = delete;
    DescriptorSetLayout& operator=(DescriptorSetLayout&&) = delete;

    DescriptorSetLayout(DescriptorSetLayout&& other);
    ~DescriptorSetLayout();

    VkDescriptorSetLayout getHandle() const{ return m_DescriptorSetLayout; }
    std::unique_ptr<VkDescriptorSetLayoutBinding> getLayoutBinding(const uint32_t binding_index) const;
    std::unique_ptr<VkDescriptorSetLayoutBinding> getLayoutBinding(const std::string& name) const;
    const std::vector<VkDescriptorSetLayoutBinding>& getBindings() const { return m_Bindings; }

private:
    const Device& m_Device;
    VkDescriptorSetLayout m_DescriptorSetLayout{ VK_NULL_HANDLE };
  
    std::vector<VkDescriptorSetLayoutBinding> m_Bindings;

    //std::vector<VkDescriptorBindingFlagsEXT> binding_flags;

    std::unordered_map<uint32_t, VkDescriptorSetLayoutBinding> m_Bindings_Lookup;

    //std::unordered_map<uint32_t, VkDescriptorBindingFlagsEXT> binding_flags_lookup;

    std::unordered_map<std::string, uint32_t> m_ResourcesLookup;
};

