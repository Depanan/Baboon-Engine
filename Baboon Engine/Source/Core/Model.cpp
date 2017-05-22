#include "Model.h"
#include <glm/gtc/matrix_transform.hpp>

void Model::Translate(const glm::vec3& i_TranslateVec)
{
	
	m_pInstanceUniforms->model = glm::translate(m_pInstanceUniforms->model, i_TranslateVec);
}

void Model::Scale(const glm::vec3& i_ScaleVec)
{

	m_pInstanceUniforms->model = glm::scale(m_pInstanceUniforms->model, i_ScaleVec);
}
