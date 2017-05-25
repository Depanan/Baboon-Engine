#include "CameraManager.h"
#include "Renderer\RendererAbstract.h"
#include "Core\ServiceLocator.h"

void CameraManager::OnWindowResize()
{
	for (int i=0;i<eCameraType_NCameras;i++)
	{
		m_Cameras[i].UpdateProjectionMatrix();
		
	}
}

void CameraManager::Init()
{
	for (int i = 0; i<eCameraType_NCameras; i++)
	{
		m_Cameras[i].Init();
	}
	RendererAbstract* renderer = ServiceLocator::GetRenderer();
	renderer->CreateStaticUniformBuffer(nullptr, sizeof(CameraUniforms));
}

void CameraManager::BindCameraUniforms(eCameraType i_camType)
{
	if (i_camType >= eCameraType_NCameras)
		return;
	m_CameraUniforms.view = m_Cameras[i_camType].GetViewMatrix();
	m_CameraUniforms.proj = m_Cameras[i_camType].GetProjMatrix();

}
