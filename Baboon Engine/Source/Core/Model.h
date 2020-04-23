#pragma once

#include "Renderer\Common\Mesh.h"
#include "Core\Material.h"
struct InstanceUBO {
	glm::mat4 model;

};



class Model
{
public:
	Model(){}
	void SetMesh(Mesh* i_Mesh) { m_Mesh = i_Mesh; }
	Mesh* GetMesh() { return m_Mesh; }

	void SetMaterial(Material* i_Mat) { m_Material = i_Mat; }
	Material* GetMaterial() { return m_Material; }

	void SetInstanceUniforms(InstanceUBO* i_InstanceUniforms, int i_Index) {
		m_pInstanceUniforms = i_InstanceUniforms;
		m_pInstanceUniforms->model = glm::mat4();
		m_iInstanceUniformIndex = i_Index;
	}

	void Scale(const glm::vec3& i_ScaleVec);
	void Translate(const glm::vec3& i_TranslateVec);
	glm::vec3 GetPosition() { return m_pInstanceUniforms->model[4]; }

  const glm::mat4& getModelMatrix()const { return m_pInstanceUniforms->model; }
private:
	Mesh* m_Mesh;
	Material* m_Material;

	InstanceUBO* m_pInstanceUniforms = nullptr;
	int m_iInstanceUniformIndex;
};
