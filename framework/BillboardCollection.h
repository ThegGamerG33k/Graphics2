#pragma once

#include "vkhelpers.h"
#include "math2801.h"

class BlitSquare;
class Buffer;
class Image;
class DescriptorSet;
class VertexManager;

class BillboardCollection {
public:

    BillboardCollection(VulkanContext* ctx_,
        VertexManager* vertexManager,
        const std::vector<math2801::vec4>& positions,
        Image* img
    );

    void draw(VkCommandBuffer cmd,
        DescriptorSet* descriptorSet
    );

    void drawInstanced(VkCommandBuffer cmd,
        unsigned numInstances);

    unsigned numBillboards;
    Image* img;
    BlitSquare* blitSquare;
    VkBufferView positionView;
    Buffer* positions;
    //...data items...
};