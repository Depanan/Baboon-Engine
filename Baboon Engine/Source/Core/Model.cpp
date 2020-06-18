#include "Model.h"
#include "Renderer/Common/GLMInclude.h"
#include <glm\gtx\matrix_decompose.hpp>
#include <algorithm>
#include <Core/ServiceLocator.h>
#include "Core/Scene.h"

Model::Model(const Mesh& i_Mesh, MeshView meshView, Scene& parentScene,  std::string name ):
    m_Mesh(i_Mesh),
    m_MeshView(meshView),
    m_ParentScene(parentScene),
    m_Name(name)
{
    m_AABB.reset();
    m_Translation = glm::vec3(0.0);
    m_Scale = glm::vec3(1.0);
    m_Rotation = glm::quat();
}

void Model::SetMaterial(Material* i_Mat)
{
    m_Material = i_Mat;
    m_InstanceUniforms.matId = i_Mat->getMaterialIndex();
    computeShaderVariant();
}

glm::vec3 Model::GetRotation()
{
    return glm::degrees(glm::eulerAngles(m_Rotation));
}



void Model::Rotate(const glm::vec3& i_Eulers)
{
    m_Rotation = glm::quat(glm::radians(i_Eulers));
    m_Dirty = true;
    m_ParentScene.SetDirty();//All this dirty stuff can be improved with a tree and the models hanging from the Scene root node propagating dirty upwards

}
void Model::Translate(const glm::vec3& i_TranslateVec)
{
  m_Translation = i_TranslateVec;
  m_Dirty = true;
  m_ParentScene.SetDirty();

}

void Model::Scale(const glm::vec3& i_ScaleVec)
{
  m_Scale = i_ScaleVec;
  m_Dirty = true;
  m_ParentScene.SetDirty();

}


void Model::SetTransform(glm::mat4 transformMat)
{
    m_InstanceUniforms.model = transformMat;
    glm::vec3 skew;
    glm::vec4 persp;
    glm::decompose(transformMat, m_Scale, m_Rotation, m_Translation, skew, persp);

    m_Dirty = true;
    m_ParentScene.SetDirty();
    
}

void Model::computeModelMatrix()
{
    glm::mat4 rotationMatrix = glm::toMat4(m_Rotation);
    glm::mat4 scaleMatrix;
    scaleMatrix = glm::scale(scaleMatrix, m_Scale);
    glm::mat4 translationMatrix;
    translationMatrix = glm::translate(translationMatrix, -m_Translation);
    m_InstanceUniforms.model = translationMatrix * rotationMatrix * scaleMatrix;

    //ServiceLocator::GetRenderer()->SceneDirty();//TODO:: do this better, again, listener? scene traversals?
    updateAABB();
    m_Dirty = false;
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
        m_Mesh.computeShaderVariant(m_Variant);
    }

}
void Model::updateAABB()
{
     
    const uint32_t* indices = m_Mesh.GetIndicesData() + m_MeshView.m_IndicesMeshStart;
    const glm::vec3* verticesPos = m_Mesh.GetPositionsData() + m_MeshView.m_VerticesMeshStart;
    
    m_AABB.reset();//TODO: Do I have to do all this everytime I transform? is going thru the mesh necessr?
    m_AABB.update(verticesPos, m_MeshView.m_NVertices, indices, m_MeshView.m_NIndices);
    m_AABB.transform(m_InstanceUniforms.model);
}