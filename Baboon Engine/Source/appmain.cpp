#include "defines.h"

#include <string>
#include "Core\Scene.h"
#include "Core\Input.h"
#include "Cameras\CameraManager.h"
#include <filesystem>
#include <functional>
namespace fs = std::filesystem;


static std::string s_Window_Title = "Baboon Engine";


enum class FileStatus { created, modified, erased };
class FileWatcher 
{
public:
    

    // Keep a record of files from the base directory and their last modification time
    FileWatcher(std::string path_to_watch, std::chrono::duration<int, std::milli> interval, const std::function<void(std::string, FileStatus)> action) :
        m_PathToWatch{ path_to_watch }, 
        m_CheckInterval{ interval } ,
        m_Action{action}
    {
        for (auto& file : std::filesystem::recursive_directory_iterator(m_PathToWatch)) {
            m_Paths[file.path().string()] = std::filesystem::last_write_time(file);

        }
        m_Start = std::chrono::steady_clock::now();
    }


    // Monitor "path_to_watch" for changes and in case of a change execute the user supplied "action" function
    void check() 
    {
       
        auto timeNow =  std::chrono::steady_clock::now();
        auto int_ms = std::chrono::duration_cast<std::chrono::milliseconds>(timeNow - m_Start);
        if (int_ms >= m_CheckInterval)
        {
            m_Start = timeNow;

            auto it = m_Paths.begin();
            while (it != m_Paths.end())
            {
                if (!std::filesystem::exists(it->first))
                {
                    m_Action(it->first, FileStatus::erased);
                    it = m_Paths.erase(it);
                }
                else
                {
                    it++;
                }

            }
            for (auto& file : std::filesystem::recursive_directory_iterator(m_PathToWatch))
            {
                auto current_file_last_write_time = std::filesystem::last_write_time(file);

                // File creation
                if (!contains(file.path().string()))
                {
                    m_Paths[file.path().string()] = current_file_last_write_time;
                    m_Action(file.path().generic_string(), FileStatus::created);
                    // File modification

                }
                else
                {
                    if (m_Paths[file.path().string()] != current_file_last_write_time)
                    {
                        m_Paths[file.path().string()] = current_file_last_write_time;
                        m_Action(file.path().generic_string(), FileStatus::modified);
                    }
                }
            }
        }

        
        
    }


private:

    const std::function<void(std::string, FileStatus)> m_Action;

    std::unordered_map<std::string, std::filesystem::file_time_type> m_Paths;

    std::string m_PathToWatch;
    // Time interval at which we check the base folder for changes
    std::chrono::duration<int, std::milli> m_CheckInterval;

    std::chrono::time_point<std::chrono::steady_clock> m_Start;
    
    // Check if "m_Paths" contains a given key
    bool contains(const std::string& key) {
        auto el = m_Paths.find(key);
        return el != m_Paths.end();
        
    }



};


static auto checkShaderChangesFunction = [](std::string path_to_watch, FileStatus status) 
{


    if (!std::filesystem::is_regular_file(std::filesystem::path(path_to_watch)) && status != FileStatus::erased) {
        return;
    }
    if (status == FileStatus::modified)
    {
        //std::cout << "File modified: " << path_to_watch << '\n';
        ServiceLocator::GetRenderer()->ReloadShader(path_to_watch);
    }


};



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

    // Create a FileWatcher instance that will check the current folder for changes every 5 seconds
    FileWatcher fileWatcherShaders{ "./Shaders", std::chrono::milliseconds(5000),checkShaderChangesFunction };
      
    

		RendererAbstract* pRenderer = ServiceLocator::GetRenderer();
		Scene* pScene = ServiceLocator::GetSceneManager()->GetScene();
		Input* pInput = ServiceLocator::GetInput();
    CameraManager* pCameraMan = ServiceLocator::GetCameraManager();
		while (!glfwWindowShouldClose(m_window)) {
			auto tStart = std::chrono::high_resolution_clock::now();
			
			glfwPollEvents();
			
			pInput->processInput();//Order here is important, 1.Input modifies camera, 2.Cameraman update modifies uniform buffers, 3. Renderer uses them 
      pCameraMan->Update();
      pRenderer->Update();
			pRenderer->DrawFrame();
      pCameraMan->EndFrame();//clears camera dirty flag mainly
			pRenderer->UpdateTimesAndFPS(tStart);
      fileWatcherShaders.check();

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