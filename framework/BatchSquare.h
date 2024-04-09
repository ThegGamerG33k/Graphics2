#pragma once
#include "vkhelpers.h"
#include "VertexManager.h"
#include "math2801.h"

//Forward declaration
class DescriptorSet;
class Image;

class BatchSquare {
public:
    VertexManager::Info drawinfo;

    BatchSquare(VertexManager* vertexManager, math2801::vec2 size);

    BatchSquare(const BatchSquare&) = delete;
    void operator=(const BatchSquare&) = delete;

    void drawInstanced(VkCommandBuffer cmd,
        unsigned numInstances);
};
