#include "CameraQuaternion.h"
#include "Core\Input.h"
#include "Core\ServiceLocator.h"

#include "glm\ext.hpp"
#include "Renderer/Common/Buffer.h"

bool areParallel(glm::vec3 a, glm::vec3 b)
{
    glm::vec3 crossResult = glm::cross(a, b);
    bool parallel = (crossResult.x > -0.0001 && crossResult.x < 0.0001) &&
        (crossResult.y > -0.0001 && crossResult.y < 0.0001) &&
        (crossResult.z > -0.0001 && crossResult.z < 0.0001);
    return parallel;
}

void CameraQuaternion::Update()
{
    if(m_Dirty)
    {
        glm::vec3 axis = glm::cross(m_CamForward, m_CamUp);
        //compute quaternion for pitch based on the camera pitch angle
        glm::quat pitch_quat = glm::angleAxis(m_Rotation.x, axis);
        //determine heading quaternion from the camera up vector and the heading angle
        glm::quat heading_quat = glm::angleAxis(m_Rotation.y, m_CamUp);
        //add the two quaternions
        glm::quat temp = glm::cross(pitch_quat, heading_quat);
        temp = glm::normalize(temp);
        //update the direction from the quaternion
        glm::vec3 tempForward = glm::rotate(temp, m_CamForward);
        if (!areParallel(tempForward, m_CamUp))
            m_CamForward = tempForward;



        //add the camera delta
        m_CamPosition += m_CamPositionDelta;
        //set the look at to be infront of the camera
        m_CamLookAt = m_CamPosition + m_CamForward * 1.0f;

        //damping values
        m_Rotation.x *= .5;
        m_Rotation.y *= .5;
        m_CamPositionDelta = m_CamPositionDelta * .8f;

        Camera::Update();
       
    }
    
}

void CameraQuaternion::Init()
{
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

    pInput->MapKey(GLFW_KEY_A, CameraQuaternion::moveLeft, this);
    pInput->MapKey(GLFW_KEY_D, CameraQuaternion::moveRight, this);
    pInput->MapKey(GLFW_KEY_W, CameraQuaternion::moveForward, this);
    pInput->MapKey(GLFW_KEY_S, CameraQuaternion::moveBackwards, this);
    pInput->MapKey(GLFW_KEY_E, CameraQuaternion::moveUp, this);
    pInput->MapKey(GLFW_KEY_C, CameraQuaternion::moveDown, this);

    pInput->MapMouseButtonClicked(1, CameraQuaternion::startRotation, this);
    pInput->MapMouseMoved(CameraQuaternion::rotate, this);
    pInput->MapMouseButtonReleased(1, CameraQuaternion::endRotation, this);
}

void CameraQuaternion::moveLeft(void* i_pCam)
{
    CameraQuaternion* pThis = (CameraQuaternion*)i_pCam;


    pThis->m_CamPositionDelta -= glm::normalize(glm::cross(pThis->m_CamForward, pThis->m_CamUp)) * pThis->m_MoveSpeed * ServiceLocator::GetRenderer()->GetDeltaTime();
    pThis->m_Dirty = true;

}
void CameraQuaternion::moveRight(void* i_pCam)
{


    CameraQuaternion* pThis = (CameraQuaternion*)i_pCam;



    pThis->m_CamPositionDelta += glm::normalize(glm::cross(pThis->m_CamForward, pThis->m_CamUp)) * pThis->m_MoveSpeed * ServiceLocator::GetRenderer()->GetDeltaTime();
    pThis->m_Dirty = true;

}
void CameraQuaternion::moveForward(void* i_pCam)
{
    CameraQuaternion* pThis = (CameraQuaternion*)i_pCam;



    pThis->m_CamPositionDelta += pThis->m_CamForward * pThis->m_MoveSpeed * ServiceLocator::GetRenderer()->GetDeltaTime();
    pThis->m_Dirty = true;

}
void CameraQuaternion::moveBackwards(void* i_pCam)
{
    CameraQuaternion* pThis = (CameraQuaternion*)i_pCam;



    pThis->m_CamPositionDelta -= pThis->m_CamForward * pThis->m_MoveSpeed * ServiceLocator::GetRenderer()->GetDeltaTime();
    pThis->m_Dirty = true;

}
void CameraQuaternion::moveUp(void* i_pCam)
{
    CameraQuaternion* pThis = (CameraQuaternion*)i_pCam;



    pThis->m_CamPositionDelta += pThis->m_CamUp * pThis->m_MoveSpeed * ServiceLocator::GetRenderer()->GetDeltaTime();
    pThis->m_Dirty = true;

}
void CameraQuaternion::moveDown(void* i_pCam)
{
    CameraQuaternion* pThis = (CameraQuaternion*)i_pCam;


    pThis->m_CamPositionDelta -= pThis->m_CamUp * pThis->m_MoveSpeed * ServiceLocator::GetRenderer()->GetDeltaTime();
    pThis->m_Dirty = true;
}
void CameraQuaternion::startRotation(void* i_pCam)
{
    CameraQuaternion* pThis = (CameraQuaternion*)i_pCam;
    pThis->m_bRotating = true;



}

void CameraQuaternion::rotate(void* i_pCam)
{
    CameraQuaternion* pThis = (CameraQuaternion*)i_pCam;
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
        else if (pThis->m_Rotation.y <= -360.0f)
            pThis->m_Rotation.y += 360.0f;

        pThis->m_Dirty = true;
    }

}
void CameraQuaternion::endRotation(void* i_pCam)
{
    CameraQuaternion* pThis = (CameraQuaternion*)i_pCam;
    pThis->m_bRotating = false;

}
