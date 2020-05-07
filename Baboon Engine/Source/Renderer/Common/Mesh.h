#pragma once

#include "Renderer/Common/GLMInclude.h"
#include "vulkan\vulkan.h"
#include <vector>

struct MeshView
{
    uint32_t m_IndicesMeshStart;
    uint32_t m_NIndices;
    uint32_t m_VerticesMeshStart;
    uint32_t m_NVertices;
    uint32_t m_MaterialIndex;
};

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

class Buffer;
class Mesh
{
public:
    ~Mesh();
    Mesh(){}
    Mesh(std::vector<Vertex> vertices, std::vector<uint32_t> indices);
    void pushVertex(Vertex v);
    void pushIndex(uint32_t index);
    const Buffer* GetIndicesBuffer()const { return m_IndicesBuffer; }
    const Buffer* GetVerticesBuffer()const { return m_VerticesBuffer; }
    const size_t GetVerticesSize() { return sizeof(m_Vertices[0]) * m_Vertices.size(); }
    const size_t GetIndicesSize() { return sizeof(m_Indices[0]) * m_Indices.size(); }
    const uint32_t* GetIndicesData() const { return m_Indices.data(); };
    const Vertex* GetVertexData() const { return m_Vertices.data(); };
    void uploadBuffers();
private:
    std::vector<Vertex> m_Vertices;
    std::vector<uint32_t> m_Indices;
    Buffer* m_VerticesBuffer;
    Buffer* m_IndicesBuffer;
};





 

