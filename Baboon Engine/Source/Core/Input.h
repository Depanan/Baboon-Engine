#pragma once
#include <map>
#include <array>
#include <glm\glm.hpp>
typedef void(*PtrFunc)(void*);

struct InputFunctionWithParam
{
	PtrFunc function = nullptr;
	void* pParam = nullptr;
};
class Input
{
public:
	void MapKey(int key, PtrFunc func, void* i_Param)
	{
		InputFunctionWithParam funcWithParam;
		funcWithParam.function = func;
		funcWithParam.pParam = i_Param;
		m_keyMaps[key] = funcWithParam;
	}

	void MapMouseButtonClicked(int button, PtrFunc func, void* i_Param)
	{
		if (button >= m_MouseMapsClicked.size())
		{
			return;
		}
		InputFunctionWithParam funcWithParam;
		funcWithParam.function = func;
		funcWithParam.pParam = i_Param;
		m_MouseMapsClicked[button] = funcWithParam;
	}
	void MapMouseButtonReleased(int button, PtrFunc func, void* i_Param)
	{
		if (button >= m_MouseMapsReleased.size())
		{
			return;
		}
		InputFunctionWithParam funcWithParam;
		funcWithParam.function = func;
		funcWithParam.pParam = i_Param;
		m_MouseMapsReleased[button] = funcWithParam;
	}

	void MapMouseMoved(PtrFunc func, void* i_Param)
	{
		
		InputFunctionWithParam funcWithParam;
		funcWithParam.function = func;
		funcWithParam.pParam = i_Param;
		m_MouseMovedFunction = funcWithParam;
	}

	void processInput();
	glm::vec3 GetMouseDelta();
	
private:
	std::map<int, InputFunctionWithParam> m_keyMaps;
	
	std::array<InputFunctionWithParam, 2>m_MouseMapsClicked;
	std::array<InputFunctionWithParam, 2>m_MouseMapsReleased;

	InputFunctionWithParam m_MouseMovedFunction;
};
