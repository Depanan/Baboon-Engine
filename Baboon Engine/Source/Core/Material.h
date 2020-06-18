#pragma once
#include <string>
#include <unordered_map>
#include "Renderer/Common/GLMInclude.h"

class Texture;

/**
 * @brief Adds support for C style preprocessor macros to glsl shaders
 *        enabling you to define or undefine certain symbols
 */
class ShaderVariant
{
public:
    ShaderVariant() { update_id(); }

    ShaderVariant(std::string&& preamble, std::vector<std::string>&& processes);

    size_t get_id() const;

    /**
     * @brief Add definitions to shader variant
     * @param definitions Vector of definitions to add to the variant
     */
    void add_definitions(const std::vector<std::string>& definitions);

    /**
     * @brief Adds a define macro to the shader
     * @param def String which should go to the right of a define directive
     */
    void add_define(const std::string& def);

    /**
     * @brief Adds an undef macro to the shader
     * @param undef String which should go to the right of an undef directive
     */
    void add_undefine(const std::string& undef);

    /**
     * @brief Specifies the size of a named runtime array for automatic reflection. If already specified, overrides the size.
     * @param runtime_array_name String under which the runtime array is named in the shader
     * @param size Integer specifying the wanted size of the runtime array (in number of elements, not size in bytes), used for automatic allocation of buffers.
     * See get_declared_struct_size_runtime_array() in spirv_cross.h
     */
    void add_runtime_array_size(const std::string& runtime_array_name, size_t size);

    void set_runtime_array_sizes(const std::unordered_map<std::string, size_t>& sizes);

    const std::string& get_preamble() const;

    const std::vector<std::string>& get_processes() const;

    const std::unordered_map<std::string, size_t>& get_runtime_array_sizes() const;

    void clear();

private:
    size_t id;

    std::string preamble;

    std::vector<std::string> processes;

    std::unordered_map<std::string, size_t> runtime_array_sizes;

    void update_id();
};



struct MaterialParameters {
  glm::vec4 m_Ambient;
  glm::vec4 m_Diffuse;
  glm::vec4 m_Specular;
};
#define MAX_MATERIALS 128
struct   alignas(16) UBOMaterial
{
  MaterialParameters m_Materials[MAX_MATERIALS];
};


class Material
{
public:
    void Init(std::string i_sMaterialName, std::vector<std::pair<std::string, Texture*>>*, bool isTransparent, MaterialParameters* parameters, uint8_t materialIndex);
	
	const std::string& GetMaterialName()
	{
		return m_sMaterialName;
	}
  Texture* GetTextureByName(std::string name );

  bool isTransparent() { return m_IsTransparent; }
  uint8_t getMaterialIndex() { return m_MaterialIndex; }

  std::unordered_map<std::string, Texture*>* getTextures() { return &m_Textures; }
private:
	std::string m_sMaterialName;
  std::unordered_map<std::string, Texture*> m_Textures;
  bool m_IsTransparent{ false };
  MaterialParameters* m_MaterialParameters;
  uint8_t m_MaterialIndex = 0;
};
