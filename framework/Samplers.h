#pragma once

#include "vkhelpers.h"

class Sampler{
  public:
    Sampler(VulkanContext* ctx, VkFilter minMagFilter, VkSamplerAddressMode repeatMode, bool useMipmap);

    VulkanContext* ctx;
    VkSampler sampler;
};
