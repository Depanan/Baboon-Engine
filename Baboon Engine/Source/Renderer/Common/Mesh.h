#pragma once

#include "Renderer/Common/GLMInclude.h"
#include "vulkan\vulkan.h"
#include <vector>

struct Vertex {
	glm::vec3 pos;
	glm::vec3 color;
	glm::vec2 texCoord;
	glm::vec3 normal;
  glm::vec3 tangent;
  glm::vec3 biTangent;

	static void GetVertexDescription(VkVertexInputBindingDescription* o_Description);
	static void GetAttributesDescription(std::vector<VkVertexInputAttributeDescription>& o_Description);
};

class Scene;
class Mesh
{
public:
    Mesh(const Scene& scene, const Vertex* ,uint32_t iIndicesStart, uint32_t iIndicesCount, uint32_t i_VerticesStart, uint32_t i_nVertices);

	
    
	

    const uint32_t GetIndexStartPosition() const { return m_IndexStartPosition; }
  const uint32_t GetVertexStartPosition() const { return m_VertexStartPosition; }
	const uint32_t GetNIndices() const { return m_NIndices; }

  const void getIndicesData(const uint32_t** o_Indices, size_t* size) const;
  const void getVertexData( const Vertex** o_Vertices, size_t* size) const;

private:

	uint32_t m_IndexStartPosition;//Position in the global index array
	uint32_t m_VertexStartPosition;//Position in the global index array
	uint32_t m_NIndices;//Number of indices
  uint32_t m_NVertices;
  const Vertex* m_Vertices;

  const Scene& m_Scene;

};


 

