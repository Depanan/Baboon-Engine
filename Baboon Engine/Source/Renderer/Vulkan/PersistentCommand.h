#pragma once
#include "Common.h"
#include <map>

class CommandPool;
class CommandBuffer;
class RenderFrame;
class PersistentCommands {
public:
    PersistentCommands(Device& device, RenderFrame& rf,size_t threadIndex);
    ~PersistentCommands();
    void resetPool();
    void setRenderFrame(RenderFrame&);
    void startRecording();
    std::vector < CommandBuffer*> getCommandBuffers(size_t nCommands);
    std::vector < CommandBuffer*>& getCommandBuffers();

private:
    CommandPool* m_PersistentCommandPoolsPerFrame{ nullptr };
    std::vector<CommandBuffer*> m_PersistentCommandsPerFrame;
    size_t m_CommandsInUse = 0;
};
class PersistentCommandsPerFrame
{
public:
    ~PersistentCommandsPerFrame();
    PersistentCommands* getPersistentCommands(size_t frameId, size_t threadIndex, Device& device, RenderFrame& renderFrame);
    void resetPersistentCommands();//Needed for example when window is resized since render targets are updated
    void setAllDirty();
    bool getDirty(size_t frameId);
    void clearDirty(size_t frameId);
    std::vector<CommandBuffer*>& startRecording(size_t frameId);
    std::vector<CommandBuffer*>& getPreRecordedCommands(size_t frameId) { return m_PreRecordedCommands[frameId]; }


private:
    std::unordered_map < size_t, std::pair<bool, std::vector<PersistentCommands* >>> m_PersistentCommandsFrameThread;
    std::unordered_map < size_t, std::vector<CommandBuffer* >> m_PreRecordedCommands;
    std::pair<bool, std::vector<PersistentCommands* >>& getFramePersistentCommands(size_t frameId);
};





