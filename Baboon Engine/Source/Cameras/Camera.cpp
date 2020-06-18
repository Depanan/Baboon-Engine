#include "Camera.h"
#include "Core\Input.h"
#include "Core\ServiceLocator.h"

#include "glm\ext.hpp"
#include "Renderer/Common/Buffer.h"


void Camera::Init()
{
  

}
void Camera::Update()
{
    if (m_Dirty)
    {
        m_UBOCamera.camPos = m_CamPosition;
        m_UBOCamera.view = glm::lookAt(m_CamPosition, m_CamLookAt, m_CamUp);
        m_UBOCamera.inverseViewProj = glm::inverse(m_UBOCamera.proj * m_UBOCamera.view);
        ServiceLocator::GetCameraManager()->GetSubject().Notify(Subject::CAMERADIRTY, this);
        m_Dirty = false;
    }
}
void Camera::UpdateProjectionMatrix(float newAspectRatio)
{
  m_AspectRatio = newAspectRatio;
  auto vfov = static_cast<float>(2 * atan(tan(m_Fov / 2) * (1.0 / m_AspectRatio)));
  m_Fov = newAspectRatio > 1.0f ? m_Fov : vfov;

  m_UBOCamera.proj = glm::perspective(m_Fov, m_AspectRatio, m_Near, m_Far);
  m_UBOCamera.proj[1][1] *= -1;//GLM was originally designed for OpenGL, where the Y coordinate of the clip coordinates is inverted.

  m_Dirty = true;
}

void Camera::CenterAt(glm::vec3 lookAt, glm::vec3 offset, glm::vec3 forward)
{
    m_CamPosition = lookAt - offset;
    m_CamLookAt = lookAt;
    m_CamForward = forward;
    m_Rotation = glm::vec3(0.0);

    m_Dirty = true;
}
void Camera::UpdateUniformBuffer(Buffer& buffer) const
{
    buffer.update((void *)&m_UBOCamera, sizeof(UBOCamera));    
}

void ShadowCamera::SetLightType(LightType ltype)
{
  m_LightType = ltype;
}

void ShadowCamera::Init()
{
    m_Near = 0.1f;
    m_Far = 10000.0f;
    m_Fov = glm::radians(100.0f);
    m_AspectRatio = 1;//shadowmaps are square
    UpdateProjectionMatrix(m_AspectRatio);

    m_UBOCamera.view = glm::mat4();
    m_Rotation = glm::vec3(0.0f);
    m_RotationConstraints = glm::vec3(90.0f, 360.0f, 0.0f);

    m_CamPosition = glm::vec3(0.0f);
    m_CamForward = glm::vec3(0, 0, 1.0f);
    m_CamLookAt = m_CamPosition + m_CamForward * 1.0f;
    m_CamUp = glm::vec3(0.0, 1.0, 0.0f);
}
void ShadowCamera::UpdateProjectionMatrix(float newAspectRatio)
{
 /* m_AspectRatio = newAspectRatio;
  auto vfov = static_cast<float>(2 * atan(tan(m_Fov / 2) * (1.0 / m_AspectRatio)));
  m_Fov = newAspectRatio > 1.0f ? m_Fov : vfov;


  if (m_LightType == LightType::LightType_Directional)
  {
    const AABB& sceneBox = ServiceLocator::GetSceneManager()->GetCurrentScene()->getSceneAABB();
    m_UBOCamera.proj = glm::ortho(sceneBox.get_min().x, sceneBox.get_max().x, sceneBox.get_min().y, sceneBox.get_max().y, sceneBox.get_min().z, sceneBox.get_max().z);
    m_UBOCamera.proj[1][1] *= -1;//GLM was originally designed for OpenGL, where the Y coordinate of the clip coordinates is inverted.
  }


  m_Dirty = true;*/
  Camera::UpdateProjectionMatrix(newAspectRatio);
}
