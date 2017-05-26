#pragma once
#include "vulkan\vulkan.h"
#include "VKTypes.h"



class VKBuffer
{
public:
	VKBuffer() {}

	void UpdateDescriptor()
	{
		m_Descriptor.buffer = m_UniformBuffer;
		m_Descriptor.offset = 0;
		m_Descriptor.range = VK_WHOLE_SIZE;


	}

	void Create(const void*  i_data, size_t i_BufferSize, bool bKeepStaging = true);
	

	VKHandleWrapper<VkBuffer>& GetStagingBuffer() { return m_UniformStagingBuffer; }
	VKHandleWrapper<VkDeviceMemory>& GetStagingMemory() { return m_UniformStagingBufferMemory; }
	VKHandleWrapper<VkBuffer>&GetBuffer() { return m_UniformBuffer; }
	VKHandleWrapper<VkDeviceMemory>&GetMemory() { return m_UniformBufferMemory; }
	VkDescriptorBufferInfo& GetDescriptor() { return m_Descriptor; }
	VkDeviceSize GetBufferSize() { return m_BufferSize; }
private:

	VKHandleWrapper<VkBuffer>		m_UniformStagingBuffer = VK_NULL_HANDLE;
	VKHandleWrapper<VkDeviceMemory> m_UniformStagingBufferMemory = VK_NULL_HANDLE;
	VKHandleWrapper<VkBuffer>		m_UniformBuffer = VK_NULL_HANDLE;
	VKHandleWrapper<VkDeviceMemory> m_UniformBufferMemory = VK_NULL_HANDLE;
	VkDescriptorBufferInfo m_Descriptor;
	VkDeviceSize m_BufferSize;




};