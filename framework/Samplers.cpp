#include "Samplers.h"
#include "CleanupManager.h"


Sampler::Sampler(VulkanContext* ctx_, VkFilter minMagFilter, VkSamplerAddressMode repeatMode, bool useMipmap)
{
    this->ctx=ctx_;
    check(
        vkCreateSampler(
            ctx->dev,
            VkSamplerCreateInfo{
                .sType=VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO,
                .pNext=nullptr,
                .flags=0,
                .magFilter=minMagFilter,
                .minFilter=minMagFilter,
                .mipmapMode=VK_SAMPLER_MIPMAP_MODE_LINEAR,
                .addressModeU = repeatMode,
                .addressModeV = repeatMode,
                .addressModeW = repeatMode,
                .mipLodBias=0,
                .anisotropyEnable=VK_FALSE, //(useMipmap ? VK_TRUE : VK_FALSE),
                .maxAnisotropy=4,
                .compareEnable=VK_FALSE,
                .compareOp=VK_COMPARE_OP_ALWAYS,
                .minLod=0,
                .maxLod=( useMipmap ? VK_LOD_CLAMP_NONE : 0 ),
                .borderColor=VK_BORDER_COLOR_INT_OPAQUE_BLACK,
                .unnormalizedCoordinates=VK_FALSE,
            },
            nullptr,
            &(this->sampler)
    ));
    CleanupManager::registerCleanupFunction( [this](){
        vkDestroySampler(this->ctx->dev, this->sampler, nullptr );
    });

}
