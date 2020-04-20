#include "../Device.h"
#include "Core/ServiceLocator.h"
#include "PipelineLayout.h"
#include "DescriptorSetLayout.h"

PipelineLayout::PipelineLayout( Device& device, const std::vector<ShaderModule*>& shader_modules):
m_Device(device),
m_ShaderModules(shader_modules)
{
    for (auto* shader_module : shader_modules)
    {
        for (const auto& shader_resource : shader_module->get_resources())
        {
            std::string key = shader_resource.name;

            // Since 'Input' and 'Output' resources can have the same name, we modify the key string
            if (shader_resource.type == ShaderResourceType::Input || shader_resource.type == ShaderResourceType::Output)
            {
                key = std::to_string(shader_resource.stages) + "_" + key;
            }

            auto it = m_ShaderResources.find(key);

            if (it != m_ShaderResources.end())
            {
                // Append stage flags if resource already exists
                it->second.stages |= shader_resource.stages;
            }
            else
            {
                // Create a new entry in the map
                m_ShaderResources.emplace(key, shader_resource);
            }
        }
    }

    // Sift through the map of name indexed shader resources
  // Seperate them into their respective sets
    for (auto& it : m_ShaderResources)
    {
        auto& shader_resource = it.second;

        // Find binding by set index in the map.
        auto it2 = m_ShaderSets.find(shader_resource.set);

        if (it2 != m_ShaderSets.end())
        {
            // Add resource to the found set index
            it2->second.push_back(shader_resource);
        }
        else
        {
            // Create a new set index and with the first resource
            m_ShaderSets.emplace(shader_resource.set, std::vector<ShaderResource>{shader_resource});
        }
    }

    // Create a descriptor set layout for each shader set in the shader modules
    for (auto& shader_set_it : m_ShaderSets)
    {
    
       m_DescriptorSetLayouts.emplace(shader_set_it.first, &m_Device.getResourcesCache().request_descriptor_set_layout(shader_set_it.second));
    }

    // Collect all the descriptor set layout handles
    std::vector<VkDescriptorSetLayout> descriptor_set_layout_handles(m_DescriptorSetLayouts.size());
    std::transform(m_DescriptorSetLayouts.begin(), m_DescriptorSetLayouts.end(), descriptor_set_layout_handles.begin(),
        [](auto& descriptor_set_layout_it) { return descriptor_set_layout_it.second->getHandle(); });
  

    // Collect all the push constant shader resources
    std::vector<VkPushConstantRange> push_constant_ranges;
    for (auto& push_constant_resource : getResources(ShaderResourceType::PushConstant))
    {
        push_constant_ranges.push_back({ push_constant_resource.stages, push_constant_resource.offset, push_constant_resource.size });
    }

    VkPipelineLayoutCreateInfo create_info{ VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO };

    create_info.setLayoutCount = descriptor_set_layout_handles.size();
    create_info.pSetLayouts = descriptor_set_layout_handles.data();
    create_info.pushConstantRangeCount =push_constant_ranges.size();
    create_info.pPushConstantRanges = push_constant_ranges.data();

    // Create the Vulkan pipeline layout handle
    auto result = vkCreatePipelineLayout(device.get_handle(), &create_info, nullptr, &m_PipelineLayout);

    if (result != VK_SUCCESS)
    {
        LOGERROR("Cannot create pipelineLayout!");
    }


}

PipelineLayout::PipelineLayout(PipelineLayout&& other):
    m_Device(other.m_Device),
    m_ShaderModules(other.m_ShaderModules),
    m_PipelineLayout(other.m_PipelineLayout),
    m_ShaderResources(other.m_ShaderResources),
    m_ShaderSets(other.m_ShaderSets),
    m_DescriptorSetLayouts(other.m_DescriptorSetLayouts)
{
    other.m_PipelineLayout = VK_NULL_HANDLE;
}

PipelineLayout::~PipelineLayout()
{
    if(m_PipelineLayout)
        vkDestroyPipelineLayout(m_Device.get_handle(), m_PipelineLayout, nullptr);
   
}

const std::vector<ShaderResource> PipelineLayout::getResources(const ShaderResourceType& type, VkShaderStageFlagBits stage) const
{
    std::vector<ShaderResource> found_resources;

    for (auto& it : m_ShaderResources)
    {
        auto& shader_resource = it.second;

        if (shader_resource.type == type || type == ShaderResourceType::All)
        {
            if (shader_resource.stages == stage || stage == VK_SHADER_STAGE_ALL)
            {
                found_resources.push_back(shader_resource);
            }
        }
    }

    return found_resources;
}


VkShaderStageFlags PipelineLayout::getPushConstantRangeStage(uint32_t offset, uint32_t size) const
{
    VkShaderStageFlags stages = 0;

    for (auto& push_constant_resource : getResources(ShaderResourceType::PushConstant))
    {
        if (offset >= push_constant_resource.offset && offset + size <= push_constant_resource.offset + push_constant_resource.size)
        {
            stages |= push_constant_resource.stages;
        }
    }
    return stages;
}