#pragma once

#include <stdint.h>

class Buffer
{
public:
    virtual ~Buffer() {}
    Buffer(uint64_t size);
    Buffer(Buffer&& other);
    Buffer& operator=(const Buffer&) = delete;
    Buffer& operator=(Buffer&&) = delete;
    Buffer(const Buffer&) = delete;
    
    virtual void update(void* data, size_t size, size_t offset = 0) = 0;
protected:

    uint8_t* m_Mapped_Data{ nullptr };
    uint64_t m_Size{ 0 };
	
	

};


 

