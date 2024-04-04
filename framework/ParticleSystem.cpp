#include "BillboardCollection.h"
#include "VertexManager.h"
#include "Buffers.h"
#include "BatchSquare.h"
#include "Descriptors.h"
#include "Samplers.h"
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
        VulkanContext* ctx_,
        VertexManager* vertexManager,
        unsigned numParticles_, vec3 startingPoint_,
        Image* img
    ) {
        if (computePipeline == nullptr) {
            initialize(ctx);
        }
        this->ctx = ctx_;
        this->numParticles = numParticles_;
        this->startingPoint = startingPoint_;

        std::vector<vec4> vel(numParticles_);
        for (unsigned i = 0; i < numParticles_; ++i) {
            //uniform() is in math2801.h
            vec4 v(uniform(-0.15f, 0.15f),
                uniform(0, 1.0f),
                uniform(-0.15f, 0.15f),
                0.0f);
            vel[i] = v;
        }

        this->billboardCollection = new BillboardCollection(
            ctx, vertexManager, pos, img);

        this->velocities = new DeviceLocalBuffer(ctx, vel,
            VK_BUFFER_USAGE_UNIFORM_TEXEL_BUFFER_BIT |
            VK_BUFFER_USAGE_STORAGE_TEXEL_BUFFER_BIT,
            "psystem vel"
        );

        this->velocityView = this->velocities->makeView(
            VK_FORMAT_R32G32B32A32_SFLOAT,
            "velocity view"
        );

        static void barrier(VkCommandBuffer cmd,
            VkPipelineStageFlagBits stage1,
            VkPipelineStageFlagBits stage2,
            std::initializer_list<Buffer*> buffers);

        void compute(VkCommandBuffer cmd);

        CleanupManager::registerCleanupFunction([this]() {
            this->velocities->cleanup();
            vkDestroyBufferView(this->ctx->dev, this->velocityView, nullptr);
            });
    }

    void draw(VkCommandBuffer cmd,
        DescriptorSet* descriptorSet)
    {
        this->billboardCollection->draw(cmd, descriptorSet);
    }

    void initialize(VulkanContext* ctx)
    {
        computePushConstants = new PushConstants("shaders/particlePushConstants.txt");

        computeDescriptorSetLayout = new DescriptorSetLayout(
            ctx,
            {
                DescriptorSetEntry(VK_DESCRIPTOR_TYPE_STORAGE_TEXEL_BUFFER,
                                    0),
                DescriptorSetEntry(VK_DESCRIPTOR_TYPE_STORAGE_TEXEL_BUFFER,
                                    1)
            }
        );

        computePipelineLayout = new PipelineLayout(
            ctx,
            computePushConstants->byteSize,
            computeDescriptorSetLayout,
            "computePipelineLayout"
        );

        computePipeline = new ComputePipeline(
            ctx,
            computePipelineLayout,
            ShaderManager::load("shaders/particle.comp"),
            "particle update pipe"
        );

        computeDescriptorSetFactory = new DescriptorSetFactory(
            ctx,
            "particle dsf",
            0,      //binding point
            computePipelineLayout
        );
        computeDescriptorSet = computeDescriptorSetFactory->make();
    }


    void update(float elapsed)
    {
        this->accumulated += elapsed;
    }


    void compute(VkCommandBuffer cmd)
    {
        if (this->accumulated < PARTICLE_TIME_QUANTUM)
            return;

        if (numParticles_ % WORKGROUP_SIZE != 0) {
            //fatal is in consoleoutput.h
            fatal("Error:", numParticles_,
                "is not a multiple of", WORKGROUP_SIZE
            );
        }

        computeDescriptorSet->setSlot(0, this->billboardCollection->positionView);
        computeDescriptorSet->setSlot(1, this->velocityView);
        computeDescriptorSet->bind(cmd);
        computePipeline->use(cmd);
        computePushConstants->set(cmd, "startingPoint", this->startingPoint);
        computePushConstants->set(cmd, "elapsed", PARTICLE_TIME_QUANTUM);

        while (this->accumulated >= PARTICLE_TIME_QUANTUM) {
            this->accumulated -= PARTICLE_TIME_QUANTUM;
            vkCmdDispatch(cmd,
                this->numParticles / WORKGROUP_SIZE,
                1,
                1
            );

            barrier(cmd,
                VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,
                VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,
                {
                    this->billboardCollection->positions,
                    this->velocities
                }
            );
        }

        barrier(cmd,
            VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,
            VK_PIPELINE_STAGE_VERTEX_SHADER_BIT,
            {
                this->billboardCollection->positions,
                this->velocities
            }
        );
    }

    void barrier(VkCommandBuffer cmd,
        VkPipelineStageFlagBits stage1,
        VkPipelineStageFlagBits stage2,
        std::initializer_list<Buffer*> buffers
    ) {

        //ensure all reads & writes from stage1 stage complete
        //before any reads or writes from stage2 stage
        VkMemoryBarrier mb = VkMemoryBarrier{
            .sType = VK_STRUCTURE_TYPE_MEMORY_BARRIER,
            .pNext = nullptr,
            .srcAccessMask = VK_ACCESS_MEMORY_READ_BIT | VK_ACCESS_MEMORY_WRITE_BIT,
            .dstAccessMask = VK_ACCESS_MEMORY_READ_BIT | VK_ACCESS_MEMORY_WRITE_BIT
        };
        std::vector<VkBufferMemoryBarrier> membars;
        for (Buffer* b : buffers) {
            membars.push_back(
                VkBufferMemoryBarrier{
                    .sType = VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER,
                    .pNext = nullptr,
                    .srcAccessMask = VK_ACCESS_MEMORY_READ_BIT | VK_ACCESS_MEMORY_WRITE_BIT,
                    .dstAccessMask = VK_ACCESS_MEMORY_READ_BIT | VK_ACCESS_MEMORY_WRITE_BIT,
                    .srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
                    .dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
                    .buffer = b->buffer,
                    .offset = 0,
                    .size = VK_WHOLE_SIZE
                }
            );
        }
        vkCmdPipelineBarrier(
            cmd,
            stage1, stage2,
            0,      //dependency flags
            1, &mb, //memory barriers
            (unsigned)membars.size(), membars.data(),   //buffer barriers
            0, nullptr      //image memory barriers
        );
    }
};


PushConstants* computePushConstants;
ComputePipeline* computePipeline;
PipelineLayout* computePipelineLayout;
DescriptorSetFactory* computeDescriptorSetFactory;
DescriptorSetLayout* computeDescriptorSetLayout;
DescriptorSet* computeDescriptorSet;