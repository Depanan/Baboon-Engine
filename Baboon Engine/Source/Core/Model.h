#pragma once

#include "Renderer\Common\Mesh.h"
#include "Core\Material.h"
struct InstanceUBO {
	glm::mat4 model;

};

struct UBO {
    glm::mat4 model;
    glm::mat4 view;
    glm::mat4 proj;
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

private:
	Mesh* m_Mesh;
	Material* m_Material;

	InstanceUBO* m_pInstanceUniforms = nullptr;
	int m_iInstanceUniformIndex;
};
