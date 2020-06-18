#pragma once
#include "Renderer/Common/GLMInclude.h"
#include "Core/Scene.h"
struct UBOCamera {
    glm::mat4 view;
    glm::mat4 proj;
    glm::mat4 inverseViewProj;
    glm::vec3 camPos;
};

class Buffer;
class Camera
{

public:
	virtual void Init() = 0;
  virtual void Update();
  void UpdateUniformBuffer(Buffer& buffer) const;
	virtual void UpdateProjectionMatrix(float newAspectRatio);
  bool GetDirty() const{ return m_Dirty; }
  void ClearDirty() { m_Dirty = false; }
 
  void CenterAt(glm::vec3 lookAt, glm::vec3 offset = glm::vec3(10.0,0.0,0.0), glm::vec3 forward = glm::vec3(0.0, 0.0, 1.0));
   UBOCamera& getCameraUBO() { return m_UBOCamera; }

	const glm::mat4& GetViewMatrix()const { return m_UBOCamera.view; }
	const glm::mat4& GetProjMatrix()const { return m_UBOCamera.proj; }
   glm::mat4 GetViewProjMatrix()const { return m_UBOCamera.proj * m_UBOCamera.view; }
	const glm::vec3& GetPosition()const { return m_UBOCamera.camPos; }
	const glm::vec3& GetForward()const { return m_CamForward; }


	

  protected:

	
  bool m_Dirty{ false };

  float m_Near;
  float m_Far;
  float m_Fov;
  float m_AspectRatio;

  glm::vec3 m_CamForward;
  glm::vec3 m_CamUp;
  glm::vec3 m_CamPosition;
  glm::vec3 m_CamLookAt;

  //In degrees
	glm::vec3 m_Rotation = glm::vec3();
  glm::vec3 m_RotationConstraints;


  UBOCamera m_UBOCamera;


};

class ShadowCamera : public Camera
{
public:
    void SetLightType(LightType ltype);
    void Init() override;
    void UpdateProjectionMatrix(float newAspectRatio) override;
private:
  LightType m_LightType{ LightType::LightType_Directional };

};
