#include "Pipeline.h"
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

Pipeline* current_ = nullptr;
static bool initialized=false;


static void frameBeginCallback(int /*imageIndex*/ )
{
    current_ = nullptr;
}

static void frameEndCallback(int /*imageIndex*/ )
{
    current_ = nullptr;
}

static void initialize(VulkanContext*){
    if(!initialized){
        utils::registerFrameBeginCallback( frameBeginCallback );
        utils::registerFrameEndCallback( frameEndCallback );
        initialized=true;
    }
}

Pipeline::Pipeline(VulkanContext* ctx_, PipelineLayout* pipelineLayout_,
        VkPipelineBindPoint bindPoint_, std::string name_)
{
    initialize(ctx);
    this->ctx=ctx_;
    this->pipelineLayout=pipelineLayout_;
    this->bindPoint=bindPoint_;
    this->name=name_;
    CleanupManager::registerCleanupFunction( [this](){
        for(auto& it : this->pipelines){
            VkPipeline p = it.second;
            vkDestroyPipeline(this->ctx->dev, p, nullptr);
        }
    });
}

void Pipeline::use(VkCommandBuffer cmd)
{
    if(this->pipelines.empty() ){
        this->finishInit();
    }

    VkPipeline pipeline;

    //~ auto none = std::make_pair((Framebuffer*)nullptr, (RenderPass*)nullptr);
    //~ if( this->pipelines.contains(none)){
        //~ //this pipeline can be used with anything
        //~ //(i.e., it's a compute pipeline)
        //~ pipeline = this->pipelines[none];
    //~ } else {
        Framebuffer* fb = Framebuffer::current;
        RenderPass* rp = Framebuffer::renderPass;
        auto key = std::make_pair(fb,rp);
        if(!this->pipelines.contains(key)){
            this->addFramebuffer(fb,rp);
            assert( this->pipelines.contains(key) );
            //~ fatal("This pipeline ("+this->name+") was not declared as being usable with the active render target ("+fb->name+")");
        }
        pipeline = this->pipelines[key];
    //~ }
    vkCmdBindPipeline(cmd,this->bindPoint,pipeline);
    current_ = this;
}

//FIXME: This assumes we only build one command list at a time
Pipeline* Pipeline::current(){
    if(!current_)
        fatal("There is no active pipeline");
    return current_;
}
