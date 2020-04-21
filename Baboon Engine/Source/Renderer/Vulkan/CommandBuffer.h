#pragma once
#include "Common.h"
#include "resources\RenderPass.h"
#include "Subpass.h"
#include <memory>
#include <vector>
#include "PipelineState.h"
#include <unordered_map>
#include "ResourceBindingState.h"

class CommandPool;
class RenderTarget;
class VulkanImageView;
class PipelineLayout;
class FrameBuffer;
class DescriptorSetLayout;

struct ImageMemoryBarrier
{
    VkPipelineStageFlags src_stage_mask{ VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT };

    VkPipelineStageFlags dst_stage_mask{ VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT };

    VkAccessFlags src_access_mask{ 0 };

    VkAccessFlags dst_access_mask{ 0 };

    VkImageLayout old_layout{ VK_IMAGE_LAYOUT_UNDEFINED };

    VkImageLayout new_layout{ VK_IMAGE_LAYOUT_UNDEFINED };
};

struct RenderPassBinding
{
    const RenderPass* render_pass;
    const FrameBuffer* framebuffer;
};

class Buffer;

class CommandBuffer
{
public:
    enum class ResetMode
    {
        ResetPool,
        ResetIndividually,
        AlwaysAllocate,
    };
    enum class State
    {
        Invalid,
        Initial,
        Recording,
        Executable,
    };

    CommandBuffer(CommandPool& commandPool, VkCommandBufferLevel level);
    ~CommandBuffer();
    VkResult reset(ResetMode reset_mode);


    VkResult begin(VkCommandBufferUsageFlags flags, CommandBuffer* primary_cmd_buf = nullptr); //Getting the commandbuffer ready to record, the second cmdbuff is to inherit (optional)
    VkResult end();

    void beginRenderPass(const RenderTarget& render_target, const std::vector<LoadStoreInfo>& load_store_infos, const std::vector<VkClearValue>& clear_values, const std::vector<std::unique_ptr<Subpass>>& subpasses, VkSubpassContents contents = VK_SUBPASS_CONTENTS_INLINE);
    void endRenderPass();

    void nextSubpass();

    void imageBarrier(VulkanImageView& image_view, const ImageMemoryBarrier& memory_barrier);

    inline bool isRecording() { return m_State == State::Recording; }
    const inline VkCommandBuffer& getHandle()const { return m_CommandBuffer; }



    void setViewport(uint32_t first_viewport, const std::vector<VkViewport>& viewports);
    void setScissor(uint32_t first_scissor, const std::vector<VkRect2D>& scissors);

    void draw(uint32_t vertex_count, uint32_t instance_count, uint32_t first_vertex, uint32_t first_instance);
    void draw_indexed(uint32_t index_count, uint32_t instance_count, uint32_t first_index, int32_t vertex_offset, uint32_t first_instance);

    void bind_vertex_buffers(uint32_t first_binding, std::vector<std::reference_wrapper<Buffer>> buffers, const std::vector<VkDeviceSize>& offsets);
    void bind_index_buffer(Buffer& buffer, VkDeviceSize offset, VkIndexType index_type);

    void bind_buffer(const Buffer& buffer, VkDeviceSize offset, VkDeviceSize range, uint32_t set, uint32_t binding, uint32_t array_element);
    void bind_image(const VulkanImageView& image_view, const VulkanSampler& sampler, uint32_t set, uint32_t binding, uint32_t array_element);

    void copy_buffer_to_image(const Buffer& buffer, const VulkanImage& image, const std::vector<VkBufferImageCopy>& regions);


    void bindPipelineLayout(PipelineLayout& pipeline_layout);
    inline void setVertexInputState(const VertexInputState& vertState) { m_PipelineState.setVertexInputState(vertState); }
    inline void setColorBlendState(const ColorBlendState& state_info){m_PipelineState.setColorBlendState(state_info);}
    inline void setRasterState(const RasterizationState& state_info) { m_PipelineState.setRasterizationState(state_info); }
    inline void setDepthStencilState(const DepthStencilState& state_info) { m_PipelineState.setDepthStencilState(state_info); }



    void pushConstants(uint32_t offset, const std::vector<uint8_t>& values);

    template <typename T>
    void pushConstants(uint32_t offset, const T& value)
    {
        pushConstants(offset,
            std::vector<uint8_t>{reinterpret_cast<const uint8_t*>(&value),
            reinterpret_cast<const uint8_t*>(&value) + sizeof(T)});
    }


   
private:
   
    CommandPool& m_Pool;
    VkCommandBuffer m_CommandBuffer{ VK_NULL_HANDLE };
    const VkCommandBufferLevel m_Level;
    State m_State{ State::Initial };
    PipelineState m_PipelineState;
    ResourceBindingState m_ResourceBindingState;//Buffers and textures bindings
    std::unordered_map<uint32_t, DescriptorSetLayout*> m_DescriptorSetLayout_BindingState;
    RenderPassBinding m_CurrentRenderPass{ NULL,NULL };

    std::unordered_map<uint32_t, DescriptorSetLayout*> m_DescriptorSet_Binding_State;


    void flushPipelineState();//Submits changes if any and binds the pipeline
    void flushDescriptorState();

};