#include "ServiceLocator.h"

RendererAbstract* ServiceLocator::s_TheRenderer = nullptr;
SceneManager* ServiceLocator::s_TheSceneManager = nullptr;
Input* ServiceLocator::s_TheInput = nullptr;
CameraManager* ServiceLocator::s_TheCamManager = nullptr;