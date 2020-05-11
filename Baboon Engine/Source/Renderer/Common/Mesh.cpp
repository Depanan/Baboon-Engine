#include "Mesh.h"
#include "Buffer.h"
#include "defines.h"
#include "Core/ServiceLocator.h"
#include "Core/Material.h"

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
   
    for (auto buffer : m_Buffers)
    {
        renderer->DeleteBuffer(buffer.second.first);
    }
}





void Mesh::setData( const std::unordered_map<std::string, AttributeDescription>& descriptions,
                    const std::vector<glm::vec3>& positions,  
                    const std::vector<uint32_t>& indices,
                    std::vector<glm::vec3>* colors,
                    std::vector<glm::vec2>* texCoords,
                    std::vector<glm::vec3>* normals,
                    std::vector<glm::vec3>* tangents,
                    std::vector<glm::vec3>* biTangents)
{
    auto renderer = ServiceLocator::GetRenderer();
    //assert(positions.size() > 0, "Cant have mesh with no positions");
    m_Positions = std::move(positions);
    m_Buffers.emplace("inPosition", std::make_pair(renderer->CreateVertexBuffer((void*)(m_Positions.data()), sizeof(glm::vec3) * m_Positions.size()), descriptions.find("inPosition")->second));




    //assert(m_Indices.size() > 0, "Cant have mesh with no indices for now");
    m_Indices =  std::move(indices);
    m_IndicesBuffer = renderer->CreateIndexBuffer((void*)(m_Indices.data()), sizeof(uint32_t)* m_Indices.size());



    if (colors && colors->size())
    {
        m_Colors = std::move(*colors);
        m_Buffers.emplace("inColor", std::make_pair(renderer->CreateVertexBuffer((void*)(m_Colors.data()), sizeof(glm::vec3) * m_Colors.size()), descriptions.find("inColor")->second));
    }
    if (texCoords && texCoords->size())
    {
        m_TexCoords = std::move(*texCoords);
        m_Buffers.emplace("inTexCoord", std::make_pair(renderer->CreateVertexBuffer((void*)(m_TexCoords.data()), sizeof(glm::vec2) * m_TexCoords.size()), descriptions.find("inTexCoord")->second));
    }
    if (normals && normals->size())
    {
        m_Normals = std::move(*normals);
        m_Buffers.emplace("inNormal", std::make_pair(renderer->CreateVertexBuffer((void*)(m_Normals.data()), sizeof(glm::vec3) * m_Normals.size()), descriptions.find("inNormal")->second));
    }
    if (tangents && tangents->size())
    {
        m_Tangents = std::move(*tangents);
        m_Buffers.emplace("inTangent", std::make_pair(renderer->CreateVertexBuffer((void*)(m_Tangents.data()), sizeof(glm::vec3) * m_Tangents.size()), descriptions.find("inTangent")->second));
    }
    if (biTangents && biTangents->size())
    {
        m_BiTangents = std::move(*biTangents);
        m_Buffers.emplace("inBiTangent", std::make_pair(renderer->CreateVertexBuffer((void*)(m_BiTangents.data()), sizeof(glm::vec3) * m_BiTangents.size()), descriptions.find("inBiTangent")->second));
    }
}

bool Mesh::GetAttributeDescription(std::string name, AttributeDescription& attribute)const
{
    auto it = m_Buffers.find(name);
    if (it != m_Buffers.end())
    {
        attribute = it->second.second;
        return true;
       
    }
    return false;
}

void Mesh::pushVertex(Vertex v)
{
    m_Vertices.push_back(v);
}
void Mesh::pushIndex(uint32_t index)
{
    m_Indices.push_back(index);
}

void Mesh::computeShaderVariant(ShaderVariant& variant) const
{
    for (auto& buffer : m_Buffers)
    {
        std::string attrib_name = buffer.first;
        std::transform(attrib_name.begin(), attrib_name.end(), attrib_name.begin(), ::toupper);
        variant.add_define("HAS_" + attrib_name);
    }
}
