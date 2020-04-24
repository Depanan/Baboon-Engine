#include "CommandPool.h"
#include "Core\ServiceLocator.h"
#include "Device.h"
#include "RenderFrame.h"
#include "VulkanContext.h"

CommandPool::CommandPool(Device& device, uint32_t queue_family_index, RenderFrame* render_frame,
    size_t                   thread_index
    ,CommandBuffer::ResetMode reset_mode):
    m_Device(device),
    m_ResetMode(reset_mode),
    m_threadIndex(thread_index),
    m_RenderFrame(render_frame)
{
    VkCommandPoolCreateFlags flags;
    switch (reset_mode)
    {
    case CommandBuffer::ResetMode::ResetIndividually:
    case CommandBuffer::ResetMode::AlwaysAllocate:
        flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
        break;
    case CommandBuffer::ResetMode::ResetPool:
    default:
        flags = VK_COMMAND_POOL_CREATE_TRANSIENT_BIT;
        break;
    }

    VkCommandPoolCreateInfo create_info{ VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO };

    create_info.queueFamilyIndex = queue_family_index;
    create_info.flags = flags;

    auto result = vkCreateCommandPool(m_Device.get_handle(), &create_info, nullptr, &m_CommandPool);

    if (result != VK_SUCCESS)
    {
        LOGERROR("Error creating CommandPool");
    }

}

CommandPool::CommandPool(CommandPool&& other) :
    m_Device{ other.m_Device },
    m_CommandPool{ other.m_CommandPool },
    m_threadIndex{ other.m_threadIndex },
    m_ResetMode{ other.m_ResetMode }
{
    other.m_CommandPool = VK_NULL_HANDLE;

  
}
CommandPool::~CommandPool()
{
    m_PrimaryCommandBuffers.clear();
    m_SecondaryCommandBuffers.clear();

    // Destroy command pool
    if (m_CommandPool != VK_NULL_HANDLE)
    {
        vkDestroyCommandPool(m_Device.get_handle(), m_CommandPool, nullptr);
    }
}

VkResult CommandPool::resetCommandBuffers()
{
    VkResult result = VK_SUCCESS;
    for (auto& cmd_buf : m_PrimaryCommandBuffers)
    {
        result = cmd_buf->reset(m_ResetMode);
        
        if (result != VK_SUCCESS)
        {
            return result;
        }
    }

    m_ActivePrimaryCBCount = 0;
    for (auto& cmd_buf : m_SecondaryCommandBuffers)
    {
        result = cmd_buf->reset(m_ResetMode);

        if (result != VK_SUCCESS)
        {
            return result;
        }
    }
    m_ActiveSecondaryCBCount = 0;
    return result;
}

VkResult CommandPool::reset_pool()
{
    VkResult result = VK_SUCCESS;

    switch (m_ResetMode)
    {
    case CommandBuffer::ResetMode::ResetIndividually:
    {
        result = resetCommandBuffers();

        break;
    }
    case CommandBuffer::ResetMode::ResetPool:
    {
        result = vkResetCommandPool(m_Device.get_handle(), m_CommandPool, 0);

        if (result != VK_SUCCESS)
        {
            return result;
        }

        result = resetCommandBuffers();

        break;
    }
    case CommandBuffer::ResetMode::AlwaysAllocate:
    {
        m_PrimaryCommandBuffers.clear();
        m_ActivePrimaryCBCount = 0;

        m_SecondaryCommandBuffers.clear();
        m_ActivePrimaryCBCount = 0;

        break;
    }
    default:
        LOGERROR("Unknown reset mode for command pools");
        result = VK_ERROR_INITIALIZATION_FAILED;//TODO change this not really right
    }

    return result;

    }

CommandBuffer& CommandPool::request_command_buffer(VkCommandBufferLevel level )
{
    if (level == VK_COMMAND_BUFFER_LEVEL_PRIMARY)
    {
        if (m_ActivePrimaryCBCount < m_PrimaryCommandBuffers.size())
        {
            return *m_PrimaryCommandBuffers.at(m_ActivePrimaryCBCount++);
        }

        m_PrimaryCommandBuffers.emplace_back(std::make_unique<CommandBuffer>(*this, level));

        m_ActivePrimaryCBCount++;

        return *m_PrimaryCommandBuffers.back();
    }
    else
    {
        if (m_ActiveSecondaryCBCount < m_SecondaryCommandBuffers.size())
        {
            return *m_SecondaryCommandBuffers.at(m_ActiveSecondaryCBCount++);
        }

        m_SecondaryCommandBuffers.emplace_back(std::make_unique<CommandBuffer>(*this, level));

        m_ActiveSecondaryCBCount++;

        return *m_SecondaryCommandBuffers.back();
    }
}

PersistentCommands* PersistentCommandsPerFrame::getPersistentCommands(const char* frameId, Device& device, RenderFrame& renderFrame)
{
    PersistentCommands* persistentCommands = nullptr;

    auto commandBuffersIt = m_PersistentCommandsPerFrame.find(frameId);
    if (commandBuffersIt == m_PersistentCommandsPerFrame.end())
    {
        //First time the current renderframe hits here, construct everything
        auto insertionRes = m_PersistentCommandsPerFrame.emplace(frameId, new PersistentCommands());
        if (insertionRes.second)
        {
            commandBuffersIt = insertionRes.first;
            persistentCommands = commandBuffersIt->second;
            persistentCommands->m_PersistentCommandPoolsPerFrame = new CommandPool(device, device.getGraphicsQueue().getFamilyIndex(), nullptr, 0, CommandBuffer::ResetMode::ResetIndividually);//TODO: Handle threads here!
            persistentCommands->m_PersistentCommandPoolsPerFrame->setRenderFrame(&renderFrame);
            persistentCommands->m_PersistentCommandsPerFrame = &(persistentCommands->m_PersistentCommandPoolsPerFrame->request_command_buffer(VK_COMMAND_BUFFER_LEVEL_SECONDARY));
            persistentCommands->m_NeedsSecondaryCommandsRecording = true;
        }

    }
    else
    {
        persistentCommands = commandBuffersIt->second;
    }

    return persistentCommands;
}

void PersistentCommandsPerFrame::resetPersistentCommands()
{
    for (auto persistentCommand : m_PersistentCommandsPerFrame)
    {
        persistentCommand.second->m_NeedsSecondaryCommandsRecording = true;

        persistentCommand.second->m_PersistentCommandPoolsPerFrame->getRenderFrame()->getFencePool().wait();//We need to wait since the commands might be in use here!, so we wait in fence associated to the Primary commandbuffer the secondary ones are executed
        persistentCommand.second->m_PersistentCommandPoolsPerFrame->reset_pool();
       
        persistentCommand.second->m_PersistentCommandsPerFrame = &(persistentCommand.second->m_PersistentCommandPoolsPerFrame->request_command_buffer(VK_COMMAND_BUFFER_LEVEL_SECONDARY));
    }
}

void PersistentCommandsPerFrame::setDirty()
{
    for (auto commandBuffersIt : m_PersistentCommandsPerFrame)
    {

        commandBuffersIt.second->m_NeedsSecondaryCommandsRecording = true;
    }
}
void PersistentCommandsPerFrame::getDirty(const char* frameId)
{
    auto commandBuffersIt = m_PersistentCommandsPerFrame.find(frameId);
    commandBuffersIt->second->m_NeedsSecondaryCommandsRecording;
}



