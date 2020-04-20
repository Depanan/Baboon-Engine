#pragma once

#include <vector>

#include "Common.h"



struct VertexInputState
{
    std::vector<VkVertexInputBindingDescription> m_Bindings;
    std::vector<VkVertexInputAttributeDescription> m_Attributes;
};

struct InputAssemblyState
{
    VkPrimitiveTopology m_Topology{ VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST };
    VkBool32 m_PrimitiveRestartEnabled{ VK_FALSE };
};

struct RasterizationState
{
    VkBool32 m_DepthClampEnabled{ VK_FALSE };
    VkBool32 m_RasterizerDiscardEnabled{ VK_FALSE };
    VkPolygonMode m_PolygonMode{ VK_POLYGON_MODE_FILL };
    VkCullModeFlags m_CullMode{ VK_CULL_MODE_BACK_BIT };
    VkFrontFace m_FrontFace{ VK_FRONT_FACE_COUNTER_CLOCKWISE };
    VkBool32 m_DepthBiasEnabled{ VK_FALSE };
};

struct ViewportState
{
    uint32_t m_ViewportCount{ 1 };
    uint32_t m_ScissorCount{ 1 };
};

struct MultisampleState
{
    VkSampleCountFlagBits m_RasterizationSamples{ VK_SAMPLE_COUNT_1_BIT };
    VkBool32 m_SampleShadingEnabled{ VK_FALSE };
    float m_minSampleShading{ 0.0f };
    VkSampleMask m_SampleMask{ 0 };
    VkBool32 m_AlphaToCoverageEnabled{ VK_FALSE };
    VkBool32 m_AlphaToOneEnabled{ VK_FALSE };
};

struct StencilOpState
{
    VkStencilOp m_FailOp{ VK_STENCIL_OP_REPLACE };
    VkStencilOp m_PassOp{ VK_STENCIL_OP_REPLACE };
    VkStencilOp m_DepthFailOp{ VK_STENCIL_OP_REPLACE };
    VkCompareOp m_CompareOp{ VK_COMPARE_OP_NEVER };
};

struct DepthStencilState
{
    VkBool32 m_DepthTestEnable{ VK_TRUE };
    VkBool32 m_DepthWriteEnable{ VK_TRUE };
    VkCompareOp m_DepthCompareOp{ VK_COMPARE_OP_LESS_OR_EQUAL };
    VkBool32 m_DepthBoundTestEnable{ VK_FALSE };
    VkBool32 m_StencilTestEnable{ VK_FALSE };
    StencilOpState m_Front{};
    StencilOpState m_Back{};
};

struct ColorBlendAttachmentState
{
    VkBool32 m_BlendEnable{ VK_FALSE };
    VkBlendFactor m_SrcColorBlendFactor{ VK_BLEND_FACTOR_ONE };
    VkBlendFactor m_DstColorBlendFactor{ VK_BLEND_FACTOR_ZERO };
    VkBlendOp m_ColorBlendOp{ VK_BLEND_OP_ADD };
    VkBlendFactor m_SrcAlphaBlendFactor{ VK_BLEND_FACTOR_ONE };
    VkBlendFactor m_DstAlphaBlendFactor{ VK_BLEND_FACTOR_ZERO };
    VkBlendOp m_AlphaBlendOp{ VK_BLEND_OP_ADD };
    VkColorComponentFlags m_ColorWriteMask{ VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT };
};

struct ColorBlendState
{
    ColorBlendState() {

    }
    VkBool32 m_LogicOpEnabled{ VK_FALSE };
    VkLogicOp m_LogicOp{ VK_LOGIC_OP_CLEAR };
    std::vector<ColorBlendAttachmentState> m_Attachments;
};

/*
/// Helper class to create specialization constants for a Vulkan pipeline. The state tracks a pipeline globally, and not per shader. Two shaders using the same constant_id will have the same data.
class SpecializationConstantState
{
public:
    void reset();

    bool is_dirty() const;

    void clear_dirty();

    template <class T>
    void setconstant(uint32_t constant_id, const T& data);

    void setconstant(uint32_t constant_id, const std::vector<uint8_t>& data);

    void setspecialization_constantState(const std::map<uint32_t, std::vector<uint8_t>>& state);

    const std::map<uint32_t, std::vector<uint8_t>>& getspecialization_constantState() const;

private:
    bool dirty{ false };
    // Map tracking state of the Specialization Constants
    std::map<uint32_t, std::vector<uint8_t>> specialization_constantState;
};

template <class T>
inline void SpecializationConstantState::setconstant(std::uint32_t constant_id, const T& data)
{
    std::uint32_t value = static_cast<std::uint32_t>(data);

    setconstant(constant_id,
        { reinterpret_cast<const uint8_t*>(&value),
         reinterpret_cast<const uint8_t*>(&value) + sizeof(T) });
}

template <>
inline void SpecializationConstantState::setconstant<bool>(std::uint32_t constant_id, const bool& data_)
{
    std::uint32_t value = static_cast<std::uint32_t>(data_);

    setconstant(constant_id,
        { reinterpret_cast<const uint8_t*>(&value),
         reinterpret_cast<const uint8_t*>(&value) + sizeof(std::uint32_t) });
}*/

class PipelineLayout;
class RenderPass;
class PipelineState
{
public:
    void reset();

    void setPipelineLayout(PipelineLayout& pipeline_layout);

    void setRenderPass(const RenderPass& render_pass);

   // void setSpecializationConstant(uint32_t constant_id, const std::vector<uint8_t>& data);

    void setVertexInputState(const VertexInputState& vertex_input_sate);

    void setInputAssemblyState(const InputAssemblyState& input_assemblyState);

    void setRasterizationState(const RasterizationState& rasterizationState);

    void setViewportState(const ViewportState& viewportState);

    void setMultisampleState(const MultisampleState& multisampleState);

    void setDepthStencilState(const DepthStencilState& depth_stencilState);

    void setColorBlendState(const ColorBlendState& color_blendState);

    void setSubpassIndex(uint32_t subpass_index);

    inline const PipelineLayout& getPipelineLayout() const {return *m_PipelineLayout;}

    inline const RenderPass* getRenderPass() const { return m_RenderPass; }


    //const SpecializationConstantState& getspecialization_constantState() const;

    const VertexInputState& getVertexInputState() const;

    const InputAssemblyState& getInputAssemblyState() const;

    const RasterizationState& getRasterizationState() const;

    const ViewportState& getViewportState() const;

    const MultisampleState& getMultisampleState() const;

    const DepthStencilState& getDepthStencilState() const;

    const ColorBlendState& getColorBlendState() const;

    uint32_t getSubpassIndex() const;

    bool isDirty() const;

    void clearDirty();

private:
    bool m_Dirty{ false };

    PipelineLayout* m_PipelineLayout{ nullptr };
    const RenderPass* m_RenderPass{ nullptr };

    //SpecializationConstantState specialization_constantState{};

    VertexInputState m_VertexInputState{};
    InputAssemblyState m_InputAssemblyState{};
    RasterizationState m_RasterizationState{};
    ViewportState m_ViewportState{};
    MultisampleState m_MultisampleState{};
    DepthStencilState m_DepthStencilState{};
    ColorBlendState m_ColorBlendState{};
    uint32_t m_SubpassIndex{ 0U };
};
