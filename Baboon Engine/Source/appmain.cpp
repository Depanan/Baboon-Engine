#include "defines.h"

#include <string>
#include "Core\Scene.h"
#include "Core\Input.h"
#include "Cameras\CameraManager.h"
#include "Core\ThreadPool.h"

static std::string s_Window_Title = "Baboon Engine";


class GraphicGLFWApp
{
public:
	void init()
	{
		initWindow();
	    initRenderer();
		ServiceLocator::GetCameraManager()->Init();
		ServiceLocator::GetSceneManager()->GetScene()->Init("F:\\Personal Work\\Vulkan Experiments\\Baboon Engine\\Scenes\\box.obj");
		
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
			pScene->UpdateUniforms();
			pRenderer->DrawFrame();
			
			pRenderer->UpdateTimesAndFPS(tStart);
			
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
		ServiceLocator::GetCameraManager()->OnWindowResize();
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
		

		unsigned int glfwExtensionCount = 0;
		const char** glfwExtensions;

		glfwExtensions = glfwGetRequiredInstanceExtensions(&glfwExtensionCount);
		

		return ServiceLocator::GetRenderer()->Init(glfwExtensions, glfwExtensionCount,m_window);

	}
	

};



int main() {
	

	ThreadPool threadPool;
	threadPool.setThreadCount(2);

	GraphicGLFWApp mainApp;
	RendererVulkan vulkanRenderer;//HERE WE COULD CREATE AN OPENGL RENDERER
	SceneManager theSceneManager;
	Input theInput;
	CameraManager camManager;

	ServiceLocator::Provide(&threadPool);
	ServiceLocator::Provide(&theSceneManager);
	ServiceLocator::Provide(&vulkanRenderer);
	ServiceLocator::Provide(&theInput);
	ServiceLocator::Provide(&camManager);
	
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