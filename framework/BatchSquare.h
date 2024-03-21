#pragma once
#include "vkhelpers.h"
#include "VertexManager.h"

//Forward declaration
class DescriptorSet;
class Image;

class BatchSquare {
public:
    VertexManager::Info drawinfo;
    BatchSquare(VertexManager* vertexManager);

    BatchSquare(const BatchSquare&) = delete;
    void operator=(const BatchSquare&) = delete;

    void drawInstanced(VkCommandBuffer cmd,
        unsigned numInstances);
};
