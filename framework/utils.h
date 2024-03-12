#pragma once

#include "vkhelpers.h"
#include <functional>
#include "DescriptorSetEntry.h"

//low level utilities that don't fit elsewhere

class ComputePipeline;
class DescriptorSet;
class PushConstants;

class utils{
  public:

    /// Call this at the start of the draw function. It calls any
    /// pending frame complete callbacks that have been registered
    /// (see registerFrameCompleteCallback()).
    /// @param ctx The context
    /// @return A command buffer that can be used for rendering
    static void beginFrame(VulkanContext* ctx);

    /// Call this at the end of the draw function
    /// @param ctx The context
    static void endFrame(VulkanContext* ctx, VkCommandBuffer cmd);

    static void beginCompute(VulkanContext* ctx);
    static void endCompute(VulkanContext* ctx, VkCommandBuffer cmd);

    /// Get the current swapchain image index
    /// @return The image index
    static int getSwapchainImageIndex();

    /// Compute padding for uniforms or push constants
    /// @param offset Current offset within file
    /// @param alignment Desired alignment of next item to add
    /// @return Number of bytes of padding that need to be added to
    ///        offset to make it a multiple of alignment.
    static int computePadding( VkDeviceSize offset, VkDeviceSize alignment );

    /// Read the contents of a file. Throws an exception if the
    /// file cannot be read.
    /// @param filename The file to read
    /// @return The file's data
    static std::vector<char> readFile(std::string filename);

    /// Register a callback to be notified when a particular
    /// frame has completely finished rendering.
    /// The check for frame completeness is performed at the very start of beginFrame()
    /// so that we have the best chance of finding any previous frames that
    /// have progressed entirely thorough the GPU pipeline.
    /// The callback is passed the identifier of the frame that
    /// has been completed (see getCurrentFrameIdentifier()).
    /// @param f The callback
    static void registerFrameCompleteCallback( std::function<void(unsigned)> f);

    /// Get a unique identifier for the current frame. This eventually
    /// wraps after approximately 4.2 billion frames have been rendered.
    /// This identifier is used in the registerFrameCallback() callback.
    /// This function may only be called between beginFrame() and endFrame().
    /// @return The frame identifier.
    static unsigned getCurrentFrameIdentifier();

    static int getRenderedSwapchainImageIndex();

    /// Register a callback to be notified at the start of frame rendering.
    /// It is passed the swapchain image index and the command buffer.
    /// @param The callback
    static void registerFrameBeginCallback(  std::function<void(int)> f);

    /// Register a callback to be notified at the end of frame rendering.
    /// This is called just before the command buffer is submitted to the queue.
    /// It is passed the swapchain image index and the command buffer.
    /// @param The callback
    static void registerFrameEndCallback(  std::function<void(int)> f);

    static void addCommandBufferPreSubmit(VkCommandBuffer cmd);



    static std::tuple<DescriptorSet*,ComputePipeline*> initializeComputePipeline(
        VulkanContext* ctx,
        std::string computeShaderFilename,
        std::vector<DescriptorSetEntry> descriptorSetEntries
    );

    static std::tuple<DescriptorSet*,ComputePipeline*,PushConstants*> initializeComputePipeline(
        VulkanContext* ctx,
        std::string computeShaderFilename,
        std::string pushConstantsFilename,
        std::vector<DescriptorSetEntry> descriptorSetEntries
    );

  private:

    static std::vector<VkCommandBuffer> additionalCommandBuffers;
    static void submitIt(VulkanContext* ctx, VkCommandBuffer cmd, bool isRendering);
    static void doBegin(VulkanContext* ctx, bool isRendering);

};
