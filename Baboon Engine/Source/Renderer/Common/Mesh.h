#pragma once

#include <glm\glm.hpp>
#include "vulkan\vulkan.h"
#include <vector>

struct Vertex {
	glm::vec3 pos;
	glm::vec3 color;
	glm::vec2 texCoord;
	glm::vec3 normal;

	static void GetVertexDescription(VkVertexInputBindingDescription* o_Description);
	static void GetAttributesDescription(std::vector<VkVertexInputAttributeDescription>& o_Description);
};

class Scene;
class Mesh
{
public:
    Mesh();

	
    void setScene(const Scene* scene);
	void SetMeshIndicesInfo (uint32_t iIndicesStart, uint32_t iIndicesCount,  uint32_t i_VerticesStart, uint32_t i_nVertices)
	{
		m_IndexStartPosition = iIndicesStart;
		m_NIndices = iIndicesCount;
		m_VertexStartPosition = i_VerticesStart;
    m_NVertices = i_nVertices;
	}

	uint32_t GetIndexStartPosition() { return m_IndexStartPosition; }
	uint32_t GetVertexStartPosition() { return m_VertexStartPosition; }
	uint32_t GetNIndices() { return m_NIndices; }

  void getIndicesData(const uint16_t** o_Indices, size_t* size);
  void getVertexData( const Vertex** o_Vertices, size_t* size);

private:

	uint32_t m_IndexStartPosition;//Position in the global index array
	uint32_t m_VertexStartPosition;//Position in the global index array
	uint32_t m_NIndices;//Number of indices
  uint32_t m_NVertices;

  const Scene* m_ScenePtr;

};


 

