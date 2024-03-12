#pragma once

#include <optional>
#include "Pipeline.h"
#include <list>


struct PipelineOption{
    std::optional< VkPipelineShaderStageCreateInfo >        shader                  = std::optional< VkPipelineShaderStageCreateInfo >() ;
    std::optional< VkPipelineVertexInputStateCreateInfo >   vertexInputState        = std::optional< VkPipelineVertexInputStateCreateInfo >() ;
    std::optional< VkPipelineInputAssemblyStateCreateInfo > inputAssemblyState      = std::optional<VkPipelineInputAssemblyStateCreateInfo >() ;
    std::optional< VkPipelineTessellationStateCreateInfo >  tessellationState       = std::optional< VkPipelineTessellationStateCreateInfo >() ;
    std::optional< VkPipelineViewportStateCreateInfo >      viewportState           = std::optional< VkPipelineViewportStateCreateInfo >() ;
    std::optional< VkPipelineRasterizationStateCreateInfo > rasterizationState      = std::optional< VkPipelineRasterizationStateCreateInfo >() ;
    std::optional< VkPipelineMultisampleStateCreateInfo >   multisampleState        = std::optional< VkPipelineMultisampleStateCreateInfo >() ;
    std::optional< VkPipelineDepthStencilStateCreateInfo >  depthStencilState       = std::optional< VkPipelineDepthStencilStateCreateInfo >() ;
    std::optional< VkPipelineColorBlendStateCreateInfo >    colorBlendState         = std::optional< VkPipelineColorBlendStateCreateInfo >() ;
    std::optional< VkPrimitiveTopology>                     topology                = std::optional< VkPrimitiveTopology>() ;
    std::optional< bool >                                   primitiveRestartEnable  = std::optional< bool >() ;
    std::optional< VkViewport >                             viewport                = std::optional< VkViewport >() ;
    std::optional< VkRect2D >                               scissor                 = std::optional< VkRect2D >() ;
    std::optional< bool >                                   depthClampEnable        = std::optional< bool >() ;
    std::optional< bool >                                   rasterizerDiscardEnable = std::optional< bool >() ;
    std::optional< VkPolygonMode >                          polygonMode             = std::optional< VkPolygonMode >() ;
    std::optional< VkCullModeFlags >                        cullMode                = std::optional< VkCullModeFlags >() ;
    std::optional< VkFrontFace >                            frontFace               = std::optional< VkFrontFace >() ;
    std::optional< bool >                                   depthBiasEnable         = std::optional< bool >() ;
    std::optional< float>                                   depthBiasConstantFactor = std::optional< float>() ;
    std::optional< float>                                   depthBiasClamp          = std::optional< float>() ;
    std::optional< float>                                   depthBiasSlopeFactor    = std::optional< float>() ;
    std::optional< float >                                  lineWidth               = std::optional< float >() ;
    std::optional< VkSampleCountFlagBits >                  rasterizationSamples    = std::optional< VkSampleCountFlagBits >() ;
    std::optional< bool >                                   sampleShadingEnable     = std::optional< bool >() ;
    std::optional< float >                                  minSampleShading        = std::optional< float >() ;
    std::optional< std::vector<VkSampleMask> >              sampleMask              = std::optional< std::vector<VkSampleMask> >() ;
    std::optional< bool >                                   alphaToCoverageEnable   = std::optional< bool >() ;
    std::optional< bool >                                   alphaToOneEnable        = std::optional< bool >() ;
    std::optional< bool >                                   depthTestEnable         = std::optional<bool>() ;
    std::optional< bool >                                   depthWriteEnable        = std::optional<bool>() ;
    std::optional< VkCompareOp >                            depthCompareOp          = std::optional<VkCompareOp>() ;
    std::optional< bool >                                   depthBoundsTestEnable   = std::optional<bool>() ;
    std::optional< bool >                                   stencilTestEnable       = std::optional<bool>() ;
    std::optional< VkStencilOpState >                       stencilFront            = std::optional<VkStencilOpState>() ;
    std::optional< VkStencilOpState >                       stencilBack             = std::optional<VkStencilOpState>() ;
    std::optional< VkStencilOpState >                       stencilFrontAndBack     = std::optional<VkStencilOpState>() ;
    std::optional< float >                                  minDepthBounds          = std::optional<float>() ;
    std::optional< float >                                  maxDepthBounds          = std::optional<float>() ;
    std::optional< bool >                                   logicOpEnable           = std::optional<bool>() ;
    std::optional< VkLogicOp >                              logicOp                 = std::optional< VkLogicOp >() ;
    std::optional< bool>                                    blendEnable             = std::optional< bool>() ;
    std::optional< VkBlendFactor >                          srcColorBlendFactor     = std::optional< VkBlendFactor >() ;
    std::optional< VkBlendFactor >                          dstColorBlendFactor     = std::optional< VkBlendFactor >() ;
    std::optional< VkBlendOp >                              colorBlendOp            = std::optional< VkBlendOp >() ;
    std::optional< VkBlendFactor >                          srcAlphaBlendFactor     = std::optional< VkBlendFactor >() ;
    std::optional< VkBlendFactor >                          dstAlphaBlendFactor     = std::optional< VkBlendFactor >() ;
    std::optional< VkBlendOp >                              alphaBlendOp            = std::optional< VkBlendOp >() ;
    std::optional< VkColorComponentFlags >                  colorWriteMask          = std::optional< VkColorComponentFlags >() ;
    std::optional< std::array<float,4> >                    blendConstants          = std::optional< std::array<float,4> >();
};

class GraphicsPipeline: public Pipeline{
  public:
    template<typename ...T>
    GraphicsPipeline(
            VulkanContext* ctx_, std::string name_,
            PipelineLayout* pipelineLayout_,
            //const std::vector<Framebuffer*>& framebuffers_,
            T... args) : Pipeline(ctx_,pipelineLayout_, VK_PIPELINE_BIND_POINT_GRAPHICS,name_)
    {
        this->_setDefaults();
        //this->framebuffers = framebuffers_;
        init(args...);
    }
    GraphicsPipeline* set(const PipelineOption& opt);


    /// Allow another framebuffer to be used with this pipeline
    void addFramebuffer(Framebuffer* framebuffer, RenderPass* renderPass) override;

  private:
        template<typename ...T>
        void init(const PipelineOption& opt, T... args){
            this->set(opt);
            init(args...);
        }
        void init(const PipelineOption& opt){
            this->set(opt);
        }
        void _setDefaults();
        void finishInit() override;
        void checkSettable();

        std::vector<Framebuffer*> framebuffers;

        //these two go together
        std::vector<VkPipelineShaderStageCreateInfo> shaderStageCreateInfo;

        VkPipelineVertexInputStateCreateInfo vertexInputState;
        VkPipelineInputAssemblyStateCreateInfo inputAssemblyState;
        VkViewport viewport;
        VkRect2D scissor;
        VkPipelineRasterizationStateCreateInfo rasterizationState;
        VkPipelineMultisampleStateCreateInfo multisampleState;
        VkPipelineDepthStencilStateCreateInfo depthStencilState;
        VkPipelineColorBlendStateCreateInfo colorBlendState;
        bool hasTessellation=false;
        VkPipelineTessellationStateCreateInfo tessellationState;
        //~ VkPipelineViewportStateCreateInfo viewportState ;
        VkPipelineColorBlendAttachmentState oneBlendAttachmentState;

        //shadow data to prevent dangling pointers


        std::vector<VkVertexInputBindingDescription> vertexBindingDescriptions;

        //must be list so existing items don't move in memory
        std::list<std::string> shaderEntryPoints;

        std::vector<VkVertexInputAttributeDescription> attributeDescriptions;
        std::vector<VkSampleMask> sampleMasks;


};
