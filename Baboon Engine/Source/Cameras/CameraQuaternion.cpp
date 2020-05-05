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