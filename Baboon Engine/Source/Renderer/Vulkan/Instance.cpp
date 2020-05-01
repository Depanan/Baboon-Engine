#include "Instance.h"
#include <cassert>
#include <iostream>

static bool checkValidationLayerSupport(const std::vector<const char*>& validationLayers) {
    uint32_t layerCount;
    vkEnumerateInstanceLayerProperties(&layerCount, nullptr);

    std::vector<VkLayerProperties> availableLayers(layerCount);
    vkEnumerateInstanceLayerProperties(&layerCount, availableLayers.data());

    for (const char* layerName : validationLayers) {
        bool layerFound = false;

        for (const auto& layerProperties : availableLayers) {
            if (strcmp(layerName, layerProperties.layerName) == 0) {
                layerFound = true;
                break;
            }
        }

        if (!layerFound) {
            return false;
        }
    }

    return true;
}

static VKAPI_ATTR VkBool32 VKAPI_CALL debugCallback(
    VkDebugReportFlagsEXT flags,
    VkDebugReportObjectTypeEXT objType,
    uint64_t obj,
    size_t location,
    int32_t code,
    const char* layerPrefix,
    const char* msg,
    void* userData) {

    std::cerr << "\nvalidation layer: " << msg << std::endl;

    return VK_FALSE;
}


Instance::Instance(const std::vector<const char*>& i_required_extensions, bool i_bValidationLayers)
{

    VkApplicationInfo applicationInfo{ VK_STRUCTURE_TYPE_APPLICATION_INFO };
    
    applicationInfo.pApplicationName = "Baboon Engine";
    applicationInfo.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
    applicationInfo.pEngineName = "Baboon Engine";
    applicationInfo.engineVersion = VK_MAKE_VERSION(1, 0, 0);
    applicationInfo.apiVersion = VK_API_VERSION_1_0;



    VkInstanceCreateInfo instanceInfo = { VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO };

    instanceInfo.pApplicationInfo = &applicationInfo;
    instanceInfo.enabledExtensionCount = i_required_extensions.size();
    instanceInfo.ppEnabledExtensionNames = i_required_extensions.data();
    instanceInfo.flags = 0;

    const std::vector<const char*> validationLayers = {
             "VK_LAYER_LUNARG_standard_validation"
    };
    if (i_bValidationLayers)
    {
        assert(checkValidationLayerSupport(validationLayers));
        instanceInfo.enabledLayerCount = validationLayers.size();
        instanceInfo.ppEnabledLayerNames = validationLayers.data();
    }
    else
    {
        instanceInfo.enabledLayerCount = 0;
    }
    //instanceInfo.pNext = nullptr;

    VkResult result = vkCreateInstance(&instanceInfo, nullptr, &m_Handle);
    assert(result == VK_SUCCESS);



#ifndef NDEBUG
    VkDebugReportCallbackCreateInfoEXT createInfo = {};
    createInfo.sType = VK_STRUCTURE_TYPE_DEBUG_REPORT_CALLBACK_CREATE_INFO_EXT;
    createInfo.flags = VK_DEBUG_REPORT_ERROR_BIT_EXT | VK_DEBUG_REPORT_WARNING_BIT_EXT;
    createInfo.pfnCallback = debugCallback;

    auto func = (PFN_vkCreateDebugReportCallbackEXT)vkGetInstanceProcAddr(m_Handle, "vkCreateDebugReportCallbackEXT");
    if (func != nullptr) {
        VkResult  res = func(m_Handle, &createInfo, nullptr, &m_DebugCallback);
        if (res != VK_SUCCESS)
            throw std::runtime_error("failed to set up debug callback!");
    }
    else {
        throw std::runtime_error("failed to set up debug callback!");
    }

#endif

}



Instance::~Instance()
{
    if (m_DebugCallback != VK_NULL_HANDLE)
    {
        auto func = (PFN_vkDestroyDebugReportCallbackEXT)vkGetInstanceProcAddr(m_Handle, "vkDestroyDebugReportCallbackEXT");
        if (func != nullptr)
            func(m_Handle, m_DebugCallback, nullptr);
    }

    if (m_Handle != VK_NULL_HANDLE)
    {
        vkDestroyInstance(m_Handle, nullptr);
    }
}

VkInstance Instance::get_handle()
{
    return m_Handle;
}
