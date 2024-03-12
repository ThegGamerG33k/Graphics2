#include "PipelineLayout.h"
#include "PushConstants.h"
#include "mischelpers.h"
#include "VertexManager.h"
#include "Descriptors.h"
#include "CleanupManager.h"
#include "Framebuffer.h"
#include "RenderPass.h"
#include "utils.h"
#include <cstring>
#include <assert.h>
#include <optional>
#include <array>

PipelineLayout::PipelineLayout(
    VulkanContext* ctx,
    unsigned pushConstantSize_,
    DescriptorSetLayout* descriptorSetLayout_,
    std::string name_
){

    this->name=name_;
    this->descriptorSetLayout=descriptorSetLayout_;
    this->pushConstantSize = pushConstantSize_;

    VkPushConstantRange pushConstantRange{
        .stageFlags=VK_SHADER_STAGE_ALL,
        .offset=0,
        .size=this->pushConstantSize
    };

    check(vkCreatePipelineLayout(
        ctx->dev,
        VkPipelineLayoutCreateInfo{
            .sType=VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO,
            .pNext=nullptr,
            .flags=0,
            .setLayoutCount= unsigned(this->descriptorSetLayout ? 1:0 ),
            .pSetLayouts=(this->descriptorSetLayout ? &(this->descriptorSetLayout->layout) : nullptr),
            .pushConstantRangeCount=unsigned( (pushConstantSize>0) ? 1 : 0),
            .pPushConstantRanges=&pushConstantRange
        },
        nullptr,
        &(this->pipelineLayout)
    ));

    ctx->setObjectName(this->pipelineLayout,"pipeline layout "+name);

    CleanupManager::registerCleanupFunction( [this,ctx](){
        vkDestroyPipelineLayout(
            ctx->dev,
            this->pipelineLayout,
            nullptr
        );
    });

}
