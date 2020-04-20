#pragma once
#include "../Common.h"


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
class ShaderSource
{
public:
    //ShaderSource() = default;
    ShaderSource(const std::string & filename);
    ShaderSource(std::vector<uint32_t> && data);

    size_t get_id() const { return m_HashId; }

    inline const std::string& get_filename() const { return m_FileName; }

    inline const std::vector<uint32_t>& get_data() const { return m_Data; }

private:
    size_t m_HashId;
    std::string m_FileName;
    std::vector<uint32_t> m_Data;
};

class ShaderModule
{
public:
    ShaderModule(const Device& device,
        VkShaderStageFlagBits stage,
        const ShaderSource& shaderSource
        /*,
        const ShaderVariant& shader_variant*/);

    ShaderModule(const ShaderModule&) = delete;

    ShaderModule(ShaderModule&& other);

    ShaderModule& operator=(const ShaderModule&) = delete;

    ShaderModule& operator=(ShaderModule&&) = delete;

    inline const size_t getId() const { return m_HashId; }
    inline const VkShaderStageFlagBits getStage() const { return m_Stage; }
    inline const std::string& getEntryPoint() const { return m_EntryPoint; }
    inline const std::vector<uint32_t>& getSourceBinary()const { return m_Source.get_data(); }

    const std::vector<ShaderResource>& get_resources() const { return m_Resources; }
private:
    const Device& m_Device;
    VkShaderStageFlagBits m_Stage;
    std::string m_EntryPoint;
    const ShaderSource& m_Source;
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