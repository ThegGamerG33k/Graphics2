#pragma once
#include "vkhelpers.h"
#include <array>
#include "PipelineLayout.h"

class DescriptorSetLayout;
class PushConstants;
class Framebuffer;
class RenderPass;

/// Base class for all pipeline objects (see GraphicsPipeline
/// and ComputePipeline). This is an abstract class
/// and is not instantiated directly.
class Pipeline{
  public:

    /// Layout of the pipeline
    PipelineLayout* pipelineLayout;

    /// Name, for debugging
    std::string name;

    /// Constructor
    /// @param ctx Associated context
    /// @param pipelineLayout The pipeline's layout
    /// @param bindPoint Type of pipeline: VK_PIPELINE_BIND_POINT_GRAPHICS
    ///     or VK_PIPELINE_BIND_POINT_COMPUTE
    /// @param name For debugging
    Pipeline(VulkanContext* ctx, PipelineLayout* pipelineLayout,
        VkPipelineBindPoint bindPoint, std::string name);

    /// Activate this pipeline.
    /// @param cmd The command buffer
    void use(VkCommandBuffer cmd);

    /// Returns the most recently used pipeline (see use()).
    /// If no pipeline has been used for the current frame,
    /// an exception is thrown.
    /// @return The pipeline
    static Pipeline* current();

    /// Allow the given framebuffer and renderpass to be used with this pipeline.
    virtual void addFramebuffer(Framebuffer* framebuffer, RenderPass* renderPass) = 0;

  protected:

    virtual void finishInit()=0;

    VulkanContext* ctx;
    //~ VkPipeline pipeline = VK_NULL_HANDLE;
    VkPipelineBindPoint bindPoint;

    //if (null,null) is the only key in here, this pipeline can
    //be used as either graphics or compute
    std::map< std::pair<Framebuffer*,RenderPass*> , VkPipeline > pipelines;


    Pipeline(const Pipeline&) = delete;
    void operator=(const Pipeline&) = delete;

};
