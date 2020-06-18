#pragma once
#include "Camera.h"



class Buffer;
class CameraQuaternion :public Camera
{

public:
	
	
	void Update() override;
  void Init() override;
  
  
  static void moveLeft(void* i_pCam);
  static void moveRight(void* i_pCam);
  static void moveForward(void* i_pCam);
  static void moveBackwards(void* i_pCam);
  static void moveUp(void* i_pCam);
  static void moveDown(void* i_pCam);

  static void startRotation(void* i_pCam);
  static void rotate(void* i_pCam);
  static void endRotation(void* i_pCam);






private:

    float m_MoveSpeed;
    float m_RotatingSpeed;
    bool m_bRotating;
    glm::vec3 m_CamPositionDelta;
 


 


};
