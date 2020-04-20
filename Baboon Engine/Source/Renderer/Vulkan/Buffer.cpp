#include "Buffer.h"
#include "Device.h"
#include "Core/ServiceLocator.h"

//NOTE THAT ALL THE SIZE PARAMETERS ARE IN BYTES! AS IN SIZEOF(TYPE) * ARRAY<TYPE>.size()
Buffer::Buffer(Device& device,
    VkDeviceSize             size,
    VkBufferUsageFlags       buffer_usage,
    VmaMemoryUsage           memory_usage,
    VmaAllocationCreateFlags flags):
    m_Device(device),
    m_Size(size)
{
    m_Persistent = (flags & VMA_ALLOCATION_CREATE_MAPPED_BIT) != 0;

    VkBufferCreateInfo buffer_info{ VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO };
    buffer_info.usage = buffer_usage;
    buffer_info.size = size;

    VmaAllocationCreateInfo memory_info{};
    memory_info.flags = flags;
    memory_info.usage = memory_usage;

    VmaAllocationInfo allocation_info{};
    auto              result = vmaCreateBuffer(device.getMemoryAllocator(),
        &buffer_info, &memory_info,
        &m_Buffer, &m_VmaAllocation,
        &allocation_info);

    if (result != VK_SUCCESS)
    {
        LOGERROR("Cannot create Buffer" );
    }

    m_Memory = allocation_info.deviceMemory;

    if (m_Persistent)
    {
        m_Mapped_Data = static_cast<uint8_t*>(allocation_info.pMappedData);
    }

}



Buffer::Buffer(Buffer&& other) :
    m_Device{ other.m_Device },
    m_Buffer{ other.m_Buffer },
    m_VmaAllocation{ other.m_VmaAllocation },
    m_Memory{ other.m_Memory },
    m_Size{ other.m_Size },
    m_Mapped_Data{ other.m_Mapped_Data },
    m_Mapped{ other.m_Mapped }
{
    // Reset other handles to avoid releasing on destruction
    other.m_Buffer = VK_NULL_HANDLE;
    other.m_VmaAllocation= VK_NULL_HANDLE;
    other.m_Memory = VK_NULL_HANDLE;
    other.m_Mapped_Data = nullptr;
    other.m_Mapped = false;
}

Buffer::~Buffer()
{
    if (m_Buffer != VK_NULL_HANDLE && m_VmaAllocation != VK_NULL_HANDLE)
    {
        unmap();
        vmaDestroyBuffer(m_Device.getMemoryAllocator(), m_Buffer, m_VmaAllocation);
    }
}


uint8_t* Buffer::map()
{
    if (!m_Mapped && !m_Mapped_Data)
    {
       vmaMapMemory(m_Device.getMemoryAllocator(), m_VmaAllocation, reinterpret_cast<void**>(&m_Mapped_Data));
        m_Mapped = true;
    }
    return m_Mapped_Data;
}

void Buffer::unmap()
{
    if (m_Mapped)
    {
        vmaUnmapMemory(m_Device.getMemoryAllocator(), m_VmaAllocation);
        m_Mapped_Data = nullptr;
        m_Mapped = false;
    }
}

void Buffer::flush() const
{
    vmaFlushAllocation(m_Device.getMemoryAllocator(), m_VmaAllocation, 0, m_Size);
}

void Buffer::update(const std::vector<uint8_t>& data, size_t offset)
{
    update(data.data(), data.size(), offset);
}

void Buffer::update(void* data, size_t size, size_t offset)
{
    update(reinterpret_cast<const uint8_t*>(data), size, offset);
}

void Buffer::update(const uint8_t* data, const size_t size, const size_t offset)
{
    if (m_Persistent)
    {
        std::copy(data, data + size, m_Mapped_Data + offset);
        flush();
    }
    else
    {
        map();
        std::copy(data, data + size, m_Mapped_Data + offset);
        flush();
        unmap();
    }
}