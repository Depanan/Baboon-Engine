#include "ServiceLocator.h"

RendererAbstract* ServiceLocator::s_TheRenderer = nullptr;
SceneManager* ServiceLocator::s_TheSceneManager = nullptr;
Input* ServiceLocator::s_TheInput = nullptr;
CameraManager* ServiceLocator::s_TheCamManager = nullptr;
Logger* ServiceLocator::s_TheLogger = nullptr;
ThreadPool* ServiceLocator::s_TheThreadPool = nullptr;