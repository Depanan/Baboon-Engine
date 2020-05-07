#include "Material.h"
#include "../Renderer/Common/Texture.h"



ShaderVariant::ShaderVariant(std::string&& preamble, std::vector<std::string>&& processes) :
    preamble{ std::move(preamble) },
    processes{ std::move(processes) }
{
    update_id();
}

size_t ShaderVariant::get_id() const
{
    return id;
}

void ShaderVariant::add_definitions(const std::vector<std::string>& definitions)
{
    for (auto& definition : definitions)
    {
        add_define(definition);
    }
}

void ShaderVariant::add_define(const std::string& def)
{
    processes.push_back("D" + def);

    std::string tmp_def = def;

    // The "=" needs to turn into a space
    size_t pos_equal = tmp_def.find_first_of("=");
    if (pos_equal != std::string::npos)
    {
        tmp_def[pos_equal] = ' ';
    }

    preamble.append("#define " + tmp_def + "\n");

    update_id();
}

void ShaderVariant::add_undefine(const std::string& undef)
{
    processes.push_back("U" + undef);

    preamble.append("#undef " + undef + "\n");

    update_id();
}

void ShaderVariant::add_runtime_array_size(const std::string& runtime_array_name, size_t size)
{
    if (runtime_array_sizes.find(runtime_array_name) == runtime_array_sizes.end())
    {
        runtime_array_sizes.insert({ runtime_array_name, size });
    }
    else
    {
        runtime_array_sizes[runtime_array_name] = size;
    }
}

void ShaderVariant::set_runtime_array_sizes(const std::unordered_map<std::string, size_t>& sizes)
{
    this->runtime_array_sizes = sizes;
}

const std::string& ShaderVariant::get_preamble() const
{
    return preamble;
}

const std::vector<std::string>& ShaderVariant::get_processes() const
{
    return processes;
}

const std::unordered_map<std::string, size_t>& ShaderVariant::get_runtime_array_sizes() const
{
    return runtime_array_sizes;
}

void ShaderVariant::clear()
{
    preamble.clear();
    processes.clear();
    runtime_array_sizes.clear();
    update_id();
}

void ShaderVariant::update_id()
{
    std::hash<std::string> hasher{};
    id = hasher(preamble);
}



void Material::Init(std::string i_sMaterialName, std::vector<std::pair<std::string, Texture*>>* i_Textures, bool isTransparent)
{
    m_IsTransparent = isTransparent;
    m_sMaterialName = i_sMaterialName;
    if (i_Textures)
    {
        for (auto nameAndTexture : *i_Textures)
        {
            m_Textures[nameAndTexture.first] = nameAndTexture.second;
        }
    }
    
}

Texture* Material::GetTextureByName(std::string name)
{
    Texture* tex;
    auto texIterator = m_Textures.find(name);
    texIterator != m_Textures.end() ? tex = texIterator->second : tex = nullptr;
    return tex;
}
