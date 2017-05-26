#include "VKBuffer.h"
#include "Core\ServiceLocator.h"
#include "RendererVulkan.h"

void VKBuffer::Create(const void*  i_data, size_t i_BufferSize, bool bKeepStaging)
{

	RendererVulkan* pRenderer = (RendererVulkan*)ServiceLocator::GetRenderer();
	m_UniformStagingBuffer = { pRenderer->m_LogicalDevice, vkDestroyBuffer };
	m_UniformStagingBufferMemory = { pRenderer->m_LogicalDevice, vkFreeMemory };
	m_UniformBuffer = { pRenderer->m_LogicalDevice, vkDestroyBuffer };
	m_UniformBufferMemory = { pRenderer->m_LogicalDevice, vkFreeMemory };
	m_BufferSize = i_BufferSize;


	pRenderer->createVKBuffer(i_BufferSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, m_UniformStagingBuffer, m_UniformStagingBufferMemory);
	pRenderer->createVKBuffer(i_BufferSize, VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, m_UniformBuffer, m_UniformBufferMemory);
	UpdateDescriptor();

	if (!bKeepStaging)//For vertex and index buffers we wanna create upload and delete staging
	{
		void* data;
		vkMapMemory(pRenderer->m_LogicalDevice, m_UniformStagingBufferMemory, 0, i_BufferSize, 0, &data);//Get a pointer to the device memory
		memcpy(data, i_data, i_BufferSize);
		vkUnmapMemory(pRenderer->m_LogicalDevice, m_UniformStagingBufferMemory);//Unmap the memory

		pRenderer->copyVKBuffer(m_UniformStagingBuffer, m_UniformBuffer, i_BufferSize);

		vkDestroyBuffer(pRenderer->m_LogicalDevice, m_UniformStagingBuffer, nullptr);
		vkFreeMemory(pRenderer->m_LogicalDevice, m_UniformStagingBufferMemory, nullptr);
	}

}