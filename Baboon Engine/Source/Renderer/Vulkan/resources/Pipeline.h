#pragma once
#include "../Common.h"


class ShaderModule;
class Device;
class PipelineState;

//TODO: Here if I wanna make a compute pipeline it could inherit from this one like in the example, for now only graphics
class Pipeline
{
public:
    Pipeline(const Device& device, const PipelineState& pipeline_state);
    ~Pipeline();
    Pipeline(Pipeline&& other);
    Pipeline(const Pipeline&) = delete;
    Pipeline& operator=(const Pipeline&) = delete;
    Pipeline& operator=(Pipeline&&) = delete;


    inline const VkPipeline getHandle()const { return m_Handle; }

protected:
    const Device& m_Device;
    VkPipeline m_Handle = VK_NULL_HANDLE;
    const PipelineState& m_State;
};

