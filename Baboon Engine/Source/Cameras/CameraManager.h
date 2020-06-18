#pragma once

#include "CameraQuaternion.h"
#include "Core/Observer.h"
#include <unordered_map>
#include <string>
#include "Renderer/Common/GLMInclude.h"

struct alignas(16) UBOShadows  {
    glm::mat4 viewprojections[32];
   
};

class Buffer;
class CameraManager
{
public:
	//Supports more than one camera, useful for reflection cameras, shadow cameras etc...
	enum eCameraType
	{
		eCameraType_Main,
		eCameraType_NCameras
	};
  void Update();
	void Init();
	
	void OnWindowResize(int width, int height);

 
  void AddCamera(std::string camId);
  void AddShadowCamera(std::string camId);
  

  Camera* GetCamera(std::string camId);
	

  void FetchShadowsUBO(Buffer& buffer) ;
  UBOShadows& GetShadowsUBO() { return m_UboShadows; }

	

  Subject& GetSubject() { return m_SceneSubject; }
private:
  Subject m_SceneSubject;
  std::unordered_map<std::string, Camera*> m_Cameras;

  UBOShadows m_UboShadows;

};