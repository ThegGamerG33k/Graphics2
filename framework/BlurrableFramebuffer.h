#pragma once
#include "vkhelpers.h"
#include "math2801.h"
#include "RenderPass.h"
#include <memory>
#include <functional>
#include <optional>
#include "Framebuffer.h"



/// A Framebuffer is a render target, either the window or else an offscreen image.
class BlurrableFramebuffer: public Framebuffer{
  public:

    /// Create offscreen Framebuffer
    /// @param w Width
    /// @param h Height
    /// @param layers Number of layers; must be positive
    /// @param VkFormat Format of image
    /// @param name For debugging
    BlurrableFramebuffer(
        VulkanContext* ctx,
        unsigned w, unsigned h, unsigned layers, VkFormat format, std::string name
    );

    /// Destructor to release resources.
    virtual ~BlurrableFramebuffer(){}

    /// Blur this Framebuffer; this cannot be called on a
    /// Framebuffer associated with the window. (It must be
    /// an offscreen image). Note that this function
    /// changes the currently bound pipeline and descriptor sets,
    /// so you must rebind any pipeline or descriptor sets that you need.
    /// @param radius The blur radius. Must be <= 56
    /// @param layer The layer to blur
    /// @param scaleFactor Each pixel channel will be multiplied by this value after blurring
    /// @param cmd The command buffer
    /// @param currentVertexManager This function changes the currently bound vertex manager;
    ///        the parameter provided will be rebound as the vertex manager before blur() returns.
    ///        This parameter may be null.
    void blur(unsigned radius, unsigned layer, float scaleFactor,
              VkCommandBuffer cmd, VertexManager* currentVertexManager);

    /// Blur this Framebuffer; this cannot be called on a
    /// Framebuffer associated with the window. (It must be
    /// an offscreen image). Note that this function
    /// changes the currently bound pipeline and descriptor sets,
    /// so you must rebind any pipeline or descriptor sets that you need.
    /// @param radius The blur radius. Must be <= 56
    /// @param layer The layer to blur
    /// @param scaleFactor Each pixel channel will be multiplied by this value after blurring
    /// @param cmd The command buffer
    /// @param currentVertexManager This function changes the currently bound vertex manager;
    ///        the parameter provided will be rebound as the vertex manager before blur() returns.
    ///        This parameter may be null.
    /// @param doDistanceDependentBlur True to do a distance-dependent blur, suitable for
    ///        depth of field effects.
    void blur(unsigned radius, unsigned layer, float scaleFactor,
              VkCommandBuffer cmd, VertexManager* currentVertexManager,
              bool doDistanceDependentBlur);


  private:

    GraphicsPipeline* blurPipeline=nullptr;
    GraphicsPipeline* blurDDPipelineWriteToHelper=nullptr;
    GraphicsPipeline* blurDDPipelineWriteToFB=nullptr;
    Framebuffer* blurHelper=nullptr;    //copy of pointer from blurHelpers map; not a private FB

    BlurrableFramebuffer(const BlurrableFramebuffer&) = delete;
    void operator=(const BlurrableFramebuffer&) = delete;
};
