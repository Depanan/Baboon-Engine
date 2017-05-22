#pragma once

#include <glm\glm.hpp>
#include "vulkan\vulkan.h"
#include <vector>

struct Vertex {
	glm::vec3 pos;
	glm::vec3 color;
	glm::vec2 texCoord;

	static void GetVertexDescription(VkVertexInputBindingDescription* o_Description);
	static void GetAttributesDescription(std::vector<VkVertexInputAttributeDescription>& o_Description);
};


class Mesh
{
public:
	Mesh() 
	{
		

	}

	

	void SetMeshIndicesInfo (uint16_t iIndicesStart, uint16_t iIndicesCount)
	{
		m_IndexStartPosition = iIndicesStart;
		m_NIndices = iIndicesCount;
	}

	uint16_t GetIndexStartPosition() { return m_IndexStartPosition; }
	uint16_t GetNIndices() { return m_NIndices; }

private:

	uint16_t m_IndexStartPosition;//Position in the global index array
	uint16_t m_NIndices;//Number of indices
	

};


 

