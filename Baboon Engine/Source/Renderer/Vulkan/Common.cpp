
#include "Common.h"
#include <fstream>

bool is_depth_only_format(VkFormat format)
{
    return format == VK_FORMAT_D16_UNORM ||
        format == VK_FORMAT_D32_SFLOAT;
}

bool is_depth_stencil_format(VkFormat format)
{
    return format == VK_FORMAT_D16_UNORM_S8_UINT ||
        format == VK_FORMAT_D24_UNORM_S8_UINT ||
        format == VK_FORMAT_D32_SFLOAT_S8_UINT ||
        is_depth_only_format(format);
}

std::vector<uint32_t> readFile(const std::string& filename)
{
    std::vector<uint32_t> data;

    std::ifstream file;

    file.open(filename, std::ios::in | std::ios::binary);

    if (!file.is_open())
    {
        throw std::runtime_error("Failed to open file: " + filename);
    }

    uint64_t read_count = 0;
   
   
    file.seekg(0, std::ios::end);
    read_count = static_cast<uint64_t>(file.tellg());
    size_t realsize = static_cast<size_t>(read_count) / sizeof(uint32_t);
    file.seekg(0, std::ios::beg);
    

    data.resize(static_cast<size_t>(realsize));
    file.read(reinterpret_cast<char*>(data.data()), read_count);
    file.close();

    return data;
}
std::vector<uint8_t> readFileUint8(const std::string& filename)
{
    std::vector<uint8_t> data;

    std::ifstream file;

    file.open(filename, std::ios::in | std::ios::binary);

    if (!file.is_open())
    {
        throw std::runtime_error("Failed to open file: " + filename);
    }

    uint64_t read_count = 0;


    file.seekg(0, std::ios::end);
    read_count = static_cast<uint64_t>(file.tellg());
    size_t realsize = static_cast<size_t>(read_count) / sizeof(uint8_t);
    file.seekg(0, std::ios::beg);


    data.resize(static_cast<size_t>(realsize));
    file.read(reinterpret_cast<char*>(data.data()), read_count);
    file.close();

    return data;
}
