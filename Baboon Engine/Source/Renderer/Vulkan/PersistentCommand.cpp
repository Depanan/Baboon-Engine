#include "CommandPool.h"
#include "PersistentCommand.h"
#include "Device.h"
#include "RenderFrame.h"

PersistentCommandsPerFrame::~PersistentCommandsPerFrame()
{
    /*for (auto persistentCommand : m_PersistentCommandsPerFrame)
    {
        delete persistentCommand.second;
    }
    m_PersistentCommandsPerFrame.clear();*/
}



PersistentCommands* PersistentCommandsPerFrame::getPersistentCommands(size_t frameId,size_t threadIndex, Device& device, RenderFrame& renderFrame)
{
    PersistentCommands* persistentCommands = nullptr;
    std::vector<PersistentCommands*>& threadsVector = getFramePersistentCommands(frameId).second;

    if (threadIndex >= threadsVector.size())
    {
        size_t diff = threadIndex - threadsVector.size() + 1;

        threadsVector.insert(threadsVector.end(),diff, new PersistentCommands(device, renderFrame, threadIndex));
    }

    
    return threadsVector[threadIndex];
}

void PersistentCommandsPerFrame::resetPersistentCommands()
{
    for (auto& persistentCommandVector : m_PersistentCommandsFrameThread)
    {
        persistentCommandVector.second.first = true;//dirty
        for (auto& persistentCommand : persistentCommandVector.second.second)
        {
            persistentCommand->resetPool();
            persistentCommand->getCommandBuffers().clear();
        }
    }
}

void PersistentCommandsPerFrame::setAllDirty()
{
    for (auto& persistentCommandVector : m_PersistentCommandsFrameThread)
    {
        persistentCommandVector.second.first = true;
    }
}
bool PersistentCommandsPerFrame::getDirty(size_t frameId)
{
    return getFramePersistentCommands(frameId).first;
}

void PersistentCommandsPerFrame::clearDirty(size_t frameId)
{
    getFramePersistentCommands(frameId).first = false;
}

std::vector<CommandBuffer*>& PersistentCommandsPerFrame::startRecording(size_t frameId)
{
    auto& persistentCommandVector = m_PersistentCommandsFrameThread[frameId];
    for (auto persistentCommandPerThread : persistentCommandVector.second)
    {
        persistentCommandPerThread->startRecording();
    }
    
    m_PreRecordedCommands[frameId].clear();
    return  m_PreRecordedCommands[frameId];
}

std::pair<bool, std::vector<PersistentCommands* >>& PersistentCommandsPerFrame::getFramePersistentCommands(size_t frameId)
{
    auto commandBuffersIt = m_PersistentCommandsFrameThread.find(frameId);
    if (commandBuffersIt == m_PersistentCommandsFrameThread.end())
    {
        //First time the current renderframe hits here, construct everything
        m_PreRecordedCommands.emplace(frameId, std::vector <CommandBuffer*>());
        auto insertionRes = m_PersistentCommandsFrameThread.emplace(frameId, std::make_pair(true, std::vector<PersistentCommands*>()));
        if (insertionRes.second)
        {
            commandBuffersIt = insertionRes.first;
            return commandBuffersIt->second;
        }

    }
    else
    {
        return commandBuffersIt->second;
    }
}


PersistentCommands::PersistentCommands(Device& device, RenderFrame& rf, size_t threadIndex)
{
    m_PersistentCommandPoolsPerFrame = new CommandPool(device, device.getGraphicsQueue().getFamilyIndex(), nullptr, threadIndex, CommandBuffer::ResetMode::ResetPool);
    m_PersistentCommandPoolsPerFrame->setRenderFrame(&rf);
    //m_PersistentCommandsPerFrame = &(m_PersistentCommandPoolsPerFrame->request_command_buffer(VK_COMMAND_BUFFER_LEVEL_SECONDARY));
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
    m_PersistentCommandsPerFrame.clear();

    //m_PersistentCommandsPerFrame = &(m_PersistentCommandPoolsPerFrame->request_command_buffer(VK_COMMAND_BUFFER_LEVEL_SECONDARY));
}

void PersistentCommands::setRenderFrame(RenderFrame& rf)
{
    m_PersistentCommandPoolsPerFrame->setRenderFrame(&rf);
}

void PersistentCommands::startRecording()
{
    m_CommandsInUse = 0;
}

std::vector<CommandBuffer*> PersistentCommands::getCommandBuffers(size_t nCommands)
{
    if ((nCommands+ m_CommandsInUse) > m_PersistentCommandsPerFrame.size())
    {
        size_t diff = (nCommands + m_CommandsInUse) - m_PersistentCommandsPerFrame.size();
        for (int i = 0; i < diff; i++)
        {
            m_PersistentCommandsPerFrame.push_back(&(m_PersistentCommandPoolsPerFrame->request_command_buffer(VK_COMMAND_BUFFER_LEVEL_SECONDARY)));
        }
    }
    size_t index = m_CommandsInUse;
    m_CommandsInUse += nCommands;
    return std::vector<CommandBuffer*>(m_PersistentCommandsPerFrame.begin() + index, m_PersistentCommandsPerFrame.begin() + index + nCommands);
}
std::vector<CommandBuffer*>& PersistentCommands::getCommandBuffers()
{
    return m_PersistentCommandsPerFrame;
}












