#include "Image.h"
#define STB_IMAGE_IMPLEMENTATION
#include "stb\stb_image.h"


void Image::Init(const char* i_sPath)
{
	int texChannels;
	m_Pixels = stbi_load(i_sPath, &m_iWidth, &m_iHeight, &texChannels, STBI_rgb_alpha);
}