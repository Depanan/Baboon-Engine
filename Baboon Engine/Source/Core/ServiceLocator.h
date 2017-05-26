#pragma once
#include "Renderer\RendererAbstract.h"
#include "Core\Scene.h"
#include "Core\Input.h"
#include "Cameras\CameraManager.h"
#include "Core\ThreadPool.h"

//Design patter to hold pointers likely to be a singleton but on a cleaner way
class ServiceLocator
{
public:
	//Implement one provide per each service
	static void Provide(RendererAbstract* i_Renderer) { s_TheRenderer = i_Renderer; }
	static void Provide(SceneManager* i_SceneMan) { s_TheSceneManager = i_SceneMan; }
	static void Provide(Input* i_Input) { s_TheInput = i_Input; }
	static void Provide(CameraManager* i_CamMan) { s_TheCamManager = i_CamMan; }
	static void Provide(ThreadPool* i_ThreadPool) { s_TheThreadPool = i_ThreadPool; }

	//One Getter for each service
	static RendererAbstract* GetRenderer() { return s_TheRenderer; }
	static SceneManager* GetSceneManager () { return s_TheSceneManager; }
	static Input* GetInput() { return s_TheInput; }
	static CameraManager* GetCameraManager() { return s_TheCamManager; }
	static ThreadPool* GetThreadPool() { return s_TheThreadPool; }

private:
	static RendererAbstract* s_TheRenderer;
	static SceneManager* s_TheSceneManager;
	static Input* s_TheInput;
	static CameraManager* s_TheCamManager;
	static ThreadPool*	s_TheThreadPool;
};
