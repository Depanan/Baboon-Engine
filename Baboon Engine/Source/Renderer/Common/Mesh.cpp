#include "Mesh.h"
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
	o_AttribDescription.resize(4);
	
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
}

Mesh::Mesh()
{
    m_ScenePtr = ServiceLocator::GetSceneManager()->GetCurrentScene();
}

void Mesh::setScene(const Scene* scene)
{
    m_ScenePtr = scene;
}

void Mesh::getIndicesData( const uint16_t** o_Indices, size_t* size)
{
  
    *o_Indices = m_ScenePtr->GetIndicesData() + m_VertexStartPosition;
    *size = m_NIndices;
   
}
void Mesh::getVertexData(  const Vertex** o_Vertices, size_t* size)
{
    *o_Vertices = m_ScenePtr->GetVerticesData()+ m_VertexStartPosition;
    *size = m_NVertices;

}
