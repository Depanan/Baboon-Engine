#pragma once
#include <glm\glm.hpp>
class Camera
{

public:
	void Init();
	
	void UpdateViewMatrix();
	
	
	const glm::mat4& GetViewMatrix()const { return m_ViewMat; }
	const glm::mat4& GetProjMatrix()const { return m_ProjMat; }
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

	glm::vec3 m_Rotation = glm::vec3();
	glm::vec3 m_Position = glm::vec3();


	glm::mat4 m_ViewMat;
	glm::mat4 m_ProjMat;


};
