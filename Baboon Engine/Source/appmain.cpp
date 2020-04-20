#include "defines.h"

#include <string>
#include "Core\Scene.h"
#include "Core\Input.h"
#include "Cameras\CameraManager.h"

static std::string s_Window_Title = "Baboon Engine";


class GraphicGLFWApp
{
public:
	void init()
	{
		initWindow();
	    initRenderer();
		ServiceLocator::GetCameraManager()->Init();
		//ServiceLocator::GetSceneManager()->GetScene()->Init();
		
	}

	void mainLoop()
	{
		RendererAbstract* pRenderer = ServiceLocator::GetRenderer();
		Scene* pScene = ServiceLocator::GetSceneManager()->GetScene();
		Input* pInput = ServiceLocator::GetInput();

		while (!glfwWindowShouldClose(m_window)) {
			auto tStart = std::chrono::high_resolution_clock::now();
			
			glfwPollEvents();
			
			pInput->processInput();
      pRenderer->Update();
			pRenderer->DrawFrame();
			
			pRenderer->UpdateTimesAndFPS(tStart);
			
      ServiceLocator::GetLogger()->process();

		}
		pRenderer->WaitToDestroy();

		glfwDestroyWindow(m_window);
		glfwTerminate();
		pRenderer->Destroy();
	}

private:
	GLFWwindow* m_window;

	

	
	
	static void onWindowResized(GLFWwindow* window, int width, int height) {
		if (width == 0 || height == 0) return;//Prevents problems when minimizing the window

		GraphicGLFWApp* thisPointer = reinterpret_cast<GraphicGLFWApp*>(glfwGetWindowUserPointer(window));
		ServiceLocator::GetRenderer()->OnWindowResize(width,height);
		ServiceLocator::GetCameraManager()->OnWindowResize(width,height);
		ServiceLocator::GetSceneManager()->GetScene()->OnWindowResize();
	}


	void initWindow()
	{
		glfwInit();

		glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
		m_window = glfwCreateWindow(WINDOW_W, WINDOW_H, s_Window_Title.c_str(), nullptr, nullptr);

		glfwSetWindowUserPointer(m_window, this);
		glfwSetWindowSizeCallback(m_window, onWindowResized);

	}
	int initRenderer()
	{
		

    std::vector<const char*> extensions;

    unsigned int glfwExtensionCount = 0;
    const char** glfwExtensions;

    glfwExtensions = glfwGetRequiredInstanceExtensions(&glfwExtensionCount);
    extensions.insert(extensions.begin(), glfwExtensions, glfwExtensions + glfwExtensionCount);

    

		return ServiceLocator::GetRenderer()->Init(extensions,m_window,ServiceLocator::GetCameraManager()->GetCamera(CameraManager::eCameraType_Main));

	}
	

};




int main() {
	
	GraphicGLFWApp mainApp;
	RendererVulkan vulkanRenderer;//HERE WE COULD CREATE AN OPENGL RENDERER
	SceneManager theSceneManager;
	Input theInput;
	CameraManager camManager;
  Logger logger;

	ServiceLocator::Provide(&theSceneManager);
	ServiceLocator::Provide(&vulkanRenderer);
	ServiceLocator::Provide(&theInput);
	ServiceLocator::Provide(&camManager);
  ServiceLocator::Provide(&logger);
	
	try {
		mainApp.init();
	}
	catch (const std::runtime_error& e) {
		std::cerr << e.what() << std::endl;
		return EXIT_FAILURE;
	}
	
	
	try {
		mainApp.mainLoop();
	}
	catch (const std::runtime_error& e) {
		std::cerr << e.what() << std::endl;
		return EXIT_FAILURE;
	}
	
}