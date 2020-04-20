#include "Input.h"
#include <imgui/imgui.h>


void Input::processInput()
{
	ImGuiIO& io = ImGui::GetIO();

	std::map<int, InputFunctionWithParam>::iterator mapIterator;
	//Process key input
	for (mapIterator = m_keyMaps.begin(); mapIterator != m_keyMaps.end(); mapIterator++)
	{
		int key = (*mapIterator).first;

		if (io.KeysDown[key])
		{
			InputFunctionWithParam functAndParam = (*mapIterator).second;
			functAndParam.function(functAndParam.pParam);
		}
	}

	//Process mouse moved
	if (io.MouseDelta.x != 0 || io.MouseDelta.y != 0)
	{
		if (m_MouseMovedFunction.function != nullptr)
		{
			m_MouseMovedFunction.function(m_MouseMovedFunction.pParam);
		}
	}

	//Process mouse clicked input
	for (int i = 0; i < m_MouseMapsClicked.size(); i++)
	{
		if (io.MouseClicked[i])
		{
			if (m_MouseMapsClicked[i].function != nullptr)
			{
				m_MouseMapsClicked[i].function(m_MouseMapsClicked[i].pParam);

			}
		}
	}
	
	//Process mouse released input
	for (int i = 0; i < m_MouseMapsReleased.size(); i++)
	{
		if (io.MouseReleased[i])
		{
			if (m_MouseMapsReleased[i].function != nullptr)
			{
				m_MouseMapsReleased[i].function(m_MouseMapsReleased[i].pParam);

			}
		}
	}
	

}

glm::vec3 Input::GetMouseDelta()
{
	ImGuiIO& io = ImGui::GetIO();
	return glm::vec3(io.MouseDelta.x, io.MouseDelta.y,0.0f);
}