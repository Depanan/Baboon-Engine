
#include "DescriptorSetLayout.h"
#include "../Device.h"
#include "Shader.h"
#include "Core/ServiceLocator.h"

VkDescriptorType find_descriptor_type(ShaderResourceType resource_type, bool dynamic)
{
    switch (resource_type)
    {
    case ShaderResourceType::InputAttachment:
        return VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT;
        break;
    case ShaderResourceType::Image:
        return VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE;
        break;
    case ShaderResourceType::ImageSampler:
        return VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
        break;
    case ShaderResourceType::ImageStorage:
        return VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
        break;
    case ShaderResourceType::Sampler:
        return VK_DESCRIPTOR_TYPE_SAMPLER;
        break;
    case ShaderResourceType::BufferUniform:
        if (dynamic)
        {
            return VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC;
        }
        else
        {
            return VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
        }
        break;
    case ShaderResourceType::BufferStorage:
        if (dynamic)
        {
            return VK_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC;
        }
        else
        {
            return VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
        }
        break;
    default:
        throw std::runtime_error("No conversion possible for the shader resource type.");
        break;
    }
}


DescriptorSetLayout::DescriptorSetLayout(const Device& device, const std::vector<ShaderResource>& resource_set)
    :m_Device(device)
    
{
    //TODO: Fix this
    for (auto& resource : resource_set)
    {
        // Skip shader resources whitout a binding point
        if (resource.type == ShaderResourceType::Input ||
            resource.type == ShaderResourceType::Output ||
            resource.type == ShaderResourceType::PushConstant ||
            resource.type == ShaderResourceType::SpecializationConstant)
        {
            continue;
        }

        // Convert from ShaderResourceType to VkDescriptorType.
        auto descriptor_type = find_descriptor_type(resource.type, resource.mode == ShaderResourceMode::Dynamic);

        /*if (resource.mode == ShaderResourceMode::UpdateAfterBind)
        {
            binding_flags.push_back(VK_DESCRIPTOR_BINDING_UPDATE_AFTER_BIND_BIT_EXT);
        }
        else
        {
            // When creating a descriptor set layout, if we give a structure to create_info.pNext, each binding needs to have a binding flag
            // (pBindings[i] uses the flags in pBindingFlags[i])
            // Adding 0 ensures the bindings that dont use any flags are mapped correctly.
            binding_flags.push_back(0);
        }
        */
        // Convert ShaderResource to VkDescriptorSetLayoutBinding
        VkDescriptorSetLayoutBinding layout_binding{};

        layout_binding.binding = resource.binding;
        layout_binding.descriptorCount = resource.array_size;
        layout_binding.descriptorType = descriptor_type;
        layout_binding.stageFlags = static_cast<VkShaderStageFlags>(resource.stages);

        m_Bindings.push_back(layout_binding);

        // Store mapping between binding and the binding point
        m_Bindings_Lookup.emplace(resource.binding, layout_binding);

        //binding_flags_lookup.emplace(resource.binding, binding_flags.back());

        m_ResourcesLookup.emplace(resource.name, resource.binding);
    }

    VkDescriptorSetLayoutCreateInfo create_info{ VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO };
    create_info.flags = 0;
    create_info.bindingCount = m_Bindings.size();
    create_info.pBindings = m_Bindings.data();

    // Handle update-after-bind extensions
   /*if (std::find_if(resource_set.begin(), resource_set.end(),
        [](const ShaderResource& shader_resource) { return shader_resource.mode == ShaderResourceMode::UpdateAfterBind; }) != resource_set.end())
    {
        // Spec states you can't have ANY dynamic resources if you have one of the bindings set to update-after-bind
        if (std::find_if(resource_set.begin(), resource_set.end(),
            [](const ShaderResource& shader_resource) { return shader_resource.mode == ShaderResourceMode::Dynamic; }) != resource_set.end())
        {
            throw std::runtime_error("Cannot create descriptor set layout, dynamic resources are not allowed if at least one resource is update-after-bind.");
        }

        if (!validate_flags(device.get_gpu(), bindings, binding_flags))
        {
            throw std::runtime_error("Invalid binding set up, couldn't create descriptor set layout.");
        }

        VkDescriptorSetLayoutBindingFlagsCreateInfoEXT binding_flags_create_info{ VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_BINDING_FLAGS_CREATE_INFO_EXT };
        binding_flags_create_info.bindingCount = to_u32(binding_flags.size());
        binding_flags_create_info.pBindingFlags = binding_flags.data();

        create_info.pNext = &binding_flags_create_info;
        create_info.flags |= std::find(binding_flags.begin(), binding_flags.end(), VK_DESCRIPTOR_BINDING_UPDATE_AFTER_BIND_BIT_EXT) != binding_flags.end() ? VK_DESCRIPTOR_SET_LAYOUT_CREATE_UPDATE_AFTER_BIND_POOL_BIT_EXT : 0;
    }*/

    // Create the Vulkan descriptor set layout handle
    VkResult result = vkCreateDescriptorSetLayout(m_Device.get_handle(), &create_info, nullptr, &m_DescriptorSetLayout);

    if (result != VK_SUCCESS)
    {
        LOGERROR("Cannot create DescriptorSetLayout" );
    }
}

DescriptorSetLayout::DescriptorSetLayout(DescriptorSetLayout&& other)
    :m_Device(other.m_Device),
    m_DescriptorSetLayout(other.m_DescriptorSetLayout),
    m_Bindings_Lookup(other.m_Bindings_Lookup),
    m_ResourcesLookup(other.m_ResourcesLookup),
    m_Bindings(other.m_Bindings)

{
    other.m_DescriptorSetLayout = VK_NULL_HANDLE;
}

DescriptorSetLayout::~DescriptorSetLayout()
{
    if(m_DescriptorSetLayout)
        vkDestroyDescriptorSetLayout(m_Device.get_handle(), m_DescriptorSetLayout, nullptr);
}

std::unique_ptr<VkDescriptorSetLayoutBinding> DescriptorSetLayout::getLayoutBinding(const uint32_t binding_index) const
{
    auto it = m_Bindings_Lookup.find(binding_index);

    if (it == m_Bindings_Lookup.end())
    {
        return nullptr;
    }

    return std::make_unique<VkDescriptorSetLayoutBinding>(it->second);
}

std::unique_ptr<VkDescriptorSetLayoutBinding> DescriptorSetLayout::getLayoutBinding(const std::string& name) const
{
    auto it = m_ResourcesLookup.find(name);

    if (it == m_ResourcesLookup.end())
    {
        return nullptr;
    }

    return getLayoutBinding(it->second);
}

