#include "Camera.h"
#include "Core\Input.h"
#include "Core\ServiceLocator.h"

#include "glm\ext.hpp"
#include "Renderer/Common/Buffer.h"





void Camera::Init()
{
  size_t size = sizeof(UBOCamera);
  m_Near = 0.1f;
  m_Far = 10000.0f;
  m_Fov = glm::radians(60.0f);
  m_AspectRatio = ServiceLocator::GetRenderer()->GetMainRTWidth() / ServiceLocator::GetRenderer()->GetMainRTHeight();
  UpdateProjectionMatrix(m_AspectRatio);

  m_UBOCamera.view = glm::mat4();
  m_Rotation = glm::vec3(0.0f);
  m_RotationConstraints = glm::vec3(90.0f, 360.0f, 0.0f);
	
  m_CamPosition = glm::vec3(0.0f);
  m_CamPositionDelta = glm::vec3(0.0f);
  m_CamForward = glm::vec3(0, 0, 1.0f);
  m_CamLookAt = m_CamPosition + m_CamForward * 1.0f;
  m_CamUp = glm::vec3(0.0, 1.0, 0.0f);


	m_MoveSpeed = 50.0f;
	m_RotatingSpeed = 0.75f;
	m_bRotating = false;

	
	
	//Map functions to input:

	Input* pInput = ServiceLocator::GetInput();

	pInput->MapKey(GLFW_KEY_A, Camera::moveLeft,this);
	pInput->MapKey(GLFW_KEY_D, Camera::moveRight, this);
	pInput->MapKey(GLFW_KEY_W, Camera::moveForward, this);
	pInput->MapKey(GLFW_KEY_S, Camera::moveBackwards, this);
  pInput->MapKey(GLFW_KEY_E, Camera::moveUp, this);
  pInput->MapKey(GLFW_KEY_C, Camera::moveDown, this);

	pInput->MapMouseButtonClicked(1, Camera::startRotation, this);
	pInput->MapMouseMoved(Camera::rotate, this);
	pInput->MapMouseButtonReleased(1, Camera::endRotation, this);

}
void Camera::Update()
{
    m_UBOCamera.camPos = m_CamPosition;
    m_UBOCamera.view = glm::lookAt(m_CamPosition, m_CamLookAt, m_CamUp);
    ServiceLocator::GetCameraManager()->GetSubject().Notify(Subject::CAMERADIRTY, this);
    m_Dirty = false;
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

void Camera::CenterAt(glm::vec3 lookAt)
{
    m_CamPosition = lookAt - glm::vec3(10.0, 0, 0.0);
    m_CamLookAt = lookAt;
    m_CamForward = glm::vec3(0, 0, 1);//glm::normalize(lookAt - newPosition);
    m_Rotation = glm::vec3(0.0);

    m_Dirty = true;
}
void Camera::UpdateUniformBuffer(Buffer& buffer) const
{

    buffer.update((void *)&m_UBOCamera, sizeof(UBOCamera));    
}


void Camera::moveLeft(void* i_pCam)
{
	Camera* pThis = (Camera*)i_pCam;

	
	pThis->m_CamPositionDelta -= glm::normalize(glm::cross(pThis->m_CamForward, pThis->m_CamUp )) * pThis->m_MoveSpeed * ServiceLocator::GetRenderer()->GetDeltaTime();
  pThis->m_Dirty = true;
	
}
 void Camera::moveRight(void* i_pCam)
{
	 

	 Camera* pThis = (Camera*)i_pCam;

	

	 pThis->m_CamPositionDelta += glm::normalize(glm::cross(pThis->m_CamForward, pThis->m_CamUp)) * pThis->m_MoveSpeed * ServiceLocator::GetRenderer()->GetDeltaTime();
   pThis->m_Dirty = true;
	
}
 void Camera::moveForward(void* i_pCam)
 {
	 Camera* pThis = (Camera*)i_pCam;

	

	 pThis->m_CamPositionDelta += pThis->m_CamForward * pThis->m_MoveSpeed * ServiceLocator::GetRenderer()->GetDeltaTime();
   pThis->m_Dirty = true;
	
 }
 void Camera::moveBackwards(void* i_pCam)
 {
	 Camera* pThis = (Camera*)i_pCam;

	 

	 pThis->m_CamPositionDelta -= pThis->m_CamForward * pThis->m_MoveSpeed * ServiceLocator::GetRenderer()->GetDeltaTime();
   pThis->m_Dirty = true;
	
 }
 void Camera::moveUp(void* i_pCam)
 {
     Camera* pThis = (Camera*)i_pCam;

     

     pThis->m_CamPositionDelta += pThis->m_CamUp * pThis->m_MoveSpeed * ServiceLocator::GetRenderer()->GetDeltaTime();
     pThis->m_Dirty = true;
     
 }
 void Camera::moveDown(void* i_pCam)
 {
     Camera* pThis = (Camera*)i_pCam;


     pThis->m_CamPositionDelta -= pThis->m_CamUp * pThis->m_MoveSpeed * ServiceLocator::GetRenderer()->GetDeltaTime();
     pThis->m_Dirty = true;
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
       float degreesX = -ServiceLocator::GetInput()->GetMouseDelta().y * pThis->m_RotatingSpeed * ServiceLocator::GetRenderer()->GetDeltaTime();
       float degreesY = -ServiceLocator::GetInput()->GetMouseDelta().x * pThis->m_RotatingSpeed * ServiceLocator::GetRenderer()->GetDeltaTime();

       if (pThis->m_Rotation.x > 90 && pThis->m_Rotation.x < 270 || (pThis->m_Rotation.x < -90 && pThis->m_Rotation.x > -270)) {
           pThis->m_Rotation.x -= degreesX;
       }
       else {
           pThis->m_Rotation.x += degreesX;
       }

       pThis->m_Rotation.y += degreesY;
       if (pThis->m_Rotation.y >= 360.0f)
           pThis->m_Rotation.y -= 360.0f;
       else if(pThis->m_Rotation.y <= -360.0f)
           pThis->m_Rotation.y += 360.0f;

       pThis->m_Dirty = true;
	 }
	
 }
 void Camera::endRotation(void* i_pCam)
 {
	 Camera* pThis = (Camera*)i_pCam;
	 pThis->m_bRotating = false;
 
 }


