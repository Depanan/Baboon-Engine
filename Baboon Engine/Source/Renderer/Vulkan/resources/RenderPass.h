#pragma once
#include "../Common.h"
#include <vector>

struct LoadStoreInfo
{
    VkAttachmentLoadOp load_op = VK_ATTACHMENT_LOAD_OP_CLEAR;
    VkAttachmentStoreOp store_op = VK_ATTACHMENT_STORE_OP_STORE;
};
struct SubpassInfo
{
    std::vector<uint32_t> input_attachments;
    std::vector<uint32_t> output_attachments;
    bool m_DisableDepthAttachment = false;
};

struct Attachment;
class Device;
class RenderPass
{
public:
    RenderPass(const Device& device, 
                 const std::vector<Attachment>& attachments,
                 const std::vector<LoadStoreInfo>& load_store_infos,
                 const std::vector<SubpassInfo>& subpasses);


    RenderPass(const RenderPass&) = delete;

    RenderPass(RenderPass&& other);

    ~RenderPass();

    RenderPass& operator=(const RenderPass&) = delete;

    RenderPass& operator=(RenderPass&&) = delete;

    inline const VkRenderPass& getHandle()const { return m_RenderPass; }

    const uint32_t getColorOutputCount(uint32_t subpass_index) const { return m_ColorOutputCount[subpass_index]; }
private:
    VkRenderPass m_RenderPass{ VK_NULL_HANDLE };
    const Device& m_Device;
    size_t m_SubpassCount;
    std::vector<uint32_t> m_ColorOutputCount;

};