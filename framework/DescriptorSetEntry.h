#pragma once
#include "vkhelpers.h"

/// Description for one descriptor set entry.
struct DescriptorSetEntry{
    VkDescriptorType type;          /// Resource type (ex: VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE).
    int slot;                       /// Slot in the descriptor set; must be nonnegative.
};
