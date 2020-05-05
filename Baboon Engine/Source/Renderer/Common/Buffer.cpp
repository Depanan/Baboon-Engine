#include "Buffer.h"

Buffer::Buffer(uint64_t size):
    m_Size(size)
{
}

Buffer::Buffer(Buffer&& other):
    m_Size(other.m_Size),
    m_Mapped_Data(other.m_Mapped_Data)
{
    other.m_Mapped_Data = nullptr;
}
