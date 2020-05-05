#include "CommandPool.h"
#include "PersistentCommand.h"
#include "Device.h"
#include "RenderFrame.h"

PersistentCommandsPerFrame::~PersistentCommandsPerFrame()
{
    for (auto persistentCommand : m_PersistentCommandsPerFrame)
    {
        delete persistentCommand.second;
    }
    m_PersistentCommandsPerFrame.clear();
}

PersistentCommands* PersistentCommandsPerFrame::getPersistentCommands(size_t frameId, Device& device, RenderFrame& renderFrame)
{
    PersistentCommands* persistentCommands = nullptr;

    auto commandBuffersIt = m_PersistentCommandsPerFrame.find(frameId);
    if (commandBuffersIt == m_PersistentCommandsPerFrame.end())
    {
        //First time the current renderframe hits here, construct everything
        auto insertionRes = m_PersistentCommandsPerFrame.emplace(frameId, new PersistentCommands(device,renderFrame));//TODO: Handle threads here!
        if (insertionRes.second)
        {
            commandBuffersIt = insertionRes.first;
            persistentCommands = commandBuffersIt->second;
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
        persistentCommand.second->setDirty();
        persistentCommand.second->resetPool();
    }
}

void PersistentCommandsPerFrame::setDirty()
{
    for (auto commandBuffersIt : m_PersistentCommandsPerFrame)
    {

        commandBuffersIt.second->setDirty();
    }
}
bool PersistentCommandsPerFrame::getDirty(size_t frameId)
{
    auto commandBuffersIt = m_PersistentCommandsPerFrame.find(frameId);
    return commandBuffersIt->second->getDirty();
}

PersistentCommands::PersistentCommands(Device& device, RenderFrame& rf)
{
    m_PersistentCommandPoolsPerFrame = new CommandPool(device, device.getGraphicsQueue().getFamilyIndex(), nullptr, 0, CommandBuffer::ResetMode::ResetPool);
    m_PersistentCommandPoolsPerFrame->setRenderFrame(&rf);
    m_PersistentCommandsPerFrame = &(m_PersistentCommandPoolsPerFrame->request_command_buffer(VK_COMMAND_BUFFER_LEVEL_SECONDARY));
    m_NeedsSecondaryCommandsRecording = true;
}

PersistentCommands::~PersistentCommands()
{
    m_PersistentCommandPoolsPerFrame->reset_pool();
    delete m_PersistentCommandPoolsPerFrame;
}

void PersistentCommands::resetPool()
{
    m_PersistentCommandPoolsPerFrame->getRenderFrame()->getFencePool().wait();//We need to wait since the commands might be in use here!, so we wait in fence associated to the Primary commandbuffer the secondary ones are executed
    m_PersistentCommandPoolsPerFrame->reset_pool();
    m_PersistentCommandsPerFrame = &(m_PersistentCommandPoolsPerFrame->request_command_buffer(VK_COMMAND_BUFFER_LEVEL_SECONDARY));
}

void PersistentCommands::setRenderFrame(RenderFrame& rf)
{
    m_PersistentCommandPoolsPerFrame->setRenderFrame(&rf);
}
