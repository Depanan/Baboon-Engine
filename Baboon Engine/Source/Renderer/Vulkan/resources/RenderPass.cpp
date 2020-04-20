#include "RenderPass.h"
#include "../Device.h"
#include "RenderTarget.h"
#include <algorithm>
#include "Core/ServiceLocator.h"



void set_attachment_layouts(std::vector<VkSubpassDescription>& subpass_descriptions, std::vector<VkAttachmentDescription>& attachment_descriptions)
{
    // Make the initial layout same as in the first subpass using that attachment
    for (auto& subpass : subpass_descriptions)
    {
        for (size_t k = 0U; k < subpass.colorAttachmentCount; ++k)
        {
            auto& reference = subpass.pColorAttachments[k];
            // Set it only if not defined yet
            if (attachment_descriptions[reference.attachment].initialLayout == VK_IMAGE_LAYOUT_UNDEFINED)
            {
                attachment_descriptions[reference.attachment].initialLayout = reference.layout;
            }
        }

        for (size_t k = 0U; k < subpass.inputAttachmentCount; ++k)
        {
            auto& reference = subpass.pInputAttachments[k];
            // Set it only if not defined yet
            if (attachment_descriptions[reference.attachment].initialLayout == VK_IMAGE_LAYOUT_UNDEFINED)
            {
                attachment_descriptions[reference.attachment].initialLayout = reference.layout;
            }
        }

        if (subpass.pDepthStencilAttachment)
        {
            auto& reference = *subpass.pDepthStencilAttachment;
            // Set it only if not defined yet
            if (attachment_descriptions[reference.attachment].initialLayout == VK_IMAGE_LAYOUT_UNDEFINED)
            {
                attachment_descriptions[reference.attachment].initialLayout = reference.layout;
            }
        }

        if (subpass.pResolveAttachments)
        {
            for (size_t k = 0U; k < subpass.colorAttachmentCount; ++k)
            {
                auto& reference = subpass.pResolveAttachments[k];
                // Set it only if not defined yet
                if (attachment_descriptions[reference.attachment].initialLayout == VK_IMAGE_LAYOUT_UNDEFINED)
                {
                    attachment_descriptions[reference.attachment].initialLayout = reference.layout;
                }
            }
        }

       /* if (const auto depth_resolve = get_depth_resolve_reference(subpass))
        {
            // Set it only if not defined yet
            if (attachment_descriptions[depth_resolve->attachment].initialLayout == VK_IMAGE_LAYOUT_UNDEFINED)
            {
                attachment_descriptions[depth_resolve->attachment].initialLayout = depth_resolve->layout;
            }
        }*/
    }

    // Make the final layout same as the last subpass layout
    {
        auto& subpass = subpass_descriptions.back();

        for (size_t k = 0U; k < subpass.colorAttachmentCount; ++k)
        {
            const auto& reference = subpass.pColorAttachments[k];

            attachment_descriptions[reference.attachment].finalLayout = reference.layout;
        }

        for (size_t k = 0U; k < subpass.inputAttachmentCount; ++k)
        {
            const auto& reference = subpass.pInputAttachments[k];

            attachment_descriptions[reference.attachment].finalLayout = reference.layout;

            // Do not use depth attachment if used as input
            if (is_depth_stencil_format(attachment_descriptions[reference.attachment].format))
            {
                subpass.pDepthStencilAttachment = nullptr;
            }
        }

        if (subpass.pDepthStencilAttachment)
        {
            const auto& reference = *subpass.pDepthStencilAttachment;

            attachment_descriptions[reference.attachment].finalLayout = reference.layout;
        }

        if (subpass.pResolveAttachments)
        {
            for (size_t k = 0U; k < subpass.colorAttachmentCount; ++k)
            {
                const auto& reference = subpass.pResolveAttachments[k];

                attachment_descriptions[reference.attachment].finalLayout = reference.layout;
            }
        }

        /*if (const auto depth_resolve = get_depth_resolve_reference(subpass))
        {
            attachment_descriptions[depth_resolve->attachment].finalLayout = depth_resolve->layout;
        }*/
    }
}


RenderPass::RenderPass(const Device& device, const std::vector<Attachment>& attachments, const std::vector<LoadStoreInfo>& load_store_infos, const std::vector<SubpassInfo>& subpasses) :
    m_Device(device),
    m_SubpassCount(subpasses.size())

{

    //Fill attachment descriptions 
    std::vector<VkAttachmentDescription> attachment_descriptions;
    for (size_t i = 0U; i < attachments.size(); ++i)
    {
        VkAttachmentDescription attachment{};

        attachment.format = attachments[i].m_Format;
        attachment.samples = attachments[i].m_Samples;
        attachment.initialLayout = attachments[i].m_InitialLayout;
        attachment.finalLayout = is_depth_stencil_format(attachment.format) ? VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL : VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

        if (i < load_store_infos.size())
        {
            attachment.loadOp = load_store_infos[i].load_op;
            attachment.storeOp = load_store_infos[i].store_op;
            attachment.stencilLoadOp = load_store_infos[i].load_op;
            attachment.stencilStoreOp = load_store_infos[i].store_op;
        }

        attachment_descriptions.push_back(std::move(attachment));
    }

    // Store attachments for every subpass
    std::vector<std::vector<VkAttachmentReference>> input_attachments{ m_SubpassCount };
    std::vector<std::vector<VkAttachmentReference>> color_attachments{ m_SubpassCount };
    std::vector<std::vector<VkAttachmentReference>> depth_stencil_attachments{ m_SubpassCount };
    //std::vector<std::vector<VkAttachmentReference>> color_resolve_attachments{ m_SubpassCount };
    std::vector<std::vector<VkAttachmentReference>> depth_resolve_attachments{ m_SubpassCount };

    for (size_t i = 0; i < subpasses.size(); ++i)
    {
        auto& subpass = subpasses[i];

        // Fill color attachments references
        for (auto o_attachment : subpass.output_attachments)
        {
            auto  initial_layout = attachments[o_attachment].m_InitialLayout == VK_IMAGE_LAYOUT_UNDEFINED ? VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL : attachments[o_attachment].m_InitialLayout;
            auto& description = attachment_descriptions[o_attachment];
            if (!is_depth_stencil_format(description.format))
            {
                VkAttachmentReference reference;
                reference.attachment = o_attachment;
                reference.layout = initial_layout;
                color_attachments[i].push_back(reference);
            }
        }

        // Fill input attachments references
        for (auto i_attachment : subpass.input_attachments)
        {
            auto default_layout = is_depth_stencil_format(attachment_descriptions[i_attachment].format) ? VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL : VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
            auto initial_layout = attachments[i_attachment].m_InitialLayout == VK_IMAGE_LAYOUT_UNDEFINED ? default_layout : attachments[i_attachment].m_InitialLayout;
            VkAttachmentReference reference;
            reference.attachment = i_attachment;
            reference.layout = initial_layout;
            input_attachments[i].push_back(reference);
        }
        if (!subpass.m_DisableDepthAttachment)
        {
            // Assumption: depth stencil attachment appears in the list before any depth stencil resolve attachment
            auto it = std::find_if(attachments.begin(), attachments.end(), [](const Attachment attachment) { return is_depth_stencil_format(attachment.m_Format); });
            if (it != attachments.end())
            {
                auto i_depth_stencil = std::distance(attachments.begin(), it);
                auto initial_layout = it->m_InitialLayout == VK_IMAGE_LAYOUT_UNDEFINED ? VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL : it->m_InitialLayout;
                VkAttachmentReference reference;
                reference.attachment = i_depth_stencil;
                reference.layout = initial_layout;
                depth_stencil_attachments[i].push_back(reference);

                /*if (subpass.depth_stencil_resolve_mode != VK_RESOLVE_MODE_NONE)
                {
                    auto i_depth_stencil_resolve = subpass.depth_stencil_resolve_attachment;
                    initial_layout = attachments[i_depth_stencil_resolve].initial_layout == VK_IMAGE_LAYOUT_UNDEFINED ? VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL : attachments[i_depth_stencil_resolve].initial_layout;
                    depth_resolve_attachments[i].push_back(get_attachment_reference<T_AttachmentReference>(i_depth_stencil_resolve, initial_layout));
                }*/
            }
        }
       

    }

    std::vector<VkSubpassDescription> subpass_descriptions;
    subpass_descriptions.reserve(m_SubpassCount);
    //VkSubpassDescriptionDepthStencilResolve depth_resolve{};
    for (size_t i = 0; i < subpasses.size(); ++i)
    {
        auto& subpass = subpasses[i];

        VkSubpassDescription subpass_description{};
      
        subpass_description.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;

        subpass_description.pInputAttachments = input_attachments[i].empty() ? nullptr : input_attachments[i].data();
        subpass_description.inputAttachmentCount = input_attachments[i].size();

        subpass_description.pColorAttachments = color_attachments[i].empty() ? nullptr : color_attachments[i].data();
        subpass_description.colorAttachmentCount = color_attachments[i].size();

        //subpass_description.pResolveAttachments = color_resolve_attachments[i].empty() ? nullptr : color_resolve_attachments[i].data();

        subpass_description.pDepthStencilAttachment = nullptr;
        if (!depth_stencil_attachments[i].empty())
        {
            subpass_description.pDepthStencilAttachment = depth_stencil_attachments[i].data();

            /*if (!depth_resolve_attachments[i].empty())
            {
                // If the pNext list of VkSubpassDescription2 includes a VkSubpassDescriptionDepthStencilResolve structure,
                // then that structure describes multisample resolve operations for the depth/stencil attachment in a subpass
                depth_resolve.sType = VK_STRUCTURE_TYPE_SUBPASS_DESCRIPTION_DEPTH_STENCIL_RESOLVE_KHR;
                depth_resolve.depthResolveMode = subpass.depth_stencil_resolve_mode;
                set_pointer_next(subpass_description, depth_resolve, depth_resolve_attachments[i][0]);

                auto& reference = depth_resolve_attachments[i][0];
                // Set it only if not defined yet
                if (attachment_descriptions[reference.attachment].initialLayout == VK_IMAGE_LAYOUT_UNDEFINED)
                {
                    attachment_descriptions[reference.attachment].initialLayout = reference.layout;
                }
            }*/
        }

        subpass_descriptions.push_back(subpass_description);
    }

    // Default subpass
    if (subpasses.empty())
    {
        VkSubpassDescription subpass_description{};
        subpass_description.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
        uint32_t default_depth_stencil_attachment{ VK_ATTACHMENT_UNUSED };

        for (uint32_t k = 0U; k < attachment_descriptions.size(); ++k)
        {
            if (is_depth_stencil_format(attachments[k].m_Format))
            {
                if (default_depth_stencil_attachment == VK_ATTACHMENT_UNUSED)
                {
                    default_depth_stencil_attachment = k;
                }
                continue;
            }
            VkAttachmentReference reference;
            reference.attachment = k;
            reference.layout = VK_IMAGE_LAYOUT_GENERAL;

            color_attachments[0].push_back(reference);
        }

        subpass_description.pColorAttachments = color_attachments[0].data();

        if (default_depth_stencil_attachment != VK_ATTACHMENT_UNUSED)
        {
            VkAttachmentReference reference;
            reference.attachment = default_depth_stencil_attachment;
            reference.layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
            depth_stencil_attachments[0].push_back(reference);

            subpass_description.pDepthStencilAttachment = depth_stencil_attachments[0].data();
        }

        subpass_descriptions.push_back(subpass_description);
    }

    set_attachment_layouts(subpass_descriptions, attachment_descriptions);


    m_ColorOutputCount.reserve(m_SubpassCount);
    for (size_t i = 0; i < m_SubpassCount; i++)
    {
        m_ColorOutputCount.push_back(color_attachments[i].size());
    }


    std::vector<VkSubpassDependency> dependencies(m_SubpassCount - 1);

    if (m_SubpassCount > 1)
    {
        for (uint32_t i = 0; i < dependencies.size(); ++i)
        {
            // Transition input attachments from color attachment to shader read
            dependencies[i].srcSubpass = i;
            dependencies[i].dstSubpass = i + 1;
            dependencies[i].srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
            dependencies[i].dstStageMask = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
            dependencies[i].srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
            dependencies[i].dstAccessMask = VK_ACCESS_INPUT_ATTACHMENT_READ_BIT;
            dependencies[i].dependencyFlags = VK_DEPENDENCY_BY_REGION_BIT;
        }
    }

    VkRenderPassCreateInfo create_info{};
    create_info.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
    create_info.attachmentCount = attachment_descriptions.size();
    create_info.pAttachments = attachment_descriptions.data();
    create_info.subpassCount = subpass_descriptions.size();
    create_info.pSubpasses = subpass_descriptions.data();
    create_info.dependencyCount = dependencies.size();
    create_info.pDependencies = dependencies.data();

    auto result = vkCreateRenderPass(m_Device.get_handle(), &create_info,nullptr, &m_RenderPass);

    if (result != VK_SUCCESS) {
        LOGERROR("Error creating render pass!");
    }
}


RenderPass::RenderPass(RenderPass&& other) :
    m_Device{ other.m_Device },
    m_RenderPass{ other.m_RenderPass },
    m_ColorOutputCount{other.m_ColorOutputCount},
    m_SubpassCount{other.m_SubpassCount}
{
    other.m_RenderPass = VK_NULL_HANDLE;
}

RenderPass::~RenderPass()
{
    if(m_RenderPass != VK_NULL_HANDLE)
        vkDestroyRenderPass(m_Device.get_handle(), m_RenderPass, nullptr);
}

