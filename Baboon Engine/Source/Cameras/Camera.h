#pragma once
#include "Renderer/Common/GLMInclude.h"

struct UBOCamera {
    glm::mat4 view;
    glm::mat4 proj;
    glm::vec3 camPos;
};

class Buffer;
class Camera
{

public:
	void Init();
  virtual void Update();
  void UpdateUniformBuffer(Buffer& buffer) const;
	void UpdateProjectionMatrix(float newAspectRatio);
  bool GetDirty() const{ return m_Dirty; }
  void ClearDirty() { m_Dirty = false; }
 
  void Teleport(glm::vec3 newPosition, glm::vec3 lookAt);
   UBOCamera& getCameraUBO() { return m_UBOCamera; }

	const glm::mat4& GetViewMatrix()const { return m_UBOCamera.view; }
	const glm::mat4& GetProjMatrix()const { return m_UBOCamera.proj; }
	const glm::vec3& GetPosition()const { return m_UBOCamera.camPos; }
	const glm::vec3& GetForward()const { return m_CamForward; }


	static void moveLeft(void* i_pCam);
	static void moveRight(void* i_pCam);
	static void moveForward(void* i_pCam);
	static void moveBackwards(void* i_pCam);
  static void moveUp(void* i_pCam);
  static void moveDown(void* i_pCam);

	static void startRotation(void* i_pCam);
	static void rotate(void* i_pCam);
	static void endRotation(void* i_pCam);


  protected:

	float m_MoveSpeed ;
	float m_RotatingSpeed;
	bool m_bRotating;
  bool m_Dirty{ false };

  float m_Near;
  float m_Far;
  float m_Fov;
  float m_AspectRatio;

  glm::vec3 m_CamForward;
  glm::vec3 m_CamUp;
  glm::vec3 m_CamPosition;
  glm::vec3 m_CamPositionDelta;
  glm::vec3 m_CamLookAt;

  //In degrees
	glm::vec3 m_Rotation = glm::vec3();
  glm::vec3 m_RotationConstraints;


  UBOCamera m_UBOCamera;


};
