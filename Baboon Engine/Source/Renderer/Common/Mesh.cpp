#include "Mesh.h"
#include "Buffer.h"
#include "defines.h"
#include "Core/ServiceLocator.h"

void Vertex::GetVertexDescription(VkVertexInputBindingDescription* o_Description)
{
	o_Description-> binding = 0;
  o_Description->stride = sizeof(Vertex);
	o_Description-> inputRate = VK_VERTEX_INPUT_RATE_VERTEX;

}
void Vertex::GetAttributesDescription(std::vector<VkVertexInputAttributeDescription>& o_AttribDescription)
{
   // o_AttribDescription.resize(3);
	o_AttribDescription.resize(6);
	
	//Position
	o_AttribDescription[0].binding = 0;
	o_AttribDescription[0].location = 0;
	o_AttribDescription[0].format = VK_FORMAT_R32G32B32_SFLOAT;
	o_AttribDescription[0].offset = offsetof(Vertex, pos);

	//Color
	o_AttribDescription[1].binding = 0;
	o_AttribDescription[1].location = 1;
	o_AttribDescription[1].format = VK_FORMAT_R32G32B32_SFLOAT;
	o_AttribDescription[1].offset = offsetof(Vertex, color);
  

	//TexCoords
	o_AttribDescription[2].binding = 0;
	o_AttribDescription[2].location = 2;
	o_AttribDescription[2].format = VK_FORMAT_R32G32_SFLOAT;
	o_AttribDescription[2].offset = offsetof(Vertex, texCoord);

	//Normal
	o_AttribDescription[3].binding = 0;
	o_AttribDescription[3].location = 3;
	o_AttribDescription[3].format = VK_FORMAT_R32G32B32_SFLOAT;
	o_AttribDescription[3].offset = offsetof(Vertex, normal);

  //Tangent
  o_AttribDescription[4].binding = 0;
  o_AttribDescription[4].location = 4;
  o_AttribDescription[4].format = VK_FORMAT_R32G32B32_SFLOAT;
  o_AttribDescription[4].offset = offsetof(Vertex, tangent);

  //biTangent
  o_AttribDescription[5].binding = 0;
  o_AttribDescription[5].location = 5;
  o_AttribDescription[5].format = VK_FORMAT_R32G32B32_SFLOAT;
  o_AttribDescription[5].offset = offsetof(Vertex, biTangent);
}


Mesh::~Mesh()
{
    auto renderer = ServiceLocator::GetRenderer();
    renderer->DeleteBuffer(m_IndicesBuffer);
    renderer->DeleteBuffer(m_VerticesBuffer);
}

void Mesh::uploadBuffers()
{
    auto renderer = ServiceLocator::GetRenderer();
    m_VerticesBuffer = renderer->CreateVertexBuffer((void*)(m_Vertices.data()), GetVerticesSize());
    m_IndicesBuffer = renderer->CreateIndexBuffer((void*)(m_Indices.data()), GetIndicesSize());
}

Mesh::Mesh(std::vector<Vertex> vertices, std::vector<uint32_t> indices):
m_Vertices(vertices),
m_Indices(indices)
{
    uploadBuffers();
}

void Mesh::pushVertex(Vertex v)
{
    m_Vertices.push_back(v);
}
void Mesh::pushIndex(uint32_t index)
{
    m_Indices.push_back(index);
}
