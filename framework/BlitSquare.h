#pragma once
#include "vkhelpers.h"
#include "VertexManager.h"

//Forward declaration
class DescriptorSet;
class Image;

class BlitSquare {
public:
    VertexManager::Info drawinfo;
    BlitSquare(VertexManager* vertexManager);
    void draw(VulkanContext* ctx, VkCommandBuffer cmd, DescriptorSet* descriptorSet, Image* img);

    BlitSquare(const BlitSquare&) = delete;
    void operator=(const BlitSquare&) = delete;

    void drawInstanced(VkCommandBuffer cmd,
        unsigned numInstances);
};
