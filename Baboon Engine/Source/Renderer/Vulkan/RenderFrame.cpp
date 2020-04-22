#include "RenderFrame.h"
#include "Device.h"
#include "Core/ServiceLocator.h"









RenderFrame::RenderFrame(Device& device, std::unique_ptr<RenderTarget>&& renderTarget, unsigned int nThreads):
    m_Device(device),
    m_Target(std::move(renderTarget)),
    m_NThreads(nThreads),
    m_SemaphorePool(device),
    m_FencePool(device),
    m_HashId((const char*) this)
{
    //Allocate resources per thread
    for (size_t i = 0; i < m_NThreads; i++)
    {
        m_DescriptorPools.push_back(std::make_unique<std::unordered_map<std::size_t, DescriptorPool>>());
        m_DescriptorSets.push_back(std::make_unique<std::unordered_map<std::size_t, DescriptorSet>>());
    }

}


std::vector<std::unique_ptr<CommandPool>>& RenderFrame::getCommandPools(const Queue& queue, CommandBuffer::ResetMode reset_mode)
{
    auto command_pool_it = m_CommandPools.find(queue.getFamilyIndex());

    if (command_pool_it != m_CommandPools.end())//If we find a commandpool for the queue..
    {
        if (command_pool_it->second.at(0)->getResetMode() != reset_mode)//If they don't have the same reset mode we need to recreate it
        {
            m_Device.wait_idle();

            // Delete pools
            m_CommandPools.erase(command_pool_it);//Delete to recreate after
        }
        else
        {
            return command_pool_it->second;//Bingo!!
        }
    }
    //We haven't find any suitable command pool so we create one per thread
    std::vector<std::unique_ptr<CommandPool>> queue_command_pools;
    for (size_t i = 0; i < m_NThreads; i++)
    {
        queue_command_pools.push_back(std::make_unique<CommandPool>(m_Device, queue.getFamilyIndex(), this, i, reset_mode));
    }

    auto res_ins_it = m_CommandPools.emplace(queue.getFamilyIndex(), std::move(queue_command_pools));

    if (!res_ins_it.second)
    {
        throw std::runtime_error("Failed to insert command pool");
    }

    command_pool_it = res_ins_it.first;

    return command_pool_it->second;//We've created the command pool, we return it
}


void RenderFrame::reset()
{
    auto waitFenceResult = m_FencePool.wait();
    if (waitFenceResult != VK_SUCCESS)
        LOGERROR("Error waiting for fence!");
    m_FencePool.reset();
    
    //reset command pools here 
    for (auto& command_pools_per_queue : m_CommandPools)
    {
        for (auto& command_pool : command_pools_per_queue.second)
        {
            command_pool->reset_pool();
        }
    }

    //reset buffer pools here TODO create them per frame

    m_SemaphorePool.reset();
}

VkSemaphore RenderFrame::requestSemaphore()
{
    return m_SemaphorePool.request_semaphore();
}
VkFence RenderFrame::requestFence()
{
    return m_FencePool.request_fence();
}



CommandBuffer&  RenderFrame::requestCommandBuffer(const Queue& queue, CommandBuffer::ResetMode reset_mode,VkCommandBufferLevel level, size_t thread_index) 
{
    assert(thread_index < m_NThreads && "Thread index is out of bounds");
    auto& command_pools = getCommandPools(queue, reset_mode);
    auto command_pool_it = std::find_if(command_pools.begin(), command_pools.end(), [&thread_index](std::unique_ptr<CommandPool>& cmd_pool) { return cmd_pool->getThreadIndex() == thread_index; });//We find the thread commandbuffer
    return (*command_pool_it)->request_command_buffer(level);

}

void RenderFrame::updateRenderTarget(std::unique_ptr<RenderTarget>&& render_target)
{
    m_Target = std::move(render_target);
}

DescriptorSet& RenderFrame::requestDescriptorSet(DescriptorSetLayout& descriptor_set_layout, const BindingMap<VkDescriptorBufferInfo>& buffer_infos, const BindingMap<VkDescriptorImageInfo>& image_infos, size_t thread_index)
{
    assert(thread_index < m_NThreads && "Thread index is out of bounds");

  
    auto& descriptor_pool = m_Device.getResourcesCache().request_descriptor_pool( *m_DescriptorPools.at(thread_index), descriptor_set_layout);
    return m_Device.getResourcesCache().request_descriptor_set(*m_DescriptorSets.at(thread_index), descriptor_set_layout, descriptor_pool, buffer_infos, image_infos);
}

