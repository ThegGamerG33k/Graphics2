#include "vkhelpers.h"
#include "ComputePipeline.h"
#include "PushConstants.h"
#include "mischelpers.h"
#include "Descriptors.h"
#include "CleanupManager.h"
#include <cstring>
#include <assert.h>

ComputePipeline::ComputePipeline(
    VulkanContext* ctx_,
    PipelineLayout* pipelineLayout_,
    VkPipelineShaderStageCreateInfo computeShader_,
    std::string name_ ) : Pipeline(ctx_,pipelineLayout_,VK_PIPELINE_BIND_POINT_COMPUTE,name_)
{
    this->computeShader=computeShader_;
}

void ComputePipeline::finishInit()
{

    assert(this->pipelines.empty());

    VkComputePipelineCreateInfo info{
        .sType = VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO,
        .pNext = nullptr,
        .flags = 0,
        .stage = this->computeShader,
        .layout = this->pipelineLayout->pipelineLayout,
        .basePipelineHandle = VK_NULL_HANDLE,
        .basePipelineIndex = 0
    };

    VkPipeline pipeline;

    check(vkCreateComputePipelines( this->ctx->dev,
        nullptr,    //cache
        1,          //count
        &info,
        nullptr,    //allocators
        &pipeline
    ));


    ctx->setObjectName(pipeline, this->name);

    this->pipelines[ std::make_pair( (Framebuffer*)nullptr, (RenderPass*)nullptr ) ] = pipeline;

}



void ComputePipeline::addFramebuffer(Framebuffer* fb, RenderPass* renderPass)
{
    //framebuffer and renderpass are irrelevant for compute shader,
    //so we always use the same vulkan pipeline object
    auto key = std::make_pair( fb,renderPass );
    this->pipelines[key] = this->pipelines[ std::make_pair( (Framebuffer*)nullptr, (RenderPass*)nullptr ) ];
}
