#pragma once

#include "Common.h"
#include "Subpass.h"
#include <vector>
#include <memory>
#include "resources/RenderPass.h"

class CommandBuffer;
class RenderTarget;
//In the examples, this is called Pipeline, but I renamed it to RenderPath to not get it confused with Resources/Pipeline. This is esentially a sequence of subpasses creating different RenderPaths (Deferred, forward.. )
class RenderPath
{
public:
    RenderPath(std::vector<std::unique_ptr<Subpass>>&& subpasses = {});
    void add_subpass(std::unique_ptr<Subpass>&& subpass);
    void draw(CommandBuffer& command_buffer, RenderTarget& render_target, VkSubpassContents contents = VK_SUBPASS_CONTENTS_INLINE);

    std::vector<std::unique_ptr<Subpass>>& getSubPasses() { return m_Subpasses; }
    void setClearValue(std::vector<VkClearValue>& clearValues) { m_ClearValue = clearValues; }
    void setLoadStoreValue(std::vector<LoadStoreInfo>& loadStore) { m_LoadStore = loadStore; }
private:
    std::vector<std::unique_ptr<Subpass>> m_Subpasses;
    std::vector<VkClearValue> m_ClearValue = std::vector<VkClearValue>(2);
    std::vector<LoadStoreInfo> m_LoadStore = std::vector<LoadStoreInfo>(2);
};
