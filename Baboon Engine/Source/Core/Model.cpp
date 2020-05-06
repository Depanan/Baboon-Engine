#include "Model.h"
#include "Renderer/Common/GLMInclude.h"
#include <algorithm>

Model::Model(const Mesh& i_Mesh, uint32_t iIndicesStart, uint32_t iIndicesCount, uint32_t i_VerticesStart, uint32_t i_nVertices):
    m_Mesh(i_Mesh),
    m_IndexStartPosition(iIndicesStart),
    m_NIndices(iIndicesCount),
    m_VertexStartPosition(i_VerticesStart),
    m_NVertices(i_nVertices)
{
    m_AABB.reset();
}

void Model::SetMaterial(Material* i_Mat)
{
    m_Material = i_Mat;
    computeShaderVariant();
}

void Model::Translate(const glm::vec3& i_TranslateVec)
{
	
	m_InstanceUniforms.model = glm::translate(m_InstanceUniforms.model, i_TranslateVec);
  updateAABB();
}

void Model::Scale(const glm::vec3& i_ScaleVec)
{

	m_InstanceUniforms.model = glm::scale(m_InstanceUniforms.model, i_ScaleVec);
  updateAABB();
}
void Model::computeShaderVariant()
{

    if (m_Material != nullptr)
    {
        for (auto& texture : *m_Material->getTextures())
        {
            std::string tex_name = texture.first;
            std::transform(tex_name.begin(), tex_name.end(), tex_name.begin(), ::toupper);

            m_Variant.add_define("HAS_" + tex_name);
        }
    }

}
void Model::updateAABB()
{
     
    const uint32_t* indices = m_Mesh.GetIndicesData() + m_IndexStartPosition;
    const Vertex* vertices = m_Mesh.GetVertexData() + m_VertexStartPosition;
    
    m_AABB.update(vertices, m_NVertices, indices, m_NIndices);
    m_AABB.transform(m_InstanceUniforms.model);
}