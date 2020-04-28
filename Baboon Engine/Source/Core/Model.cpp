#include "Model.h"
#include <glm/gtc/matrix_transform.hpp>
#include <algorithm>

void Model::SetMesh(Mesh* i_Mesh)
{
    m_Mesh = i_Mesh;
    updateAABB();
}

void Model::SetMaterial(Material* i_Mat)
{
    m_Material = i_Mat;
    computeShaderVariant();
}

void Model::Translate(const glm::vec3& i_TranslateVec)
{
	
	m_pInstanceUniforms->model = glm::translate(m_pInstanceUniforms->model, i_TranslateVec);
  updateAABB();
}

void Model::Scale(const glm::vec3& i_ScaleVec)
{

	m_pInstanceUniforms->model = glm::scale(m_pInstanceUniforms->model, i_ScaleVec);
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
    const uint16_t* indices = nullptr;
    size_t nIndices;
    m_Mesh->getIndicesData(&indices, &nIndices);
    const Vertex* vertices = nullptr;
    size_t nVertices;
    m_Mesh->getVertexData(&vertices, &nVertices);
    m_AABB.update(vertices, nVertices, indices, nIndices);
    m_AABB.transform(m_pInstanceUniforms->model);
}