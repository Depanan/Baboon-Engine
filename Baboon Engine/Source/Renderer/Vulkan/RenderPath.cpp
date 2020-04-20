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
    std::vector<LoadStoreInfo> load_store = std::vector<LoadStoreInfo>(2);
    std::vector<VkClearValue> clear_value = std::vector<VkClearValue>(2);

    clear_value[0].color.float32[0] = 1.0f;
    clear_value[1].color.float32[0] = 1.0f;

    for(int i = 0; i<m_Subpasses.size(); i++)
    {
        auto& subpass = m_Subpasses[i];
        if (i == 0)
            command_buffer.beginRenderPass(render_target, load_store, clear_value,m_Subpasses);
        else
            command_buffer.nextSubpass();

        subpass->draw(command_buffer);

    }

    
}
