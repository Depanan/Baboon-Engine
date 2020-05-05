#pragma once
#include "Common.h"
#include <map>

class CommandPool;
class CommandBuffer;
class RenderFrame;
class PersistentCommands {
public:
    PersistentCommands(Device& device, RenderFrame& rf);
    ~PersistentCommands();
    void resetPool();
    void setRenderFrame(RenderFrame&);
    void setDirty() { m_NeedsSecondaryCommandsRecording = true; }
    void clearDirty() { m_NeedsSecondaryCommandsRecording = false; }
    bool getDirty() { return m_NeedsSecondaryCommandsRecording; }
    CommandBuffer* getCommandBuffer() { return m_PersistentCommandsPerFrame; }

private:
    bool m_NeedsSecondaryCommandsRecording{ true };
    CommandPool* m_PersistentCommandPoolsPerFrame{ nullptr };
    CommandBuffer* m_PersistentCommandsPerFrame{ nullptr };//TODO: This needs to be an array to allow more than one!
};
class PersistentCommandsPerFrame
{
public:
    ~PersistentCommandsPerFrame();
    PersistentCommands* getPersistentCommands(size_t frameId, Device& device, RenderFrame& renderFrame);
    void resetPersistentCommands();//Needed for example when window is resized since render targets are updated
    void setDirty();
    bool getDirty(size_t frameId);

private:
    std::unordered_map < size_t, PersistentCommands*> m_PersistentCommandsPerFrame;
};