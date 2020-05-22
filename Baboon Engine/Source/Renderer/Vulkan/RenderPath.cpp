#include "RenderPath.h"
#include "resources/RenderTarget.h"
#include "CommandBuffer.h"

RenderPath::RenderPath(std::vector<std::unique_ptr<Subpass>>&& subpasses):
    m_Subpasses{std::move(subpasses)}
{
    //call prepare
}


void RenderPath::add_subpass(std::unique_ptr<Subpass>&& subpass)
{
    //call prepare on the subpass to init stuff
    subpass->prepare();
    m_Subpasses.emplace_back(std::move(subpass));
}

void RenderPath::draw(CommandBuffer& command_buffer, RenderTarget& render_target, VkSubpassContents contents)
{
    
    while (m_ClearValue.size() < render_target.getAttachments().size())
    {
        m_ClearValue.push_back({ 0.0f, 0.0f, 0.0f, 1.0f });
    }

    for(int i = 0; i<m_Subpasses.size(); i++)
    {
        auto& subpass = m_Subpasses[i];
        subpass->updateRenderTargetAttachments();
        if (i == 0)
            command_buffer.beginRenderPass(render_target, m_LoadStore, m_ClearValue,m_Subpasses,contents);
        else
            command_buffer.nextSubpass(contents);

        subpass->draw(command_buffer);

    }

    
}
