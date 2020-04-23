#pragma once
#include <glm\glm.hpp>

struct UBOCamera {
    glm::mat4 view;
    glm::mat4 proj;
};

class Buffer;
class Camera
{

public:
	void Init();
	
  void Update();
	void UpdateViewMatrix();
	void UpdateProjectionMatrix(float newAspectRatio);
  bool GetDirty() const{ return m_Dirty; }
  void ClearDirty() { m_Dirty = false; }


  const Buffer* GetCameraUniformBuffer()const { return m_CameraUniformBuffer; }
	const glm::mat4& GetViewMatrix()const { return m_UBOCamera.view; }
	const glm::mat4& GetProjMatrix()const { return m_UBOCamera.proj; }
	const glm::vec3& GetPosition()const { return m_Position; }
	const glm::vec3& GetRotation()const { return m_Rotation; }


	static void moveLeft(void* i_pCam);
	static void moveRight(void* i_pCam);
	static void moveForward(void* i_pCam);
	static void moveBackwards(void* i_pCam);

	static void startRotation(void* i_pCam);
	static void rotate(void* i_pCam);
	static void endRotation(void* i_pCam);


private:

	float m_MoveSpeed ;
	float m_RotatingSpeed;
	bool m_bRotating;
  bool m_Dirty{ false };

	glm::vec3 m_Rotation = glm::vec3();
	glm::vec3 m_Position = glm::vec3();


  UBOCamera m_UBOCamera;
  Buffer* m_CameraUniformBuffer;


};
