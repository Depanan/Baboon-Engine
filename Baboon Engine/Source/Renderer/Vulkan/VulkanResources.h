#pragma once
#include <unordered_map>
#include <mutex>

#include "Core/ServiceLocator.h"

#include "resources/RenderPass.h"
#include "resources/RenderTarget.h"
#include "resources/FrameBuffer.h"
#include "resources/Shader.h"
#include "resources/PipelineLayout.h"
#include "resources/Pipeline.h"
#include "resources/DescriptorSetLayout.h"
#include "resources/DescriptorSet.h"
#include "resources/DescriptorPool.h"
#include <memory>


class Device;
class PipelineState;
class VulkanResources
{
public:
    VulkanResources(Device&, std::chrono::duration<int, std::milli> garbageCollectorInterval);
    RenderPass& request_render_pass(const std::vector<Attachment>& attachments,
        const std::vector<LoadStoreInfo>& load_store_infos,
        const std::vector<SubpassInfo>& subpasses);

    FrameBuffer& request_framebuffer(const RenderTarget& render_target, const RenderPass& render_pass);
    ShaderModule& request_shader_module(VkShaderStageFlagBits stage, const std::shared_ptr<ShaderSource>& glsl_source);//TODO: Add shadervariant here!
    PipelineLayout& request_pipeline_layout(std::vector<ShaderModule*> shader_modules);
    Pipeline& request_pipeline(const PipelineState& pipelineState);
    DescriptorSetLayout& request_descriptor_set_layout(const std::vector<ShaderResource>& set_resources);

    //A bit of a hack to let other classes call request_resource template function without having to create the specialization functions in their cpp so we can keep them all in vulkanResources.cpp
    DescriptorPool& request_descriptor_pool(std::unordered_map<std::size_t, DescriptorPool>& descriptorPoolsCache, DescriptorSetLayout& descriptor_set_layout);
    DescriptorSet& request_descriptor_set(std::unordered_map<std::size_t, DescriptorSet>& descriptorSetCache, DescriptorSetLayout& descriptor_set_layout, DescriptorPool& pool, const BindingMap<VkDescriptorBufferInfo>& buffer_infos, const BindingMap<VkDescriptorImageInfo>& image_infos);

    void clear();
    void GarbageCollect();

private:
    Device& m_Device;
    std::unordered_map<std::size_t, RenderPass> m_RenderPasses_Cache;
    std::unordered_map<std::size_t, FrameBuffer> m_FrameBuffers_Cache;
    std::unordered_map<std::size_t, ShaderModule> m_Shaders_Cache;
    std::unordered_map<std::size_t, PipelineLayout> m_PipelinesLayout_Cache;
    std::unordered_map<std::size_t, Pipeline> m_Pipelines_Cache;
    std::unordered_map<std::size_t, DescriptorSetLayout> m_DescriptorSetLayout_Cache;

    
    //Mutex to prevent concurrent thread accesses in the future
    std::mutex m_PipelineLayoutMutex;
    std::mutex m_PipelineMutex;
    std::mutex m_ShaderModuleMutex;
    std::mutex m_RenderPassMutex;
    std::mutex m_FramebufferMutex;
    std::mutex m_DescriptorSetLayoutMutex;

    std::chrono::duration<int, std::milli> m_GarbageCollectorInterval;
    std::chrono::time_point<std::chrono::steady_clock> m_StartGarbageCollection;

};


