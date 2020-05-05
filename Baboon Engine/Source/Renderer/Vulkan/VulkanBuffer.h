#pragma once
#include "Common.h"
#include <vk_mem_alloc.h>
#include "../Common/Buffer.h"

class Device;
class VulkanBuffer : public Buffer
{
public:
    VulkanBuffer(Device& device,
        VkDeviceSize             size,
        VkBufferUsageFlags       buffer_usage,
        VmaMemoryUsage           memory_usage,
        VmaAllocationCreateFlags flags = VMA_ALLOCATION_CREATE_MAPPED_BIT);

   

    VulkanBuffer(VulkanBuffer&& other);

    ~VulkanBuffer();

    VulkanBuffer& operator=(const VulkanBuffer&) = delete;
    VulkanBuffer& operator=(VulkanBuffer&&) = delete;
    VulkanBuffer(const VulkanBuffer&) = delete;

    VkBuffer getHandle() const{ return m_Buffer; }

    /**
   * @brief Maps vulkan memory if it isn't already mapped to an host visible address
   * @return Pointer to host visible memory
   */
    uint8_t* map();

    /**
     * @brief Unmaps vulkan memory from the host visible address
     */
    void unmap();

    /**
   * @brief Flushes memory if it is HOST_VISIBLE and not HOST_COHERENT
   */
    void flush() const;

    /**
   * @brief Copies byte data into the buffer
   * @param data The data to copy from
   * @param size The amount of bytes to copy
   * @param offset The offset to start the copying into the mapped data
   */
    void update(const uint8_t* data, size_t size, size_t offset = 0);

    /**
     * @brief Converts any non byte data into bytes and then updates the buffer
     * @param data The data to copy from
     * @param size The amount of bytes to copy
     * @param offset The offset to start the copying into the mapped data
     */
    void update(void* data, size_t size, size_t offset = 0) override;

    /**
     * @brief Copies a vector of bytes into the buffer
     * @param data The data vector to upload
     * @param offset The offset to start the copying into the mapped data
     */
    void update(const std::vector<uint8_t>& data, size_t offset = 0);
private:
  
    Device& m_Device;
    VkBuffer m_Buffer{ VK_NULL_HANDLE };

    

    VmaAllocation m_VmaAllocation{ VK_NULL_HANDLE };
    VkDeviceMemory m_Memory{ VK_NULL_HANDLE };
  
   

    /// Whether the buffer is persistently mapped or not
    bool m_Persistent{ false };

    /// Whether the buffer has been mapped with vmaMapMemory
    bool m_Mapped{ false };
};