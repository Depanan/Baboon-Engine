#pragma once

#include "Renderer\Common\Mesh.h"
#include "Core\Material.h"
#include "aabb.h"
#include <glm/gtc/quaternion.hpp> 
#include <glm/gtx/quaternion.hpp>



struct InstanceUBO {
	glm::mat4 model;

};



class Model
{
public:
  Model(const Mesh& mesh, MeshView meshView, std::string name = " ");
 

	const Mesh& GetMesh()const { return m_Mesh; }

  void SetMaterial(Material* i_Mat);
	Material* GetMaterial()const { return m_Material; }

  void SetTransform(glm::mat4 transformMat);

	void Scale(const glm::vec3& i_ScaleVec);
	void Translate(const glm::vec3& i_TranslateVec);
  void Rotate(const glm::vec3& i_Eulers);

  
  glm::vec3 GetTranslation() { return m_Translation; }
  glm::vec3 GetScale() { return m_Scale; }
  glm::vec3 GetRotation();


  bool IsVisible() { return true; }//TODO: implement visibility check here


  const glm::mat4& getModelMatrix()const { return m_InstanceUniforms.model; }
  const AABB& getAABB() { return m_AABB; }

  const ShaderVariant& getShaderVariant()const { return m_Variant; }
  void updateAABB();
  void computeModelMatrix();

  const uint32_t GetIndexStartPosition() const { return m_MeshView.m_IndicesMeshStart; }
  const uint32_t GetVertexStartPosition() const { return m_MeshView.m_VerticesMeshStart; }
  const uint32_t GetNIndices() const { return m_MeshView.m_NIndices; }
  const std::string& getName()const { return m_Name; }
  void SetSelection(bool select) { m_Selected = select; }

  bool GetDirty() { return m_Dirty; }
  void SetDirty() { m_Dirty = true; }
private:
   
  std::string m_Name;
	const Mesh& m_Mesh;
  bool m_Selected = false;
  
  MeshView m_MeshView;

	Material* m_Material = nullptr;
  ShaderVariant m_Variant;


  bool m_Dirty = false;

  AABB m_AABB;


  glm::vec3 m_Translation;
  glm::vec3 m_Scale;
  glm::quat m_Rotation;

	InstanceUBO m_InstanceUniforms;
	int m_iInstanceUniformIndex;

  void computeShaderVariant();
  
};
