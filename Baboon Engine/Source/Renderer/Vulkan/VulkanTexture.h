#pragma once
#include "../Common/Texture.h"
#include "VulkanImage.h"
#include "VulkanImageView.h"
#include "VulkanSampler.h"

class Device;
class VulkanTexture: public Texture
{
public:
    VulkanTexture() { }

    void setImage(VulkanImage* image) { m_Image = image; }
    void setImageView(VulkanImageView* imageView) { m_View = imageView; }
    void setSampler(VulkanSampler* sampler) { m_Sampler = sampler; }

    VulkanImage* getImage() { return m_Image; }
    VulkanSampler* getSampler() { return m_Sampler; }
    VulkanImageView* getImageView() { return m_View; }

private:
    VulkanImage* m_Image{ nullptr };
    VulkanSampler* m_Sampler{ nullptr };
    VulkanImageView* m_View{ nullptr };
};