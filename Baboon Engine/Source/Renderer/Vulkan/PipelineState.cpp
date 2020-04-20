#include "PipelineState.h"
#include <tuple>
#include <cassert>
#include "resources/PipelineLayout.h"
#include "resources/RenderPass.h"


bool operator==(const VkVertexInputAttributeDescription& lhs, const VkVertexInputAttributeDescription& rhs)
{
    return std::tie(lhs.binding, lhs.format, lhs.location, lhs.offset) == std::tie(rhs.binding, rhs.format, rhs.location, rhs.offset);
}

bool operator==(const VkVertexInputBindingDescription& lhs, const VkVertexInputBindingDescription& rhs)
{
    return std::tie(lhs.binding, lhs.inputRate, lhs.stride) == std::tie(rhs.binding, rhs.inputRate, rhs.stride);
}
bool operator!=(const VertexInputState& lhs, const VertexInputState& rhs)
{
    return std::tie(lhs.m_Attributes, lhs.m_Bindings) != std::tie(rhs.m_Attributes, rhs.m_Bindings);
}

bool operator==(const ColorBlendAttachmentState& lhs, const ColorBlendAttachmentState& rhs)
{
    return std::tie(lhs.m_AlphaBlendOp, lhs.m_BlendEnable, lhs.m_ColorBlendOp, lhs.m_ColorWriteMask, lhs.m_DstAlphaBlendFactor, lhs.m_DstColorBlendFactor, lhs.m_SrcAlphaBlendFactor, lhs.m_SrcColorBlendFactor) ==
        std::tie(rhs.m_AlphaBlendOp, rhs.m_BlendEnable, rhs.m_ColorBlendOp, rhs.m_ColorWriteMask, rhs.m_DstAlphaBlendFactor, rhs.m_DstColorBlendFactor, rhs.m_SrcAlphaBlendFactor, rhs.m_SrcColorBlendFactor);
}

bool operator!=(const StencilOpState& lhs, const StencilOpState& rhs)
{
    return std::tie(lhs.m_CompareOp, lhs.m_DepthFailOp, lhs.m_FailOp, lhs.m_PassOp) != std::tie(rhs.m_CompareOp, rhs.m_DepthFailOp, rhs.m_FailOp, rhs.m_PassOp);
}

bool operator!=(const RasterizationState& lhs, const RasterizationState& rhs)
{
    return std::tie(lhs.m_CullMode, lhs.m_DepthBiasEnabled, lhs.m_DepthClampEnabled, lhs.m_FrontFace, lhs.m_PolygonMode, lhs.m_RasterizerDiscardEnabled) !=
        std::tie(rhs.m_CullMode, rhs.m_DepthBiasEnabled, rhs.m_DepthClampEnabled, rhs.m_FrontFace, rhs.m_PolygonMode, rhs.m_RasterizerDiscardEnabled);
}

bool operator!=(const ViewportState& lhs, const ViewportState& rhs)
{
    return std::tie(lhs.m_ViewportCount, lhs.m_ScissorCount) != std::tie(rhs.m_ViewportCount, rhs.m_ScissorCount);
}

bool operator!=(const MultisampleState& lhs, const MultisampleState& rhs)
{
    return std::tie(lhs.m_AlphaToCoverageEnabled, lhs.m_AlphaToOneEnabled, lhs.m_minSampleShading, lhs.m_RasterizationSamples, lhs.m_SampleMask, lhs.m_SampleShadingEnabled) !=
        std::tie(rhs.m_AlphaToCoverageEnabled, rhs.m_AlphaToOneEnabled, rhs.m_minSampleShading, rhs.m_RasterizationSamples, rhs.m_SampleMask, rhs.m_SampleShadingEnabled);
}

bool operator!=(const DepthStencilState& lhs, const DepthStencilState& rhs)
{
    return std::tie(lhs.m_DepthBoundTestEnable, lhs.m_DepthCompareOp, lhs.m_DepthTestEnable, lhs.m_DepthWriteEnable, lhs.m_StencilTestEnable) !=
        std::tie(rhs.m_DepthBoundTestEnable, rhs.m_DepthCompareOp, rhs.m_DepthTestEnable, rhs.m_DepthWriteEnable, rhs.m_StencilTestEnable) ||
        lhs.m_Back != rhs.m_Back || lhs.m_Front != rhs.m_Front;
}

bool operator!=(const ColorBlendState& lhs, const ColorBlendState& rhs)
{
    return std::tie(lhs.m_LogicOp, lhs.m_LogicOpEnabled) != std::tie(rhs.m_LogicOp, rhs.m_LogicOpEnabled) ||
        lhs.m_Attachments.size() != rhs.m_Attachments.size() ||
        !std::equal(lhs.m_Attachments.begin(), lhs.m_Attachments.end(), rhs.m_Attachments.begin(),
            [](const ColorBlendAttachmentState& lhs, const ColorBlendAttachmentState& rhs) {
        return lhs == rhs;
    });
}










void PipelineState::reset()
{
    clearDirty();
    m_PipelineLayout = nullptr;
    m_RenderPass = nullptr;
    //specialization_constantState.reset();
    m_VertexInputState = {};
    m_InputAssemblyState = {};
    m_RasterizationState = {};
    m_MultisampleState = {};
    m_DepthStencilState = {};
    m_ColorBlendState = {};
    m_SubpassIndex = { 0U };
}

void PipelineState::setPipelineLayout(PipelineLayout& pipeline_layout)
{
    if (m_PipelineLayout)
    {
        if (m_PipelineLayout->getHandle() != pipeline_layout.getHandle())
        {
            m_PipelineLayout = &pipeline_layout;
            m_Dirty = true;
        }
    }
    else
    {
        m_PipelineLayout = &pipeline_layout;
        m_Dirty = true;
    }
}



void PipelineState::setRenderPass(const RenderPass& new_render_pass)
{
    if (m_RenderPass)
    {
        if (m_RenderPass->getHandle() != new_render_pass.getHandle())
        {
            m_RenderPass = &new_render_pass;
            m_Dirty = true;
        }
    }
    else
    {
        m_RenderPass = &new_render_pass;
        m_Dirty = true;
    }
}

/*
void PipelineState::set_specialization_constant(uint32_t constant_id, const std::vector<uint8_t>& data)
{
    specialization_constantState.set_constant(constant_id, data);

    if (specialization_constantState.is_dirty())
    {
        dirty = true;
    }
}
*/



void PipelineState::setVertexInputState(const VertexInputState& new_vertex_input_sate)
{
    if (m_VertexInputState != new_vertex_input_sate)
    {
        m_VertexInputState = new_vertex_input_sate;
        m_Dirty = true;
    }
}


bool operator!=(const InputAssemblyState& lhs, const InputAssemblyState& rhs)
{
    return std::tie(lhs.m_PrimitiveRestartEnabled, lhs.m_Topology) != std::tie(rhs.m_PrimitiveRestartEnabled, rhs.m_Topology);
}
void PipelineState::setInputAssemblyState(const InputAssemblyState& new_input_assemblyState)
{
    if (m_InputAssemblyState != new_input_assemblyState)
    {
        m_InputAssemblyState = new_input_assemblyState;
        m_Dirty = true;
    }
}

void PipelineState::setRasterizationState(const RasterizationState& new_rasterizationState)
{
    if (m_RasterizationState != new_rasterizationState)
    {
        m_RasterizationState = new_rasterizationState;

        m_Dirty = true;
    }
}

void PipelineState::setViewportState(const ViewportState& new_viewportState)
{
    if (m_ViewportState != new_viewportState)
    {
        m_ViewportState = new_viewportState;

        m_Dirty = true;
    }
}

void PipelineState::setMultisampleState(const MultisampleState& new_multisampleState)
{
    if (m_MultisampleState != new_multisampleState)
    {
        m_MultisampleState = new_multisampleState;

        m_Dirty = true;
    }
}

void PipelineState::setDepthStencilState(const DepthStencilState& new_depth_stencilState)
{
    if (m_DepthStencilState != new_depth_stencilState)
    {
        m_DepthStencilState = new_depth_stencilState;

        m_Dirty = true;
    }
}

void PipelineState::setColorBlendState(const ColorBlendState& new_color_blendState)
{
    if (m_ColorBlendState != new_color_blendState)
    {
        m_ColorBlendState = new_color_blendState;

        m_Dirty = true;
    }
}

void PipelineState::setSubpassIndex(uint32_t new_subpass_index)
{
    if (m_SubpassIndex != new_subpass_index)
    {
        m_SubpassIndex = new_subpass_index;

        m_Dirty = true;
    }
}

/*const PipelineLayout& PipelineState::getpipeline_layout() const
{
    assert(pipeline_layout && "Graphics state Pipeline layout is not set");
    return *pipeline_layout;
}

const RenderPass* PipelineState::getrender_pass() const
{
    return render_pass;
}*/
/*
const SpecializationConstantState& PipelineState::getspecialization_constantState() const
{
    return specialization_constantState;
}
*/
const VertexInputState& PipelineState::getVertexInputState() const
{
    return m_VertexInputState;
}

const InputAssemblyState& PipelineState::getInputAssemblyState() const
{
    return m_InputAssemblyState;
}

const RasterizationState& PipelineState::getRasterizationState() const
{
    return m_RasterizationState;
}

const ViewportState& PipelineState::getViewportState() const
{
    return m_ViewportState;
}

const MultisampleState& PipelineState::getMultisampleState() const
{
    return m_MultisampleState;
}

const DepthStencilState& PipelineState::getDepthStencilState() const
{
    return m_DepthStencilState;
}

const ColorBlendState& PipelineState::getColorBlendState() const
{
    return m_ColorBlendState;
}

uint32_t PipelineState::getSubpassIndex() const
{
    return m_SubpassIndex;
}

bool PipelineState::isDirty() const
{
    return m_Dirty;//|| specialization_constantState.is_dirty();
}

void PipelineState::clearDirty()
{
    m_Dirty = false;
    //specialization_constantState.clear_dirty();
}
