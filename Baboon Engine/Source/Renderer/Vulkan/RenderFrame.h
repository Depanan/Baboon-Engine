#pragma once
#include "Common.h"
#include "resources/RenderTarget.h"
#include "resources/DescriptorPool.h"
#include "Queue.h"
#include "SemaphorePool.h"
#include "FencePool.h"
#include "CommandPool.h"
#include <map>
#include <memory>
#include "resources/DescriptorSet.h"

class Device;
class RenderFrame
{
public:
    RenderFrame(Device& device, std::unique_ptr<RenderTarget>&& renderTarget,unsigned int nThreads);

    RenderFrame(const RenderFrame&) = delete;
    RenderFrame(RenderFrame&&) = delete;
    RenderFrame& operator=(const RenderFrame&) = delete;
    RenderFrame& operator=(RenderFrame&&) = delete;

    std::vector<std::unique_ptr<CommandPool>>& getCommandPools(const Queue& queue, CommandBuffer::ResetMode reset_mode);
    void reset();
    VkSemaphore requestSemaphore();
    VkFence requestFence();
    CommandBuffer& requestCommandBuffer(const Queue& queue,CommandBuffer::ResetMode reset_mode = CommandBuffer::ResetMode::ResetPool,
        VkCommandBufferLevel     level = VK_COMMAND_BUFFER_LEVEL_PRIMARY,
        size_t                   thread_index = 0);

    RenderTarget& getRenderTarget()const { return *m_Target; }

    void updateRenderTarget(std::unique_ptr<RenderTarget>&& render_target);

    DescriptorSet& requestDescriptorSet(DescriptorSetLayout& descriptor_set_layout, const BindingMap<VkDescriptorBufferInfo>& buffer_infos, const BindingMap<VkDescriptorImageInfo>& image_infos, size_t thread_index);
   
    FencePool& getFencePool() { return m_FencePool; }
   
private:
  
    Device& m_Device;
    std::unique_ptr<RenderTarget>  m_Target;
    unsigned int m_NThreads;

    /// Commands pools for this frame. There is a VECTOR PER QUEUE FAMILY (mapped with the queue family id in the map) having that VECTOR AS MANY QUEUES AS THREADS
    std::map<uint32_t, std::vector<std::unique_ptr<CommandPool>>> m_CommandPools;

    /// Descriptor pools for the frame
    std::vector<std::unique_ptr<std::unordered_map<std::size_t, DescriptorPool>>> m_DescriptorPools;

    /// Descriptor sets for the frame
    std::vector<std::unique_ptr<std::unordered_map<std::size_t, DescriptorSet>>> m_DescriptorSets;

    SemaphorePool m_SemaphorePool;
    FencePool m_FencePool;
};