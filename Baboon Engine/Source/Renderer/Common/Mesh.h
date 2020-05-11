#pragma once

#include "Renderer/Common/GLMInclude.h"
#include "vulkan\vulkan.h"
#include <vector>
#include <string>
#include <unordered_map>

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


struct AttributeDescription
{
    VkFormat m_Format = VK_FORMAT_UNDEFINED;
    uint32_t m_Stride = 0;
    uint32_t m_Offset = 0;
    
};

class Buffer;
class ShaderVariant;
class Mesh
{
public:
    ~Mesh();
    Mesh(){}

    void setData(
             const std::unordered_map<std::string,AttributeDescription>& descriptions,
            const std::vector<glm::vec3>& positions,
            const std::vector<uint32_t>& indices,
            std::vector<glm::vec3>* colors = nullptr,
            std::vector<glm::vec2>* texCoords = nullptr,
            std::vector<glm::vec3>* normals = nullptr,
            std::vector<glm::vec3>* tangents = nullptr,
            std::vector<glm::vec3>* biTangents = nullptr
            );


    bool GetAttributeDescription(std::string name, AttributeDescription& attribute) const;
    void pushVertex(Vertex v);
    void pushIndex(uint32_t index);
    const Buffer* GetIndicesBuffer()const { return m_IndicesBuffer; }
    const std::unordered_map<std::string, std::pair<Buffer*, AttributeDescription>>& GetVerticesBuffers()const { return m_Buffers; }
    const size_t GetVerticesSize() { return sizeof(m_Vertices[0]) * m_Vertices.size(); }
    const size_t GetIndicesSize() { return sizeof(m_Indices[0]) * m_Indices.size(); }
    const uint32_t* GetIndicesData() const { return m_Indices.data(); };
    const glm::vec3* GetPositionsData() const { return m_Positions.data(); };

    void computeShaderVariant(ShaderVariant& variant) const;
private:
    std::vector<Vertex> m_Vertices;
    std::vector<uint32_t> m_Indices;
    Buffer* m_IndicesBuffer;

    std::unordered_map<std::string, std::pair<Buffer*,AttributeDescription>> m_Buffers;
    std::vector<glm::vec3> m_Positions;
    std::vector<glm::vec3> m_Colors;
    std::vector<glm::vec2> m_TexCoords;
    std::vector<glm::vec3> m_Normals;
    std::vector<glm::vec3> m_Tangents;
    std::vector<glm::vec3> m_BiTangents;
};





 

