#pragma once
#include "../Common.h"
#include <unordered_map>
#include <memory>
namespace spirv_cross
{
    class CompilerGLSL;
}


/// Types of shader resources
enum class ShaderResourceType
{
    Input,
    InputAttachment,
    Output,
    Image,
    ImageSampler,
    ImageStorage,
    Sampler,
    BufferUniform,
    BufferStorage,
    PushConstant,
    SpecializationConstant,
    All
};

/// This determines the type and method of how descriptor set should be created and bound
enum class ShaderResourceMode
{
    Static,
    Dynamic,
    UpdateAfterBind
};

/// Store shader resource data.
/// Used by the shader module.
struct ShaderResource
{
    VkShaderStageFlags stages;

    ShaderResourceType type;

    ShaderResourceMode mode;

    uint32_t set;

    uint32_t binding;

    uint32_t location;

    uint32_t input_attachment_index;

    uint32_t vec_size;

    uint32_t columns;

    uint32_t array_size;

    uint32_t offset;

    uint32_t size;

    uint32_t constant_id;

    std::string name;
};

class Device;
class ShaderSourcePool;
class ShaderSource
{
public:
    //ShaderSource() = default;
   
    ShaderSource(std::vector<uint8_t> && data);

    size_t get_id() const { return m_HashId; }

    inline const std::string& get_filename() const { return m_FileName; }

    inline const std::vector<uint8_t>& get_data() const { return m_Data; }
    
private:
    ShaderSource(const std::string& filename);
    size_t m_HashId;
    std::string m_FileName;
    std::vector<uint8_t> m_Data;

    friend class ShaderSourcePool;
};


class ShaderSourcePool
{
public:
    std::weak_ptr<ShaderSource> getShaderSource(std::string shaderPath);
    void reloadShader(std::string shaderPath);//TODO: we could do this individually?
private:
    std::unordered_map<std::string, std::shared_ptr<ShaderSource>> m_ShaderSources;
};

class ShaderModule
{
public:
    ShaderModule(const Device& device,
        VkShaderStageFlagBits stage,
        const std::shared_ptr<ShaderSource>& shaderSource
        /*,
        const ShaderVariant& shader_variant*/);

    ShaderModule(const ShaderModule&) = delete;

    ShaderModule(ShaderModule&& other);

    ShaderModule& operator=(const ShaderModule&) = delete;

    ShaderModule& operator=(ShaderModule&&) = delete;

    inline const size_t getId() const { return m_HashId; }
    inline const VkShaderStageFlagBits getStage() const { return m_Stage; }
    inline const std::string& getEntryPoint() const { return m_EntryPoint; }
    inline const std::vector<uint32_t>& getSourceBinary()const { return m_Spirv; }

    const std::vector<ShaderResource>& get_resources() const { return m_Resources; }

    bool isStillValid();
private:
    const Device& m_Device;
    VkShaderStageFlagBits m_Stage;
    std::string m_EntryPoint;
    std::weak_ptr<ShaderSource> m_Source;
    /// Compiled source
    std::vector<uint32_t> m_Spirv;

    VkShaderModule m_ShaderModule{ VK_NULL_HANDLE };
    size_t m_HashId;

    std::vector<ShaderResource> m_Resources;

    void readShaderResources();
    void readInputs(spirv_cross::CompilerGLSL* compiler);
    void readOutputs(spirv_cross::CompilerGLSL* compiler);
    void readSamplers(spirv_cross::CompilerGLSL* compiler);
    void readUniforms(spirv_cross::CompilerGLSL* compiler);
    void readPushConstants(spirv_cross::CompilerGLSL* compiler);

};


/*class ShaderVariant
{
};



TODO: Eventually add ShaderVariant. Basically what it does is, depending on the submesh (vertex attributes) and its material (textures), add defines to the shader beginning (preamble) to slightly modify the shader accordingly (Like in virtalis with kierans defines)
At the moment we have precompiled spirv stuff. I think that, to follow this approach I will have to compile in runtime using GLSLANG for example*/