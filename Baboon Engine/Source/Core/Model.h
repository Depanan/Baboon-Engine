#pragma once

#include "Renderer\Common\Mesh.h"
#include "Core\Material.h"
#include "aabb.h"
struct InstanceUBO {
	glm::mat4 model;

};



class Model
{
public:
  Model(const Mesh& mesh, uint32_t iIndicesStart, uint32_t iIndicesCount, uint32_t i_VerticesStart, uint32_t i_nVertices);
 

	const Mesh& GetMesh() { return m_Mesh; }

  void SetMaterial(Material* i_Mat);
	Material* GetMaterial() { return m_Material; }


	void Scale(const glm::vec3& i_ScaleVec);
	void Translate(const glm::vec3& i_TranslateVec);
	glm::vec3 GetPosition() { return m_InstanceUniforms.model[4]; }

  const glm::mat4& getModelMatrix()const { return m_InstanceUniforms.model; }
  const AABB& getAABB() { return m_AABB; }

  const ShaderVariant& getShaderVariant()const { return m_Variant; }
  void updateAABB();

  const uint32_t GetIndexStartPosition() const { return m_IndexStartPosition; }
  const uint32_t GetVertexStartPosition() const { return m_VertexStartPosition; }
  const uint32_t GetNIndices() const { return m_NIndices; }

private:
   
	const Mesh& m_Mesh;
  uint32_t m_IndexStartPosition;//Position in the global index array
  uint32_t m_VertexStartPosition;//Position in the global index array
  uint32_t m_NIndices;//Number of indices
  uint32_t m_NVertices;


	Material* m_Material;
  ShaderVariant m_Variant;



  AABB m_AABB;
	InstanceUBO m_InstanceUniforms;
	int m_iInstanceUniformIndex;

  void computeShaderVariant();
  
};
