#include "utils.h"
#include <tuple>
#include "CommandBuffer.h"
#include "GraphicsPipeline.h"
#include <chrono>
#include <fstream>
#include <iostream>
#include "CleanupManager.h"
#include "ShaderManager.h"
#include "Descriptors.h"
#include "PushConstants.h"
#include "ComputePipeline.h"

#if defined _MSC_VER
    #include <intrin.h>
#else
    #include <signal.h>
#endif

static std::vector<std::function<void(unsigned)> > _frameCompleteCallbacks;
static std::vector<std::function<void(int)> > frameBeginCallbacks;
static std::vector<std::function<void(int)> > _frameEndCallbacks;
std::vector<VkCommandBuffer> utils::additionalCommandBuffers;
static int _mostRecentlyRenderedSwapchainIndex=-1;

//eventually, this wraps, but that's not a problem since we shouldn't have
//4 billion frames outstanding anyway
//This gets incremented at the end of endFrame in preparation for the next frame.
//This also means that anything set up before rendering begins
//is associated with frame 0
static unsigned _currentFrameIdentifier=0;

static std::vector<VkFence> _availableFences;

struct ActiveFence{
    unsigned frameNumber;
    VkFence fence;
};

//first=frame number, second=fence
static std::vector< ActiveFence > _activeFences;

static bool _inFrame=false;
static bool _inCompute=false;

static int _currentSwapchainIndex=-1;
//~ static VkCommandBuffer _currentCommandBuffer=VK_NULL_HANDLE;



void utils::doBegin(VulkanContext* ctx, bool isRendering)
{
    //deal with any signaled fences from past frames
    for( auto it = _activeFences.begin(); it != _activeFences.end() ; ){
        auto fstat = vkGetFenceStatus(ctx->dev, it->fence);
        if( fstat == VK_SUCCESS ){
            //this frame is done; notify components that they can
            //reclaim resources
            _availableFences.push_back(it->fence);
            vkResetFences(ctx->dev,1,&it->fence);
            for(auto& f : _frameCompleteCallbacks ){
                f(it->frameNumber);       //tell them the frame number
            }
            it = _activeFences.erase(it);
        } else if( fstat == VK_NOT_READY ){
            //frame not yet done; look at next fence
            it++;
        } else if( fstat == VK_ERROR_DEVICE_LOST ){
            fatal("Device was lost");
        }
    }

    if(isRendering){
        std::uint32_t imageindex;
        vkAcquireNextImageKHR(
            ctx->dev,
            ctx->swapchain,
            0xffffffffffffffff,
            ctx->imageAcquiredSemaphore,
            nullptr,
            &(imageindex)
        );

        _currentSwapchainIndex = (int)imageindex;
    } else {
        _currentSwapchainIndex=0;   //dummy
    }

    for(auto& f : frameBeginCallbacks ){
        f(_currentSwapchainIndex);
    }

    return;
}

void utils::beginCompute(VulkanContext* ctx)
{
    if(_inCompute ){
        fatal("beginCompute() called twice with no intervening endCompute()");
    }
    if(_inFrame){
        fatal("beginCompute() called while still rendering a frame");
    }
    _inCompute=true;

    doBegin(ctx,false);
}

void utils::beginFrame(VulkanContext* ctx)
{
    if(_inFrame ){
        fatal("beginFrame() called twice with no intervening endFrame() or endCompute()");
    }
    if(_inCompute){
        fatal("beginFrame() called while still in a compute operation");
    }
    _inFrame=true;
    doBegin(ctx,true);
}



void utils::submitIt(VulkanContext* ctx, VkCommandBuffer cmd, bool isRendering)
{
    //if isRendering is false: Compute shader only
    for(auto& f : _frameEndCallbacks ){
        f(_currentSwapchainIndex);
    }

    if( _availableFences.empty() ){
        VkFenceCreateInfo ci{
            .sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO,
            .pNext = nullptr,
            .flags = 0          //create as unsignaled
        };
        VkFence f;
        check(vkCreateFence(ctx->dev, &ci, nullptr, &f ));
        _activeFences.push_back( ActiveFence{ .frameNumber=_currentFrameIdentifier, .fence=f } ) ;
        CleanupManager::registerCleanupFunction( [f,ctx](){
            vkDestroyFence(ctx->dev,f,nullptr);
        });
    } else {
        VkFence f = _availableFences.back();
        _availableFences.pop_back();
        _activeFences.push_back( ActiveFence{ .frameNumber=_currentFrameIdentifier, .fence=f } );
    }
    VkPipelineStageFlags waitDestStageMask = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;

    std::vector<VkCommandBuffer> C(1+utils::additionalCommandBuffers.size());
    for(unsigned i=0;i<(unsigned)utils::additionalCommandBuffers.size();++i){
        C[i]=utils::additionalCommandBuffers[i];
    }
    C[C.size()-1] = cmd;

    check(vkQueueSubmit(
        ctx->graphicsQueue,
        1,
        VkSubmitInfo{
            .sType=VK_STRUCTURE_TYPE_SUBMIT_INFO,
            .pNext=nullptr,
            .waitSemaphoreCount = (unsigned)(isRendering ? 1:0 ),
            .pWaitSemaphores= (isRendering ? &ctx->imageAcquiredSemaphore : nullptr),
            .pWaitDstStageMask = &waitDestStageMask,
            .commandBufferCount=(unsigned)(C.size()),
            .pCommandBuffers = C.data(),
            .signalSemaphoreCount= (unsigned)(isRendering ? 1:0 ),
            .pSignalSemaphores=(isRendering ? &ctx->renderCompleteSemaphore : nullptr)
        },
        _activeFences.back().fence         //always the last one in the list
    ));

    if( isRendering ){
        std::uint32_t ii = (std::uint32_t)_currentSwapchainIndex;
        check(vkQueuePresentKHR(
            ctx->presentQueue,
            VkPresentInfoKHR{
                .sType=VK_STRUCTURE_TYPE_PRESENT_INFO_KHR,
                .pNext=nullptr,
                .waitSemaphoreCount=1,
                .pWaitSemaphores = &ctx->renderCompleteSemaphore,
                .swapchainCount=1,
                .pSwapchains = &ctx->swapchain,
                .pImageIndices=&ii,
                .pResults=nullptr
            }
        ));
    }

    //FIXME: Remove this wait and then dispose of command buffer
    //when frame is complete. Also make several renderCompleteSemaphore's above.
    check(vkQueueWaitIdle(ctx->presentQueue));

    for(auto& c : utils::additionalCommandBuffers ){
        CommandBuffer::dispose(c);
    }
    utils::additionalCommandBuffers.clear();

    CommandBuffer::dispose(cmd);

    ++_currentFrameIdentifier;
    _mostRecentlyRenderedSwapchainIndex=_currentSwapchainIndex;
    _currentSwapchainIndex=-1;

}
void utils::endCompute(VulkanContext* ctx, VkCommandBuffer cmd)
{
    if(!_inCompute ){
        fatal("endCompute called without matching beginCompute");
    }
    submitIt(ctx,cmd,false);
    _inCompute=false;
}

void utils::endFrame(VulkanContext* ctx, VkCommandBuffer cmd)
{
    if(!_inFrame){
        fatal("endFrame called without matching beginFrame");
    }

    submitIt(ctx,cmd,true);
    _inFrame=false;

}



int utils::computePadding(VkDeviceSize offset, VkDeviceSize alignment )
{
    if( alignment == 0 )
        return 0;
    VkDeviceSize extra = offset  % alignment;
    if(extra == 0)
        return 0;
    VkDeviceSize padding = alignment-extra;
    return (int)padding;
}

int utils::getSwapchainImageIndex()
{
    /**
    Get the current swapchain image index
    */
    if(! _inFrame)
        fatal("Cannot call getSwapchainImageIndex() outside of a render operation");
    return _currentSwapchainIndex;
}


int utils::getRenderedSwapchainImageIndex()
{
    return _mostRecentlyRenderedSwapchainIndex;
}



std::vector<char> utils::readFile(std::string filename)
{
    std::ifstream in(filename,std::ios::binary);
    if(!in.good()){
        fatal("Cannot read file "+filename);
    }
    in.seekg(0,std::ios::end);
    auto size = in.tellg();
    in.seekg(0);
    std::vector<char> d;
    d.resize(size);
    in.read(d.data(),size);
    return d;
}

void utils::registerFrameCompleteCallback( std::function<void(unsigned)> f){
    _frameCompleteCallbacks.push_back(f);
}


void utils::registerFrameBeginCallback( std::function<void(int)> f){
    frameBeginCallbacks.push_back(f);
}


void utils::registerFrameEndCallback( std::function<void(int)> f){
    _frameEndCallbacks.push_back(f);
}

unsigned utils::getCurrentFrameIdentifier()
{
    if(!_inFrame && !_inCompute)
        fatal("Cannot call getCurrentFrameIdentifier() outside of a render operation");
    return _currentFrameIdentifier;
}

void utils::addCommandBufferPreSubmit(VkCommandBuffer cmd)
{
    /**
    """Add a command buffer that will be
        submitted to the queue prior to the
        one passed as a parameter to endFrame().
        This is useful to push buffer data to the GPU
        before the main rendering occurs.
        No memory barriers are added to ensure visibility
        or availability.
    """
    */
    utils::additionalCommandBuffers.push_back(cmd);
}


std::tuple<DescriptorSet*,ComputePipeline*> utils::initializeComputePipeline(
    VulkanContext* ctx,
    std::string computeShaderFilename,
    std::vector<DescriptorSetEntry> descriptorSetEntries
){
    auto tmp = initializeComputePipeline(ctx,computeShaderFilename,"",descriptorSetEntries);
    return std::make_tuple(std::get<0>(tmp), std::get<1>(tmp) );
}

std::tuple<DescriptorSet*,ComputePipeline*,PushConstants*> utils::initializeComputePipeline(
    VulkanContext* ctx,
    std::string computeShaderFilename,
    std::string pushConstantsFilename,
    std::vector<DescriptorSetEntry> descriptorSetEntries
){

    CommandBuffer::initialize(ctx);
    ShaderManager::initialize(ctx);

    DescriptorSetLayout* descriptorSetLayout = new DescriptorSetLayout(
        ctx, descriptorSetEntries
    );

    PushConstants* pushConstants = nullptr;
    if(!pushConstantsFilename.empty()){
        pushConstants = new PushConstants(pushConstantsFilename);
    }

    PipelineLayout* pipelineLayout = new PipelineLayout(
        ctx,
        (pushConstants ? pushConstants->byteSize : 0),
        descriptorSetLayout,    //descriptors
        "pipeline"
    );

    DescriptorSetFactory* descriptorSetFactory = new DescriptorSetFactory(
        ctx, "factory",
        0,      //binding point
        pipelineLayout
    );

    DescriptorSet* descriptorSet = descriptorSetFactory->make();

    auto computeShader = ShaderManager::load(computeShaderFilename);

    ComputePipeline* pipeline = new ComputePipeline(
        ctx,
        pipelineLayout,
        computeShader,
        "pipeline"
    );

    CleanupManager::registerCleanupFunction(
        [   descriptorSetLayout,pushConstants,pipelineLayout,descriptorSetFactory,
            pipeline
        ](){
            delete descriptorSetLayout;
            delete pushConstants;
            delete pipelineLayout;
            delete descriptorSetFactory;
            delete pipeline;
            //descriptorSet is deleted by its factory
        }
    );


    return std::make_tuple(descriptorSet,pipeline,pushConstants);
}
