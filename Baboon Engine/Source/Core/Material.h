#pragma once
#include <string>
#include <unordered_map>

class Texture;
class Material
{
public:
    void Init(std::string i_sMaterialName, std::vector<std::pair<std::string, Texture*>>, bool isTransparent);
	
	const std::string& GetMaterialName()
	{
		return m_sMaterialName;
	}
  Texture* GetTextureByName(std::string name );

  bool isTransparent() { return m_IsTransparent; }


  std::unordered_map<std::string, Texture*>* getTextures() { return &m_Textures; }
private:
	std::string m_sMaterialName;
  std::unordered_map<std::string, Texture*> m_Textures;
  bool m_IsTransparent{ false };
};
