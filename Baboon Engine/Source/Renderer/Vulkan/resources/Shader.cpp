#include "Shader.h"
#include "../Device.h"
#include "../glsl_compiler.h"
#include "Core/ServiceLocator.h"
__pragma(warning(push, 0))
#include <spirv-cross/spirv_glsl.hpp>
__pragma(warning(pop))




ShaderSource::ShaderSource(const std::string& filename)
    :m_FileName(filename),
    m_Data(readFileUint8(m_FileName))
{
    std::hash<std::string> hasher{};
    m_HashId = hasher(std::string{ m_Data.cbegin(), m_Data.cend() });
    //m_HashId = std::time(NULL);
}

ShaderSource::ShaderSource(std::vector<uint8_t>&& data):
    m_Data{ std::move(data) }
{
    m_FileName = "\0";
    std::hash<std::string> hasher{};
    m_HashId = hasher(std::string{ m_Data.cbegin(), m_Data.cend() });
    //m_HashId = std::time(NULL);
}


ShaderModule::ShaderModule(const Device& device,
    VkShaderStageFlagBits stage,
    const std::shared_ptr<ShaderSource>& shaderSource
    /*,
    const ShaderVariant& shader_variant*/):
    m_Device(device),
    m_Stage(stage),
    m_Source(shaderSource),
    m_SourceName(shaderSource->get_filename())

{
    auto srcPtr = m_Source.lock();
    if(srcPtr)
    {
        m_EntryPoint = "main";//TODO: Maybe don't hardcode this?
        assert(srcPtr->get_data().size(), "Source code for shadermodule can't be empty!");

        std::string infoLog;
        GLSLCompiler compiler;
        compiler.compile_to_spirv(m_Stage, srcPtr->get_data(), m_EntryPoint, m_Spirv, infoLog);
    
        readShaderResources();
    }
}

ShaderModule::ShaderModule(ShaderModule&& other) :
m_Device(other.m_Device),
m_Stage(other.m_Stage),
m_Source(other.m_Source),
m_Spirv(other.m_Spirv),
m_ShaderModule(other.m_ShaderModule),
m_EntryPoint(other.m_EntryPoint),
m_Resources(other.m_Resources),
m_SourceName(other.m_SourceName)

{
    other.m_ShaderModule = VK_NULL_HANDLE;
}

bool ShaderModule::isStillValid()
{
    bool valid = m_Source.lock() != nullptr;
    return valid;
}




void ShaderModule::readShaderResources()
{
    spirv_cross::CompilerGLSL compiler{ m_Spirv };
    readInputs(&compiler);
    readOutputs(&compiler);
    readSamplers(&compiler);
    readUniforms(&compiler);
    readPushConstants(&compiler);

}

void ShaderModule::readInputs(spirv_cross::CompilerGLSL* compiler)
{
    auto shaderInputs = compiler->get_shader_resources().stage_inputs;

    for (auto& resource : shaderInputs)
    {
        ShaderResource shader_resource{};
        shader_resource.type = ShaderResourceType::Input;
        shader_resource.stages = m_Stage;
        shader_resource.name = resource.name;

        const auto& spirv_type = compiler->get_type_from_variable(resource.id);

        shader_resource.vec_size = spirv_type.vecsize;
        shader_resource.columns = spirv_type.columns;

        shader_resource.location = compiler->get_decoration(resource.id, spv::DecorationLocation);
        shader_resource.array_size = spirv_type.array.size() ? spirv_type.array[0] : 1;
        m_Resources.push_back(shader_resource);
    }
}
void ShaderModule::readOutputs(spirv_cross::CompilerGLSL* compiler)
{
    auto shaderOutputs = compiler->get_shader_resources().stage_outputs;
    for (auto& resource : shaderOutputs)
    {
        ShaderResource shader_resource{};
        shader_resource.type = ShaderResourceType::Output;
        shader_resource.stages = m_Stage;
        shader_resource.name = resource.name;
        shader_resource.location = compiler->get_decoration(resource.id, spv::DecorationLocation);
        m_Resources.push_back(shader_resource);
    }
}
void ShaderModule::readSamplers(spirv_cross::CompilerGLSL* compiler)
{
    auto shaderSamplers = compiler->get_shader_resources().sampled_images;
    for (auto& resource : shaderSamplers)
    {
        ShaderResource shader_resource{};
        shader_resource.type = ShaderResourceType::ImageSampler;
        shader_resource.stages = m_Stage;
        shader_resource.name = resource.name;

        const auto& spirv_type = compiler->get_type_from_variable(resource.id);
        shader_resource.array_size = spirv_type.array.size() ? spirv_type.array[0] : 1;

        shader_resource.set = compiler->get_decoration(resource.id, spv::DecorationDescriptorSet);
        shader_resource.binding = compiler->get_decoration(resource.id, spv::DecorationBinding);
        m_Resources.push_back(shader_resource);
    }
}
void ShaderModule::readUniforms(spirv_cross::CompilerGLSL* compiler)
{
    auto shaderUniforms = compiler->get_shader_resources().uniform_buffers;
    for (auto& resource : shaderUniforms)
    {
        ShaderResource shader_resource{};
        shader_resource.type = ShaderResourceType::BufferUniform;
        shader_resource.stages = m_Stage;
        shader_resource.name = resource.name;

        const auto& spirv_type = compiler->get_type_from_variable(resource.id);
        size_t array_size = 0;
        shader_resource.size = compiler->get_declared_struct_size_runtime_array(spirv_type, array_size);
        shader_resource.array_size = spirv_type.array.size() ? spirv_type.array[0] : 1;
        shader_resource.set = compiler->get_decoration(resource.id, spv::DecorationDescriptorSet);
        shader_resource.binding = compiler->get_decoration(resource.id, spv::DecorationBinding);
        m_Resources.push_back(shader_resource);

    }
}
void ShaderModule::readPushConstants(spirv_cross::CompilerGLSL* compiler)
{
    auto pushConstantBuffers = compiler->get_shader_resources().push_constant_buffers;
    for (auto& resource : pushConstantBuffers)
    {
        ShaderResource shader_resource{};
        shader_resource.type = ShaderResourceType::PushConstant;
        shader_resource.stages = m_Stage;
        shader_resource.name = resource.name;

        const auto& spirv_type = compiler->get_type_from_variable(resource.id);
        std::uint32_t offset = std::numeric_limits<std::uint32_t>::max();

        for (auto i = 0U; i < spirv_type.member_types.size(); ++i)
        {
            auto mem_offset = compiler->get_member_decoration(spirv_type.self, i, spv::DecorationOffset);

            offset = std::min(offset, mem_offset);
        }
        shader_resource.offset = offset;

        
        size_t array_size = 0;
        shader_resource.size = compiler->get_declared_struct_size_runtime_array(spirv_type, array_size) - offset;
        shader_resource.array_size = 0;

        m_Resources.push_back(shader_resource);

    }
}

std::weak_ptr<ShaderSource> ShaderSourcePool::getShaderSource(std::string shaderPath)
{

   
    auto it = m_ShaderSources.find(shaderPath);
    if (it != m_ShaderSources.end())
        return it->second;

    
    auto insertionResult = m_ShaderSources.emplace(shaderPath, std::shared_ptr<ShaderSource>(new ShaderSource(shaderPath)));
    if(insertionResult.second)
        it = insertionResult.first;
    return it->second;
   
}

void ShaderSourcePool::reloadShader(std::string shaderPath)
{

    m_ShaderSources.erase(shaderPath);
    getShaderSource(shaderPath);

    /*std::vector<std::string> paths;
    for (int i = 0;i<m_ShaderSources.size();i++)
    {
        paths.push_back(m_ShaderSources[i]->get_filename());
        m_ShaderSources[i].reset();
    }
    m_ShaderSources.clear();
    for (auto path : paths)
    {
        getShaderSource(path);//this should regenerate everything, TODO: Obviously we are not changing the spv yet, this should be done inside shadersource 
    }*/
}
