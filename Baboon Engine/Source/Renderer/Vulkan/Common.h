#pragma once
#include "vulkan/vulkan.h"
#include <vector>
#include <string>
#include <map>


bool is_depth_only_format(VkFormat format);
bool is_depth_stencil_format(VkFormat format);

std::vector<uint32_t> readFile(const std::string& filename);
std::vector<uint8_t> readFileUint8(const std::string& filename);

template <class T>
using BindingMap = std::map<uint32_t, std::map<uint32_t, T>>;
