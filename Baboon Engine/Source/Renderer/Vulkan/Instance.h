#pragma once

#include "Common.h"
#include <vector>

class Instance
{
public:
    Instance(const std::vector<const char*>& required_extensions, bool bValidationLayers);
    ~Instance();

    
    VkInstance get_handle();
    
    //Disabling undesired copy constructors and undesired functionality
    Instance(const Instance&) = delete;
    Instance(Instance&&) = delete;
    Instance& operator=(const Instance&) = delete;
    Instance& operator=(Instance&&) = delete;

private:
    VkInstance m_Handle{ VK_NULL_HANDLE };

    VkDebugReportCallbackEXT m_DebugCallback{ VK_NULL_HANDLE };
};
