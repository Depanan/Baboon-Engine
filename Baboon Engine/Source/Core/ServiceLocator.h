#pragma once
#include "Renderer\RendererAbstract.h"
#include "Core\Scene.h"
#include "Core\Input.h"

//Design patter to hold pointers likely to be a singleton but on a cleaner way
class ServiceLocator
{
public:
	//Implement one provide per each service
	static void Provide(RendererAbstract* i_Renderer) { s_TheRenderer = i_Renderer; }
	static void Provide(SceneManager* i_SceneMan) { s_TheSceneManager = i_SceneMan; }
	static void Provide(Input* i_Input) { s_TheInput = i_Input; }

	//One Getter for each service
	static RendererAbstract* GetRenderer() { return s_TheRenderer; }
	static SceneManager* GetSceneManager () { return s_TheSceneManager; }
	static Input* GetInput() { return s_TheInput; }

private:
	static RendererAbstract* s_TheRenderer;
	static SceneManager* s_TheSceneManager;
	static Input* s_TheInput;
};
