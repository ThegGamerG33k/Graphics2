#pragma once

#include <array>
#include "vkhelpers.h"

class DescriptorSetLayout;
class PushConstants;

/// Describes layout of a pipeline
class PipelineLayout{
  public:

    /// Constructor
    /// @param ctx The associated context
    /// @param pushConstantSize Size of push constants that will be used with this pipeline
    /// @param descriptorSetLayouts Layout of the first three descriptor sets
    ///         that will be used with the pipeline. If a set will not
    ///         be used, null may be passed in for that item.
    /// @param name Name, for debuggin
    PipelineLayout( VulkanContext* ctx,
                    unsigned pushConstantSize,
                    DescriptorSetLayout* descriptorSetLayout,
                    std::string name);

    /// The descriptor sets used for this layout
    DescriptorSetLayout* descriptorSetLayout;

    /// The push constants used for the pipeline
    unsigned pushConstantSize;

    /// The pipline layout itself
    VkPipelineLayout pipelineLayout;

    /// Name, for debugging
    std::string name;
};
