#include "CameraManager.h"
#include "Renderer\RendererAbstract.h"
#include "Core\ServiceLocator.h"
#include "Renderer\Common\Buffer.h"


void CameraManager::AddCamera(std::string camId)
{
    auto& res = m_Cameras.emplace(camId, new CameraQuaternion());
    if (res.second)
    {
        CameraQuaternion* cam = (CameraQuaternion*)(res.first->second);
        cam->Init();
    }
        
}
void CameraManager::AddShadowCamera(std::string camId)
{
    auto& res = m_Cameras.emplace(camId, new ShadowCamera());
    if (res.second)
    {
        ShadowCamera* cam = (ShadowCamera*)(res.first->second);
        cam->Init();
    }
}

Camera* CameraManager::GetCamera(std::string camId)
{
    auto res = m_Cameras.find(camId);
    if (res == m_Cameras.end())
        return nullptr;
    return res->second;

}

void CameraManager::FetchShadowsUBO(Buffer& buffer) 
{
    int i = 0;
    for (auto& camera : m_Cameras)
    {
        ShadowCamera* shadowCam = dynamic_cast<ShadowCamera*>(camera.second);
        if (shadowCam)
        {
            m_UboShadows.viewprojections[i] = shadowCam->GetViewProjMatrix();
            i++;

        }
    }
    buffer.update((void*)&m_UboShadows, sizeof(UBOShadows));
}



void CameraManager::OnWindowResize(int width, int height)
{
  float aspectRatio = (float)width / height;
    
  for (auto& camera : m_Cameras)
  {
      camera.second->UpdateProjectionMatrix(aspectRatio);
  }
}



void CameraManager::Update()
{
    
    for (auto& camera : m_Cameras)
    {
        camera.second->Update();
    }
}



void CameraManager::Init()
{

}


