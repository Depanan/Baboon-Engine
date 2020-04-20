
#include "Pipeline.h"
#include "../PipelineState.h"
#include "PipelineLayout.h"
#include "Shader.h"
#include "../Device.h"
#include "Core/ServiceLocator.h"

Pipeline::Pipeline(const Device& device, const PipelineState& pipeline_state):
    m_Device(device),
    m_State(pipeline_state)
{
    std::vector<VkShaderModule> shader_modules;

    std::vector<VkPipelineShaderStageCreateInfo> stage_create_infos;


/*
    // Create specialization info from tracked state. This is shared by all shaders.
    std::vector<uint8_t>                  data{};
    std::vector<VkSpecializationMapEntry> map_entries{};

    const auto specialization_constant_state = pipeline_state.get_specialization_constant_state().get_specialization_constant_state();

    for (const auto specialization_constant : specialization_constant_state)
    {
        map_entries.push_back({ specialization_constant.first, to_u32(data.size()), specialization_constant.second.size() });
        data.insert(data.end(), specialization_constant.second.begin(), specialization_constant.second.end());
    }

    VkSpecializationInfo specialization_info{};
    specialization_info.mapEntryCount = to_u32(map_entries.size());
    specialization_info.pMapEntries = map_entries.data();
    specialization_info.dataSize = data.size();
    specialization_info.pData = data.data();
    */
    

    for (const ShaderModule* shader_module : pipeline_state.getPipelineLayout().getShaderModules())
    {
        VkPipelineShaderStageCreateInfo stage_create_info{ VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO };

        stage_create_info.stage = shader_module->getStage();
        stage_create_info.pName = shader_module->getEntryPoint().c_str();

        
        auto shader_source = shader_module->getSourceBinary();
        VkShaderModuleCreateInfo createInfo = {};
        createInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
        createInfo.codeSize = shader_source.size() * sizeof(uint32_t);

        
        createInfo.pCode = shader_source.data();

        VkResult result = vkCreateShaderModule(m_Device.get_handle(), &createInfo, nullptr, &stage_create_info.module);



        if (result != VK_SUCCESS)
        {
            LOGERROR("Error creating shader!");
        }

        //stage_create_info.pSpecializationInfo = &specialization_info;

        stage_create_infos.push_back(stage_create_info);
        shader_modules.push_back(stage_create_info.module);
    }

    VkGraphicsPipelineCreateInfo create_info{ VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO };

    create_info.stageCount = stage_create_infos.size();
    create_info.pStages = stage_create_infos.data();

    VkPipelineVertexInputStateCreateInfo vertex_input_state{ VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO };

    auto vertexInputState = pipeline_state.getVertexInputState();
    vertex_input_state.pVertexAttributeDescriptions = vertexInputState.m_Attributes.data();
    vertex_input_state.vertexAttributeDescriptionCount = vertexInputState.m_Attributes.size();

    vertex_input_state.pVertexBindingDescriptions = vertexInputState.m_Bindings.data();
    vertex_input_state.vertexBindingDescriptionCount = vertexInputState.m_Bindings.size();
    
    //TODO: This is like this until we get to create vertex buffers, now hardcoding in shader for testing
    /*vertex_input_state.pVertexAttributeDescriptions = nullptr;
    vertex_input_state.vertexAttributeDescriptionCount =0;

    vertex_input_state.pVertexBindingDescriptions = nullptr;
    vertex_input_state.vertexBindingDescriptionCount = 0;*/
    

    VkPipelineInputAssemblyStateCreateInfo input_assembly_state{ VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO };

    input_assembly_state.topology = pipeline_state.getInputAssemblyState().m_Topology;
    input_assembly_state.primitiveRestartEnable = pipeline_state.getInputAssemblyState().m_PrimitiveRestartEnabled;

    VkPipelineViewportStateCreateInfo viewport_state{ VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO };

    viewport_state.viewportCount = pipeline_state.getViewportState().m_ViewportCount;
    viewport_state.scissorCount = pipeline_state.getViewportState().m_ScissorCount;

    VkPipelineRasterizationStateCreateInfo rasterization_state{ VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO };

    rasterization_state.depthClampEnable = pipeline_state.getRasterizationState().m_DepthClampEnabled;
    rasterization_state.rasterizerDiscardEnable = pipeline_state.getRasterizationState().m_RasterizerDiscardEnabled;
    rasterization_state.polygonMode = pipeline_state.getRasterizationState().m_PolygonMode;
    rasterization_state.cullMode = pipeline_state.getRasterizationState().m_CullMode;
    rasterization_state.frontFace = pipeline_state.getRasterizationState().m_FrontFace;
    rasterization_state.depthBiasEnable = pipeline_state.getRasterizationState().m_DepthBiasEnabled;
    rasterization_state.depthBiasClamp = 1.0f;
    rasterization_state.depthBiasSlopeFactor = 1.0f;
    rasterization_state.lineWidth = 1.0f;

    VkPipelineMultisampleStateCreateInfo multisample_state{ VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO };

    multisample_state.sampleShadingEnable = pipeline_state.getMultisampleState().m_SampleShadingEnabled;
    multisample_state.rasterizationSamples = pipeline_state.getMultisampleState().m_RasterizationSamples;
    multisample_state.minSampleShading = pipeline_state.getMultisampleState().m_minSampleShading;
    multisample_state.alphaToCoverageEnable = pipeline_state.getMultisampleState().m_AlphaToCoverageEnabled;
    multisample_state.alphaToOneEnable = pipeline_state.getMultisampleState().m_AlphaToOneEnabled;

    if (pipeline_state.getMultisampleState().m_SampleMask)
    {
        multisample_state.pSampleMask = &pipeline_state.getMultisampleState().m_SampleMask;
    }

    VkPipelineDepthStencilStateCreateInfo depth_stencil_state{ VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO };

    depth_stencil_state.depthTestEnable = pipeline_state.getDepthStencilState().m_DepthTestEnable;
    depth_stencil_state.depthWriteEnable = pipeline_state.getDepthStencilState().m_DepthWriteEnable;
    depth_stencil_state.depthCompareOp = pipeline_state.getDepthStencilState().m_DepthCompareOp;
    depth_stencil_state.depthBoundsTestEnable = pipeline_state.getDepthStencilState().m_DepthBoundTestEnable;
    depth_stencil_state.stencilTestEnable = pipeline_state.getDepthStencilState().m_StencilTestEnable;
    depth_stencil_state.front.failOp = pipeline_state.getDepthStencilState().m_Front.m_FailOp;
    depth_stencil_state.front.passOp = pipeline_state.getDepthStencilState().m_Front.m_PassOp;
    depth_stencil_state.front.depthFailOp = pipeline_state.getDepthStencilState().m_Front.m_DepthFailOp;
    depth_stencil_state.front.compareOp = pipeline_state.getDepthStencilState().m_Front.m_CompareOp;
    depth_stencil_state.front.compareMask = ~0U;
    depth_stencil_state.front.writeMask = ~0U;
    depth_stencil_state.front.reference = ~0U;
    depth_stencil_state.back.failOp = pipeline_state.getDepthStencilState().m_Back.m_FailOp;
    depth_stencil_state.back.passOp = pipeline_state.getDepthStencilState().m_Back.m_PassOp;
    depth_stencil_state.back.depthFailOp = pipeline_state.getDepthStencilState().m_Back.m_DepthFailOp;
    depth_stencil_state.back.compareOp = pipeline_state.getDepthStencilState().m_Back.m_CompareOp;
    depth_stencil_state.back.compareMask = ~0U;
    depth_stencil_state.back.writeMask = ~0U;
    depth_stencil_state.back.reference = ~0U;

    VkPipelineColorBlendStateCreateInfo color_blend_state{ VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO };

    color_blend_state.logicOpEnable = pipeline_state.getColorBlendState().m_LogicOpEnabled;
    color_blend_state.logicOp = pipeline_state.getColorBlendState().m_LogicOp;
    color_blend_state.attachmentCount = pipeline_state.getColorBlendState().m_Attachments.size();
    color_blend_state.pAttachments = reinterpret_cast<const VkPipelineColorBlendAttachmentState*>(pipeline_state.getColorBlendState().m_Attachments.data());
    color_blend_state.blendConstants[0] = 1.0f;
    color_blend_state.blendConstants[1] = 1.0f;
    color_blend_state.blendConstants[2] = 1.0f;
    color_blend_state.blendConstants[3] = 1.0f;

    std::array<VkDynamicState, 9> dynamic_states{
        VK_DYNAMIC_STATE_VIEWPORT,
        VK_DYNAMIC_STATE_SCISSOR,
        VK_DYNAMIC_STATE_LINE_WIDTH,
        VK_DYNAMIC_STATE_DEPTH_BIAS,
        VK_DYNAMIC_STATE_BLEND_CONSTANTS,
        VK_DYNAMIC_STATE_DEPTH_BOUNDS,
        VK_DYNAMIC_STATE_STENCIL_COMPARE_MASK,
        VK_DYNAMIC_STATE_STENCIL_WRITE_MASK,
        VK_DYNAMIC_STATE_STENCIL_REFERENCE,
    };

    VkPipelineDynamicStateCreateInfo dynamic_state{ VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO };

    dynamic_state.pDynamicStates = dynamic_states.data();
    dynamic_state.dynamicStateCount = dynamic_states.size();

    create_info.pVertexInputState = &vertex_input_state;
    create_info.pInputAssemblyState = &input_assembly_state;
    create_info.pViewportState = &viewport_state;
    create_info.pRasterizationState = &rasterization_state;
    create_info.pMultisampleState = &multisample_state;
    create_info.pDepthStencilState = &depth_stencil_state;
    create_info.pColorBlendState = &color_blend_state;
    create_info.pDynamicState = &dynamic_state;

    create_info.layout = pipeline_state.getPipelineLayout().getHandle();
    create_info.renderPass = pipeline_state.getRenderPass()->getHandle();
    create_info.subpass = pipeline_state.getSubpassIndex();

    auto result = vkCreateGraphicsPipelines(device.get_handle(), nullptr, 1, &create_info, nullptr, &m_Handle);

    if (result != VK_SUCCESS)
    {
        LOGERROR("Cant create Pipeline!");
    }

    for (auto shader_module : shader_modules)
    {
        vkDestroyShaderModule(device.get_handle(), shader_module, nullptr);
    }

}
