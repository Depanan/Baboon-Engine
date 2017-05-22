#pragma once
#define GLFW_INCLUDE_VULKAN
#define GLFW_EXPOSE_NATIVE_WIN32
#include <GLFW/glfw3.h>
#include <GLFW/glfw3native.h>
#include <iostream>
#include "Renderer\Vulkan\RendererVulkan.h"
#include "Renderer\RendererAbstract.h"
#include <vector>
#include <array>
#include <set>
#include <iostream>
#include "Core\ServiceLocator.h"


#define WINDOW_W 800
#define WINDOW_H 600
