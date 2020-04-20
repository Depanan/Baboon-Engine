#pragma once
#include "../Common.h"
#include <unordered_map>
#include "Shader.h"

class Device;
class DescriptorSetLayout;

//Pipeline layouts define what types of resources can be accessed by a given pipeline (Resources like textures uniforms...)
class PipelineLayout
{
public:
    PipelineLayout( Device& device, const std::vector<ShaderModule*>& shader_modules);
   
    PipelineLayout(PipelineLayout&& other);
    ~PipelineLayout();
    PipelineLayout(const PipelineLayout&) = delete;
    PipelineLayout& operator=(const PipelineLayout&) = delete;
    PipelineLayout& operator=(PipelineLayout&&) = delete;

    inline VkPipelineLayout getHandle() const { return m_PipelineLayout; }
    inline const std::vector<ShaderModule*>& getShaderModules()const { return m_ShaderModules; }
    inline const std::unordered_map<uint32_t, std::vector<ShaderResource>>& getShaderSets()const { return m_ShaderSets; }
    const std::vector<ShaderResource> getResources(const ShaderResourceType& type = ShaderResourceType::All, VkShaderStageFlagBits stage = VK_SHADER_STAGE_ALL) const;

    inline DescriptorSetLayout& PipelineLayout::getDescriptorSetLayout(uint32_t set_index) const{return *m_DescriptorSetLayouts.at(set_index);}
    inline bool PipelineLayout::hasDescriptorSetLayout(uint32_t set_index) const{ return set_index < m_DescriptorSetLayouts.size();}

    VkShaderStageFlags getPushConstantRangeStage(uint32_t offset, uint32_t size) const;
private:
     Device& m_Device;
    VkPipelineLayout m_PipelineLayout{ VK_NULL_HANDLE };
    std::vector<ShaderModule*> m_ShaderModules;

    

    // The shader resources that this pipeline layout uses, indexed by their name
    std::unordered_map<std::string, ShaderResource> m_ShaderResources;

    // A map of each set and the resources it owns used by the pipeline layout
    std::unordered_map<uint32_t, std::vector<ShaderResource>> m_ShaderSets;

    // The different descriptor set layouts for this pipeline layout
    std::unordered_map<uint32_t, DescriptorSetLayout*> m_DescriptorSetLayouts;
};

