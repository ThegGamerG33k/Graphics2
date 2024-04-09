#pragma once
#include "Images.h"
#include "shaders/importantconstants.h"
#include "CleanupManager.h"
#include "Globals.h"


class ParticleSystem {
public:
    VulkanContext* ctx;
    unsigned numParticles;
    BillboardCollection* billboardCollection;
    float accumulated = 0.0f;
    math2801::vec3 startingPoint;

    VkBufferView velocityView;
    Buffer* velocities;

    ParticleSystem(
        VulkanContext* ctx,
        VertexManager* vertexManager,
        unsigned numParticles,
        math2801::vec3 startingPoint,
        Image* img
    );

    void draw(VkCommandBuffer cmd,
        DescriptorSet* descriptorSet);

    void initialize(VulkanContext* ctx);

    void update(float elapsed);

    void compute(VkCommandBuffer cmd);

    static void barrier(VkCommandBuffer cmd,
        VkPipelineStageFlagBits stage1,
        VkPipelineStageFlagBits stage2,
        std::initializer_list<Buffer*> buffers);
};