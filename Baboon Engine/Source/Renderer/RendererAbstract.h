#pragma once
#include <GLFW/glfw3.h>
#include <chrono>
#include <string>
#include <vector>
class RendererAbstract
{
public:
	
	virtual ~RendererAbstract(){}
	virtual int Init(std::vector<const char*>& required_extensions,  GLFWwindow* i_window) = 0;
	virtual void Destroy() = 0;
	virtual void DrawFrame() = 0;
	virtual void OnWindowResize(int i_NewW, int i_NewH) = 0;
	virtual void WaitToDestroy() {}//Function to wait till we can delete renderer stuff, like in vulkan we have to wait for vkDeviceWaitIdle(device);
	virtual void SetupRenderCalls(){}
	virtual float GetMainRTAspectRatio() = 0;
	virtual float GetMainRTWidth() = 0;
	virtual float GetMainRTHeight() = 0;
	virtual void UpdateTimesAndFPS(std::chrono::time_point<std::chrono::high_resolution_clock>  i_tStartTime) = 0;
	virtual int CreateTexture(void*  i_data, int i_Widht, int i_Height) = 0;
	virtual void CreateMaterial(std::string i_MatName, int* iTexIndices, int iNumTextures) = 0;
	virtual void DeleteMaterials() = 0;
	virtual void CreateVertexBuffer(const void*  i_data, size_t iBufferSize) = 0;
	virtual void CreateIndexBuffer(const void*  i_data, size_t iBufferSize) = 0;
	virtual void DeleteVertexBuffer() = 0;
	virtual void DeleteIndexBuffer() = 0;
	virtual void CreateStaticUniformBuffer(const void*  i_data, size_t iBufferSize){}
	virtual void CreateInstancedUniformBuffer(const void*  i_data, size_t iBufferSize){}
	virtual void DeleteStaticUniformBuffer() {}
	virtual void DeleteInstancedUniformBuffer() {}
	float GetDeltaTime() { return m_LastFrameTime; }

protected:

	float m_FpsTimer = 1000.0f;//Timer to update FPS 
	float m_LastFrameTime = 1.0f;//Last frame Time (delta time) (In seconds)
	uint32_t m_FrameCounter = 0;
	uint32_t m_LastFPS = 0;
	
};
