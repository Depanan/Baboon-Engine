#pragma once

#include "Camera.h"


struct CameraUniforms {
	glm::mat4 view;
	glm::mat4 proj;
};

class CameraManager
{
public:
	//Supports more than one camera, useful for reflection cameras, shadow cameras etc...
	enum eCameraType
	{
		eCameraType_Main,
		eCameraType_NCameras
	};
	void Init();
	
	void OnWindowResize();

	const Camera* GetCamera(eCameraType i_camType)
	{
		if (i_camType >= eCameraType_NCameras)
			return nullptr;

		return &m_Cameras[i_camType];
	}

	const CameraUniforms* GetCamUniforms() { return &m_CameraUniforms; }

	void BindCameraUniforms(eCameraType i_camType);
	


private:
	Camera m_Cameras[eCameraType_NCameras];
	CameraUniforms m_CameraUniforms;

};