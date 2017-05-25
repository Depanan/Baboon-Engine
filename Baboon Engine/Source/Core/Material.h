#pragma once
#include <string>

class Material
{
public:
	void Init(std::string i_sMaterialName)
	{
		m_sMaterialName = i_sMaterialName;
	}
	const std::string& GetMaterialName()
	{
		return m_sMaterialName;
	}

private:
	std::string m_sMaterialName;

};
