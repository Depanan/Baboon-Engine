#include "Camera.h"
#include "Core\Input.h"
#include "Core\ServiceLocator.h"
#define GLM_FORCE_RADIANS
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include "Renderer/Common/Buffer.h"


void Camera::Init()
{
  size_t size = sizeof(UBOCamera);
  m_CameraUniformBuffer = ServiceLocator::GetRenderer()->CreateStaticUniformBuffer(nullptr, size);
  m_UBOCamera.view = glm::mat4();
	UpdateProjectionMatrix(ServiceLocator::GetRenderer()->GetMainRTWidth() / ServiceLocator::GetRenderer()->GetMainRTHeight());

  

	m_MoveSpeed = 100.0f;
	m_RotatingSpeed = 10.0f;
	m_bRotating = false;

	
	
	//Map functions to input:

	Input* pInput = ServiceLocator::GetInput();

	pInput->MapKey(GLFW_KEY_A, Camera::moveLeft,this);
	pInput->MapKey(GLFW_KEY_D, Camera::moveRight, this);
	pInput->MapKey(GLFW_KEY_W, Camera::moveForward, this);
	pInput->MapKey(GLFW_KEY_S, Camera::moveBackwards, this);

	pInput->MapMouseButtonClicked(1, Camera::startRotation, this);
	pInput->MapMouseMoved(Camera::rotate, this);
	pInput->MapMouseButtonReleased(1, Camera::endRotation, this);

}
void Camera::UpdateProjectionMatrix(float newAspectRatio)
{
	float fNear = 0.1f;
	float fFar = 10000.0f;
	float fFov = glm::radians(60.0f);

  auto vfov = static_cast<float>(2 * atan(tan(fFov / 2) * (1.0 / newAspectRatio)));
  fFov = newAspectRatio > 1.0f ? fFov : vfov;

  m_UBOCamera.proj = glm::perspective(fFov, newAspectRatio, fNear, fFar);//TODO:: Make far near and fov config
  m_UBOCamera.proj[1][1] *= -1;//GLM was originally designed for OpenGL, where the Y coordinate of the clip coordinates is inverted.

  m_Dirty = true;
}
void Camera::Update()
{
    if (m_Dirty)
    {
        m_CameraUniformBuffer->update(&m_UBOCamera, sizeof(UBOCamera));
    }
    
}
void Camera::UpdateViewMatrix()
{
	glm::mat4 rotM = glm::mat4();
	glm::mat4 transM;

	rotM = glm::rotate(rotM, glm::radians(m_Rotation.x), glm::vec3(1.0f, 0.0f, 0.0f));
	rotM = glm::rotate(rotM, glm::radians(m_Rotation.y), glm::vec3(0.0f, 1.0f, 0.0f));
	rotM = glm::rotate(rotM, glm::radians(m_Rotation.z), glm::vec3(0.0f, 0.0f, 1.0f));

	transM = glm::translate(glm::mat4(), m_Position);

	
  m_UBOCamera.view = rotM * transM;

  m_Dirty = true;
}

void Camera::moveLeft(void* i_pCam)
{
	Camera* pThis = (Camera*)i_pCam;

	//TODO: store camFront to avoid this
	glm::vec3 camFront;
	camFront.x = -cos(glm::radians(pThis->m_Rotation.x)) * sin(glm::radians(pThis->m_Rotation.y));
	camFront.y = sin(glm::radians(pThis->m_Rotation.x));
	camFront.z = cos(glm::radians(pThis->m_Rotation.x)) * cos(glm::radians(pThis->m_Rotation.y));
	camFront = glm::normalize(camFront);
	/////////////////

	pThis->m_Position -= glm::normalize(glm::cross(camFront, glm::vec3(0.0f, 1.0f, 0.0f))) * pThis->m_MoveSpeed * ServiceLocator::GetRenderer()->GetDeltaTime();

	pThis->UpdateViewMatrix();
}
 void Camera::moveRight(void* i_pCam)
{
	 

	 Camera* pThis = (Camera*)i_pCam;

	 //TODO: store camFront to avoid this
	 glm::vec3 camFront;
	 camFront.x = -cos(glm::radians(pThis->m_Rotation.x)) * sin(glm::radians(pThis->m_Rotation.y));
	 camFront.y = sin(glm::radians(pThis->m_Rotation.x));
	 camFront.z = cos(glm::radians(pThis->m_Rotation.x)) * cos(glm::radians(pThis->m_Rotation.y));
	 camFront = glm::normalize(camFront);
	 /////////////////

	 pThis->m_Position += glm::normalize(glm::cross(camFront, glm::vec3(0.0f, 1.0f, 0.0f))) * pThis->m_MoveSpeed * ServiceLocator::GetRenderer()->GetDeltaTime();

	 pThis->UpdateViewMatrix();
}
 void Camera::moveForward(void* i_pCam)
 {
	 Camera* pThis = (Camera*)i_pCam;

	 //TODO: store camFront to avoid this
	 glm::vec3 camFront;
	 camFront.x = -cos(glm::radians(pThis->m_Rotation.x)) * sin(glm::radians(pThis->m_Rotation.y));
	 camFront.y = sin(glm::radians(pThis->m_Rotation.x));
	 camFront.z = cos(glm::radians(pThis->m_Rotation.x)) * cos(glm::radians(pThis->m_Rotation.y));
	 camFront = glm::normalize(camFront);
	 /////////////////

	 pThis->m_Position += camFront * pThis->m_MoveSpeed * ServiceLocator::GetRenderer()->GetDeltaTime();

	 pThis->UpdateViewMatrix();
 }
 void Camera::moveBackwards(void* i_pCam)
 {
	 Camera* pThis = (Camera*)i_pCam;

	 //TODO: store camFront to avoid this
	 glm::vec3 camFront;
	 camFront.x = -cos(glm::radians(pThis->m_Rotation.x)) * sin(glm::radians(pThis->m_Rotation.y));
	 camFront.y = sin(glm::radians(pThis->m_Rotation.x));
	 camFront.z = cos(glm::radians(pThis->m_Rotation.x)) * cos(glm::radians(pThis->m_Rotation.y));
	 camFront = glm::normalize(camFront);
	 /////////////////

	 pThis->m_Position -= camFront * pThis->m_MoveSpeed * ServiceLocator::GetRenderer()->GetDeltaTime();

	 pThis->UpdateViewMatrix();
 }
 void Camera::startRotation(void* i_pCam)
 {
	 Camera* pThis = (Camera*)i_pCam;
	 pThis->m_bRotating = true;
 }
	
 void Camera::rotate(void* i_pCam)
 {
	 Camera* pThis = (Camera*)i_pCam;
	 if (pThis->m_bRotating)
	 {
	
		 pThis->m_Rotation += glm::vec3(ServiceLocator::GetInput()->GetMouseDelta().y, ServiceLocator::GetInput()->GetMouseDelta().x, 0.0f) *
			 pThis->m_RotatingSpeed * ServiceLocator::GetRenderer()->GetDeltaTime();

		pThis->UpdateViewMatrix();
		
	 }
	
 }
 void Camera::endRotation(void* i_pCam)
 {
	 Camera* pThis = (Camera*)i_pCam;
	 pThis->m_bRotating = false;
 }


