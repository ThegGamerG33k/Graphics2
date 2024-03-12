#include "GraphicsPipeline.h"
#include <limits>
#include "Framebuffer.h"
#include <cassert>
#include <algorithm>
#include "mischelpers.h"



void GraphicsPipeline::_setDefaults()
{
    this->vertexInputState = VkPipelineVertexInputStateCreateInfo{
        .sType=VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO,
        .pNext=nullptr,
        .flags=0,
        .vertexBindingDescriptionCount=0,
        .pVertexBindingDescriptions=nullptr,
        .vertexAttributeDescriptionCount=0,
        .pVertexAttributeDescriptions=nullptr
    };
    this->inputAssemblyState = VkPipelineInputAssemblyStateCreateInfo {
        .sType=VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO,
        .pNext=nullptr,
        .flags=0,
        .topology=VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST,
        .primitiveRestartEnable=0
    };

    this->viewport = VkViewport{
        .x=0,
        .y=0,
        .width=std::numeric_limits<float>::max(),
        .height=std::numeric_limits<float>::max(),
        .minDepth=0.0f,
        .maxDepth=1.0f
    };

    this->scissor = VkRect2D{
        .offset=VkOffset2D{
            .x=0,
            .y=0
         },
        .extent=VkExtent2D{
            .width=0x7fffffff,
            .height=0x7fffffff
        }
    };

    this->rasterizationState = VkPipelineRasterizationStateCreateInfo {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO,
        .pNext=nullptr,
        .flags=0,
        .depthClampEnable=0,
        .rasterizerDiscardEnable=0,
        .polygonMode = VK_POLYGON_MODE_FILL,
        .cullMode = VK_CULL_MODE_NONE  ,
        .frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE,
        .depthBiasEnable=0,
        .depthBiasConstantFactor=0.0f,
        .depthBiasClamp=0.0f,
        .depthBiasSlopeFactor=0.0f,
        .lineWidth=1.0f
    };

    this->multisampleState = VkPipelineMultisampleStateCreateInfo {
        .sType=VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO,
        .pNext=nullptr,
        .flags=0,
        .rasterizationSamples = VK_SAMPLE_COUNT_1_BIT,
        .sampleShadingEnable=0,
        .minSampleShading = 0.0f,
        .pSampleMask = nullptr,
        .alphaToCoverageEnable=0,
        .alphaToOneEnable=0
    };

    this->depthStencilState = VkPipelineDepthStencilStateCreateInfo {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO,
        .pNext=nullptr,
        .flags=0,
        .depthTestEnable = VK_TRUE,
        .depthWriteEnable = VK_TRUE,
        .depthCompareOp =VK_COMPARE_OP_LESS_OR_EQUAL,
        .depthBoundsTestEnable = VK_FALSE,
        .stencilTestEnable = VK_FALSE,
        .front = VkStencilOpState{
            .failOp=VK_STENCIL_OP_KEEP,
            .passOp=VK_STENCIL_OP_KEEP,
            .depthFailOp=VK_STENCIL_OP_KEEP,
            .compareOp=VK_COMPARE_OP_ALWAYS,
            .compareMask=0xff,
            .writeMask=0xff,
            .reference=0
        },
        .back = VkStencilOpState{
            .failOp=VK_STENCIL_OP_KEEP,
            .passOp=VK_STENCIL_OP_KEEP,
            .depthFailOp=VK_STENCIL_OP_KEEP,
            .compareOp=VK_COMPARE_OP_ALWAYS,
            .compareMask=0xff,
            .writeMask=0xff,
            .reference=0
        },
        .minDepthBounds=0.0f,
        .maxDepthBounds=1.0f
    };

    this->colorBlendState = VkPipelineColorBlendStateCreateInfo{
        .sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO,
        .pNext=nullptr,
        .flags=0,
        .logicOpEnable=0,
        .logicOp = VK_LOGIC_OP_CLEAR,
        .attachmentCount=0,         //updated in finishInit()
        .pAttachments=nullptr,      //updated in finishInit()
        .blendConstants = {0.0f, 0.0f, 0.0f, 0.0f }
    };

    this->oneBlendAttachmentState = VkPipelineColorBlendAttachmentState{
        .blendEnable=0,
        .srcColorBlendFactor = VK_BLEND_FACTOR_ONE,
        .dstColorBlendFactor = VK_BLEND_FACTOR_ZERO,
        .colorBlendOp = VK_BLEND_OP_ADD,
        .srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE,
        .dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO,
        .alphaBlendOp =  VK_BLEND_OP_ADD,
        .colorWriteMask = (
            VK_COLOR_COMPONENT_R_BIT |
            VK_COLOR_COMPONENT_G_BIT |
            VK_COLOR_COMPONENT_B_BIT |
            VK_COLOR_COMPONENT_A_BIT
        )
    };

    //~ this->viewportState = VkPipelineViewportStateCreateInfo {
        //~ .sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO,
        //~ .pNext = nullptr,
        //~ .flags=0,
        //~ .viewportCount=0,       //set at finishInit time
        //~ .pViewports=nullptr,    //set at finishInit time
        //~ .scissorCount=0,        //set at finishInit time
        //~ .pScissors=nullptr      //set at finishInit time
    //~ };

    this->hasTessellation=false;
    this->tessellationState = VkPipelineTessellationStateCreateInfo{
        .sType = VK_STRUCTURE_TYPE_PIPELINE_TESSELLATION_STATE_CREATE_INFO,
        .pNext=nullptr,
        .flags=0,
        .patchControlPoints=0
    };
}

GraphicsPipeline* GraphicsPipeline::set(const PipelineOption& opt)
{
    this->checkSettable();

    if( opt.shader.has_value() ){
        const VkPipelineShaderStageCreateInfo& ssci =  opt.shader.value();
        this->shaderEntryPoints.push_back(std::string(ssci.pName));
        this->shaderStageCreateInfo.push_back(VkPipelineShaderStageCreateInfo{
            .sType=VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
            .pNext=nullptr,
            .flags=0,
            .stage=ssci.stage,
            .module=ssci.module,
            .pName=this->shaderEntryPoints.back().c_str(),
            .pSpecializationInfo=nullptr
        });
    }
    if( opt.vertexInputState.has_value() ){
        this->vertexInputState = opt.vertexInputState.value();

        this->vertexBindingDescriptions.resize(this->vertexInputState.vertexBindingDescriptionCount);
        for(unsigned i=0;i<(unsigned)this->vertexBindingDescriptions.size();++i){
            this->vertexBindingDescriptions[i] = opt.vertexInputState.value().pVertexBindingDescriptions[i];
        }
        this->vertexInputState.pVertexBindingDescriptions = this->vertexBindingDescriptions.data();

        this->attributeDescriptions.resize(opt.vertexInputState.value().vertexAttributeDescriptionCount);
        for(unsigned i=0;i<(unsigned)this->vertexBindingDescriptions.size();++i){
            this->attributeDescriptions[i] = opt.vertexInputState.value().pVertexAttributeDescriptions[i];
        }
        this->vertexInputState.pVertexAttributeDescriptions = this->attributeDescriptions.data();
    }

    if( opt.inputAssemblyState.has_value() ){
        this->inputAssemblyState = opt.inputAssemblyState.value();
    }
    if( opt.tessellationState.has_value() ){
        this->hasTessellation=true;
        this->tessellationState = opt.tessellationState.value();
    }
    //~ if( opt.viewportState.has_value() ){
        //~ this->viewportState = opt.viewportState.value();
    //~ }
    if( opt.rasterizationState.has_value() ){
        this->rasterizationState = opt.rasterizationState.value();
    }
    if( opt.multisampleState.has_value() ){
        this->multisampleState = opt.multisampleState.value();
        if( opt.multisampleState.value().pSampleMask ){
            int numMasks = int(opt.multisampleState.value().rasterizationSamples/32);
            if( opt.multisampleState.value().rasterizationSamples % 32 )
                numMasks+=1;

            this->sampleMasks.resize(numMasks);
            for(int i=0;i<numMasks;++i){
                this->sampleMasks[i] = opt.multisampleState.value().pSampleMask[i];
            }
            this->multisampleState.pSampleMask = this->sampleMasks.data();
        }
    }

    if( opt.depthStencilState.has_value() ){
        this->depthStencilState = opt.depthStencilState.value();
    }
    if( opt.colorBlendState.has_value() ){
        //note: some data is set up in finishInit
        this->colorBlendState = opt.colorBlendState.value();
    }
    if( opt.topology.has_value() ){
        this->inputAssemblyState.topology=opt.topology.value();
    }
    if( opt.primitiveRestartEnable.has_value() ){
        this->inputAssemblyState.primitiveRestartEnable = opt.primitiveRestartEnable.value();
    }
    if( opt.viewport.has_value() ){
        this->viewport = opt.viewport.value();
    }
    if( opt.scissor.has_value() ){
        this->scissor = opt.scissor.value();
    }
    if( opt.depthClampEnable.has_value() ){
        this->rasterizationState.depthClampEnable = opt.depthClampEnable.value();
    }
    if( opt.rasterizerDiscardEnable.has_value() ){
        this->rasterizationState.rasterizerDiscardEnable = opt.rasterizerDiscardEnable.value();
    }
    if( opt.polygonMode.has_value() ){
        this->rasterizationState.polygonMode = opt.polygonMode.value();
    }
    if( opt.cullMode.has_value() ){
        this->rasterizationState.cullMode = opt.cullMode.value();
    }
    if( opt.frontFace.has_value() ){
        this->rasterizationState.frontFace = opt.frontFace.value();
    }
    if( opt.depthBiasEnable.has_value() ){
        this->rasterizationState.depthBiasEnable = opt.depthBiasEnable.value();
    }
    if( opt.depthBiasConstantFactor.has_value() ){
        this->rasterizationState.depthBiasConstantFactor = opt.depthBiasConstantFactor.value();
    }
    if( opt.depthBiasClamp.has_value() ){
        this->rasterizationState.depthBiasClamp = opt.depthBiasClamp.value();
    }
    if( opt.depthBiasSlopeFactor.has_value() ){
        this->rasterizationState.depthBiasSlopeFactor = opt.depthBiasSlopeFactor.value();
    }
    if( opt.lineWidth.has_value() ){
        this->rasterizationState.lineWidth = opt.lineWidth.value();
    }
    if( opt.rasterizationSamples.has_value() ){
        this->multisampleState.rasterizationSamples = opt.rasterizationSamples.value();
    }
    if( opt.sampleShadingEnable.has_value() ){
        this->multisampleState.sampleShadingEnable = opt.sampleShadingEnable.value();
    }
    if( opt.minSampleShading.has_value() ){
        this->multisampleState.minSampleShading = opt.minSampleShading.value();
    }
    if( opt.sampleMask.has_value() ){
        this->sampleMasks = opt.sampleMask.value();
    }
    if( opt.alphaToCoverageEnable.has_value() ){
        this->multisampleState.alphaToCoverageEnable = opt.alphaToCoverageEnable.value();
    }
    if( opt.alphaToOneEnable.has_value() ){
        this->multisampleState.alphaToOneEnable = opt.alphaToOneEnable.value();
    }
    if( opt.depthTestEnable.has_value() ){
        this->depthStencilState.depthTestEnable = opt.depthTestEnable.value();
    }
    if( opt.depthWriteEnable.has_value() ){
        this->depthStencilState.depthWriteEnable = opt.depthWriteEnable.value();
    }
    if( opt.depthCompareOp.has_value() ){
        this->depthStencilState.depthCompareOp = opt.depthCompareOp.value();
    }
    if( opt.depthBoundsTestEnable.has_value() ){
        this->depthStencilState.depthBoundsTestEnable = opt.depthBoundsTestEnable.value();
    }
    if( opt.stencilTestEnable.has_value() ){
        this->depthStencilState.stencilTestEnable = opt.stencilTestEnable.value();
    }
    if( opt.stencilFront.has_value() ){
        this->depthStencilState.front = opt.stencilFront.value();
    }
    if( opt.stencilBack.has_value() ){
        this->depthStencilState.back = opt.stencilBack.value();
    }
    if( opt.stencilFrontAndBack.has_value() ){
        this->depthStencilState.front = opt.stencilFrontAndBack.value();
        this->depthStencilState.back = opt.stencilFrontAndBack.value();
    }
    if( opt.minDepthBounds.has_value() ){
        this->depthStencilState.minDepthBounds = opt.minDepthBounds.value();
    }
    if( opt.maxDepthBounds.has_value() ){
        this->depthStencilState.maxDepthBounds = opt.maxDepthBounds.value();
    }
    if( opt.logicOpEnable.has_value() ){
        this->colorBlendState.logicOpEnable = opt.logicOpEnable.value();
    }
    if( opt.logicOp.has_value() ){
        this->colorBlendState.logicOp = opt.logicOp.value();
    }
    if( opt.blendEnable.has_value() ){
        this->oneBlendAttachmentState.blendEnable = opt.blendEnable.value();
    }
    if( opt.srcColorBlendFactor.has_value() ){
        this->oneBlendAttachmentState.srcColorBlendFactor = opt.srcColorBlendFactor.value();
    }
    if( opt.dstColorBlendFactor.has_value() ){
        this->oneBlendAttachmentState.dstColorBlendFactor = opt.dstColorBlendFactor.value();
    }
    if( opt.colorBlendOp.has_value() ){
        this->oneBlendAttachmentState.colorBlendOp = opt.colorBlendOp.value();
    }
    if( opt.srcAlphaBlendFactor.has_value() ){
        this->oneBlendAttachmentState.srcAlphaBlendFactor = opt.srcAlphaBlendFactor.value();
    }
    if( opt.dstAlphaBlendFactor.has_value() ){
        this->oneBlendAttachmentState.dstAlphaBlendFactor = opt.dstAlphaBlendFactor.value();
    }
    if( opt.alphaBlendOp.has_value() ){
        this->oneBlendAttachmentState.alphaBlendOp = opt.alphaBlendOp.value();
    }
    if( opt.colorWriteMask.has_value() ){
        this->oneBlendAttachmentState.colorWriteMask = opt.colorWriteMask.value();
    }
    if( opt.blendConstants.has_value() ){
        auto tmp = opt.blendConstants.value();
        this->colorBlendState.blendConstants[0] = tmp[0] ;
        this->colorBlendState.blendConstants[1] = tmp[1] ;
        this->colorBlendState.blendConstants[2] = tmp[2] ;
        this->colorBlendState.blendConstants[3] = tmp[3] ;
    }
    return this;
}


void GraphicsPipeline::checkSettable()
{
    //flag error if use() has been called before set()
    if( !this->pipelines.empty() )
        fatal("Cannot set options on pipeline after use() has been called");
}

void GraphicsPipeline::finishInit()
{

    assert(this->pipelines.empty() );

    //~ for(Framebuffer* fb : this->framebuffers){
        //~ this->addFramebuffer(fb);
    //~ }
}

void GraphicsPipeline::addFramebuffer(Framebuffer* fb, RenderPass* renderPass)
{


    auto key = std::make_pair(fb,renderPass);
    if( this->pipelines.contains(key) )
        return;

    std::vector<VkPipelineColorBlendAttachmentState> blendAttachmentState(renderPass->numLayers);
    for(int i=0;i<renderPass->numLayers;++i){
        blendAttachmentState[i] = this->oneBlendAttachmentState;
    }

    //we overwrite this one each time through the loop
    this->colorBlendState.pAttachments = blendAttachmentState.data();
    this->colorBlendState.attachmentCount = renderPass->numLayers;


    //clamp width to size of target
    std::vector<VkViewport> vport(renderPass->numLayers);
    std::vector<VkRect2D> scissors(renderPass->numLayers);
    for(int i=0;i<renderPass->numLayers;++i){
        vport[i] = this->viewport;
        vport[i].width  = std::min(vport[i].width,(float)fb->width);
        vport[i].height = std::min(vport[i].height,(float)fb->height);
        scissors[i] = this->scissor;
        scissors[i].extent.width  = (std::min( scissors[i].extent.width, fb->width ) ) ;
        scissors[i].extent.height = (std::min( scissors[i].extent.height,fb->height ) ) ;
    }

    VkPipelineViewportStateCreateInfo vstate {
        .sType=VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO,
        .pNext=nullptr,
        .flags=0,
        .viewportCount = 1, //(unsigned)renderPass->numLayers,
        .pViewports = vport.data(),
        .scissorCount = 1, //(unsigned)renderPass->numLayers,
        .pScissors = scissors.data()
    };

    VkGraphicsPipelineCreateInfo pipeInfo{
        .sType=VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO,
        .pNext=nullptr,
        .flags=0,
        .stageCount=len(shaderStageCreateInfo),
        .pStages = shaderStageCreateInfo.data(),
        .pVertexInputState = &(this->vertexInputState),
        .pInputAssemblyState = &(this->inputAssemblyState),
        .pTessellationState= ( this->hasTessellation ? &(this->tessellationState) : nullptr ),
        .pViewportState = &(vstate),
        .pRasterizationState = &(this->rasterizationState),
        .pMultisampleState = &(this->multisampleState),
        .pDepthStencilState = &(this->depthStencilState),
        .pColorBlendState = &(this->colorBlendState),
        .pDynamicState = nullptr,
        .layout = this->pipelineLayout->pipelineLayout,
        .renderPass = renderPass->renderPass,
        .subpass = 0,
        .basePipelineHandle=0,
        .basePipelineIndex=0
    };

    VkPipeline pipeline;
    check(vkCreateGraphicsPipelines(
        this->ctx->dev,
        VK_NULL_HANDLE,
        1,
        &(pipeInfo),
        nullptr,
        &(pipeline)
    ));

    this->ctx->setObjectName(pipeline, this->name);
    this->pipelines[key] = pipeline;
}
