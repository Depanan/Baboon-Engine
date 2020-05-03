#include "Camera.h"
#include "Core\Input.h"
#include "Core\ServiceLocator.h"

#include "Renderer/Common/Buffer.h"


void Camera::Init()
{
  size_t size = sizeof(UBOCamera);
 
  m_UBOCamera.view = glm::mat4();
	UpdateProjectionMatrix(ServiceLocator::GetRenderer()->GetMainRTWidth() / ServiceLocator::GetRenderer()->GetMainRTHeight());
  m_CamPosition = glm::vec3(0.0f);
  m_CamForward = glm::vec3(0, 0, -1.0f);
  m_CamRight = glm::vec3(1.0, 0.0, 0.0f);
	m_MoveSpeed = 100.0f;
	m_RotatingSpeed = 10.0f;
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
void Camera::Teleport(glm::vec3 newPosition, glm::vec3 lookAt)
{
    /*m_UBOCamera.view = glm::lookAt(newPosition, lookAt, glm::vec3(0, 1, 0));
   m_CamPosition = glm::vec3(m_UBOCamera.view[3]);

    m_Dirty = true;*/
}
void Camera::Update(Buffer& buffer) const
{
   

    buffer.update((void *)&m_UBOCamera, sizeof(UBOCamera));
    
    
}
void Camera::UpdateViewMatrix()
{
	glm::mat4 rotM = glm::mat4();
	glm::mat4 transM;

	rotM = glm::rotate(rotM, glm::radians(m_Rotation.x), glm::vec3(1.0f, 0.0f, 0.0f));
	rotM = glm::rotate(rotM, glm::radians(m_Rotation.y), glm::vec3(0.0f, 1.0f, 0.0f));
	rotM = glm::rotate(rotM, glm::radians(m_Rotation.z), glm::vec3(0.0f, 0.0f, 1.0f));

	transM = glm::translate(glm::mat4(), glm::vec3(-m_CamPosition.x,-m_CamPosition.y,-m_CamPosition.z));

	//TODO: All this camera stuff is being a bit confusing. Will probably have to come back here when depth stuff won't work
  m_UBOCamera.view = rotM * transM;
  m_UBOCamera.camPos = m_CamPosition;


  m_Dirty = true;
}

void Camera::moveLeft(void* i_pCam)
{
	Camera* pThis = (Camera*)i_pCam;

	
	pThis->m_CamPosition -= pThis->m_CamRight * pThis->m_MoveSpeed * ServiceLocator::GetRenderer()->GetDeltaTime();

	pThis->UpdateViewMatrix();
}
 void Camera::moveRight(void* i_pCam)
{
	 

	 Camera* pThis = (Camera*)i_pCam;

	

	 pThis->m_CamPosition += pThis->m_CamRight * pThis->m_MoveSpeed * ServiceLocator::GetRenderer()->GetDeltaTime();

	 pThis->UpdateViewMatrix();
}
 void Camera::moveForward(void* i_pCam)
 {
	 Camera* pThis = (Camera*)i_pCam;

	

	 pThis->m_CamPosition += pThis->m_CamForward * pThis->m_MoveSpeed * ServiceLocator::GetRenderer()->GetDeltaTime();

	 pThis->UpdateViewMatrix();
 }
 void Camera::moveBackwards(void* i_pCam)
 {
	 Camera* pThis = (Camera*)i_pCam;

	 

	 pThis->m_CamPosition -= pThis->m_CamForward * pThis->m_MoveSpeed * ServiceLocator::GetRenderer()->GetDeltaTime();

	 pThis->UpdateViewMatrix();
 }
 void Camera::moveUp(void* i_pCam)
 {
     Camera* pThis = (Camera*)i_pCam;

     

     pThis->m_CamPosition += glm::vec3(0,1,0) * pThis->m_MoveSpeed * ServiceLocator::GetRenderer()->GetDeltaTime();

     pThis->UpdateViewMatrix();
 }
 void Camera::moveDown(void* i_pCam)
 {
     Camera* pThis = (Camera*)i_pCam;


     pThis->m_CamPosition -= glm::vec3(0, 1, 0) * pThis->m_MoveSpeed * ServiceLocator::GetRenderer()->GetDeltaTime();

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


     pThis->m_CamForward.x = -cos(glm::radians(pThis->m_Rotation.x)) * sin(glm::radians(pThis->m_Rotation.y));
     pThis->m_CamForward.y = sin(glm::radians(pThis->m_Rotation.x));
     pThis->m_CamForward.z = cos(glm::radians(pThis->m_Rotation.x)) * cos(glm::radians(pThis->m_Rotation.y));
     pThis->m_CamForward = glm::normalize(-pThis->m_CamForward);
     pThis->m_CamRight = glm::normalize(glm::cross(pThis->m_CamForward, glm::vec3(0.0f, 1.0f, 0.0f)));
		pThis->UpdateViewMatrix();
		
	 }
	
 }
 void Camera::endRotation(void* i_pCam)
 {
	 Camera* pThis = (Camera*)i_pCam;
	 pThis->m_bRotating = false;

   
 }


