#include "Framebuffer.h"
#include "ImageManager.h"
#include "GraphicsPipeline.h"
#include "Descriptors.h"
#include "VertexManager.h"
#include "CleanupManager.h"
#include "ShaderManager.h"
#include "PushConstants.h"
#include "Samplers.h"
#include "utils.h"
#include <cmath>
#include <assert.h>
#include <algorithm>
#include "mischelpers.h"
#include "Samplers.h"

using namespace math2801;

int Framebuffer::_currentSwapchainIndex_=-1;
bool Framebuffer::_registeredCallbacks=false;
std::vector<Framebuffer*> Framebuffer::_allFramebuffers;
Framebuffer* Framebuffer::current = nullptr;
RenderPass* Framebuffer::renderPass = nullptr;

void Framebuffer::_frameBegin(int imageIndex)
{
    Framebuffer::_currentSwapchainIndex_ = imageIndex;
}

void Framebuffer::_frameEnd( int /*imageIndex*/)
{
    Framebuffer::_currentSwapchainIndex_ = -1;
}

int Framebuffer::_currentSwapchainIndex()
{
    if( Framebuffer::_currentSwapchainIndex_ == -1 )
        fatal("Cannot use a framebuffer outside a frame draw operation");
    return Framebuffer::_currentSwapchainIndex_;
}

Framebuffer::Framebuffer(VulkanContext* ctx_) : Framebuffer(ctx_,-1,-1,-1,VK_FORMAT_UNDEFINED,"default Framebuffer")
{}

Framebuffer::Framebuffer(
    VulkanContext* ctx_,
    unsigned width_, unsigned height_, unsigned layers_,
    VkFormat format_, std::string name_)
{
    this->ctx=ctx_;

    if(! Framebuffer::_registeredCallbacks){
        utils::registerFrameBeginCallback( Framebuffer::_frameBegin );
        utils::registerFrameEndCallback( Framebuffer::_frameEnd );
        Framebuffer::_registeredCallbacks=true;
    }

    this->insideRenderpass=false;
    this->completedRenderIndex=-1 ;
    this->currentRenderIndex=-1 ;
    if( width_== (unsigned)-1 )
        this->isDefaultFB = true;
    else
        this->isDefaultFB = false;

    this->pushedToGPU=false;

    if(this->isDefaultFB){
        width_=ctx->width;
        height_=ctx->height;
        layers_=1;
        format_=ctx->surfaceFormat.format;
        name_="DefaultFramebuffer";
    }

    assert(ctx_);
    this->width=width_;
    this->height=height_;
    this->numLayers=layers_;
    this->format=format_;
    this->name=name_;
    Framebuffer::_allFramebuffers.push_back(this);

    VkImageLayout finalLayoutC=VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;     //dummy, gets changed later
    VkImageLayout finalLayoutD=VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;    //dummy, gets changed later

    for(int i=0;i<ctx->numSwapchainImages;++i){
        Image* cb;
        Image* db;

        if(this->isDefaultFB){
            cb = ImageManager::createImageFromVkImage(
                this->width, this->height, 1,
                ctx->surfaceFormat.format,
                0,  //usage; ignored
                VK_IMAGE_VIEW_TYPE_2D_ARRAY,
                VK_IMAGE_LAYOUT_PRESENT_SRC_KHR,
                VK_IMAGE_ASPECT_COLOR_BIT,
                ctx->swapchainImages[i],1,
                "image:colorbuffer"+str(i)
            );
            db = ImageManager::createImageFromVkImage(
                this->width, this->height, 1,
                ctx->depthFormat,
                0,  //usage; ignored
                VK_IMAGE_VIEW_TYPE_2D_ARRAY,
                VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL,
                VK_IMAGE_ASPECT_DEPTH_BIT|VK_IMAGE_ASPECT_STENCIL_BIT,
                ctx->depthbufferImages[i],1,
                "image:depthbuffer"+str(i)
            );

            finalLayoutC = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
            finalLayoutD = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
        } else {
            cb = ImageManager::createImage(
                width,height,layers_,
                format,
                VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT|VK_IMAGE_USAGE_TRANSFER_SRC_BIT|VK_IMAGE_USAGE_TRANSFER_DST_BIT|VK_IMAGE_USAGE_SAMPLED_BIT,
                VK_IMAGE_VIEW_TYPE_2D_ARRAY,
                VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,   //final layout
                VK_IMAGE_ASPECT_COLOR_BIT,
                std::vector<std::span<const char> >(),
                this->name+"["+str(i)+"].color"
            );
            db = ImageManager::createImage(
                width,height,1,
                ctx->depthFormat,
                VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT|VK_IMAGE_USAGE_TRANSFER_SRC_BIT|VK_IMAGE_USAGE_TRANSFER_DST_BIT|VK_IMAGE_USAGE_SAMPLED_BIT,
                VK_IMAGE_VIEW_TYPE_2D_ARRAY,
                VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,  //final layout VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL,
                VK_IMAGE_ASPECT_DEPTH_BIT | VK_IMAGE_ASPECT_STENCIL_BIT,
                std::vector<std::span<const char> >(),
                this->name+"["+str(i)+"].depth"
            );

            finalLayoutC = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
            finalLayoutD = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
        }

        this->colorBuffers.push_back(cb);
        this->depthBuffers.push_back(db);
    }

    unsigned clampedLayers = this->numLayers;
    if(this->numLayers > 8){
        //FIXME: Should query dynamically since
        //some mobiles have limit of 4
        warn("Too many output layers; clamped to 8");
        clampedLayers = 8;
    }

    this->allLayersRenderPassDiscard = new RenderPass(
        ctx,
        format,
        clampedLayers,
        ctx->depthFormat,
        VK_IMAGE_LAYOUT_UNDEFINED,
        VK_IMAGE_LAYOUT_UNDEFINED,
        finalLayoutC,finalLayoutD,
        VK_ATTACHMENT_LOAD_OP_DONT_CARE,
        "all layers renderpass for "+this->name
    );
    this->allLayersRenderPassKeep = new RenderPass(
        ctx,
        format,
        clampedLayers,
        ctx->depthFormat,
        VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
        VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL,
        finalLayoutC,finalLayoutD,
        VK_ATTACHMENT_LOAD_OP_LOAD,
        "all layers renderpass for "+this->name
    );
    this->allLayersRenderPassClear = new RenderPass(
        ctx,
        format,
        clampedLayers,
        ctx->depthFormat,
        VK_IMAGE_LAYOUT_UNDEFINED,VK_IMAGE_LAYOUT_UNDEFINED,
        finalLayoutC,finalLayoutD,
        VK_ATTACHMENT_LOAD_OP_CLEAR,
        "all layers renderpass for "+this->name
    );

    this->singleLayerRenderPassDiscard = new RenderPass(
        ctx,
        format,
        1,
        ctx->depthFormat,
        VK_IMAGE_LAYOUT_UNDEFINED,VK_IMAGE_LAYOUT_UNDEFINED,
        finalLayoutC,finalLayoutD,
        VK_ATTACHMENT_LOAD_OP_DONT_CARE,
        "single layer renderpass for "+this->name
    );
    this->singleLayerRenderPassKeep = new RenderPass(
        ctx,
        format,
        1,
        ctx->depthFormat,
        VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL,
        finalLayoutC,finalLayoutD,
        VK_ATTACHMENT_LOAD_OP_LOAD,
        "single layer renderpass for "+this->name
    );
    this->singleLayerRenderPassClear = new RenderPass(
        ctx,
        format,
        1,
        ctx->depthFormat,
        VK_IMAGE_LAYOUT_UNDEFINED,VK_IMAGE_LAYOUT_UNDEFINED,
        finalLayoutC,finalLayoutD,
        VK_ATTACHMENT_LOAD_OP_CLEAR,
        "single layer renderpass for "+this->name
    );


    CleanupManager::registerCleanupFunction( [this](){this->_cleanup();} );

    ImageManager::addCallback( [this](){ this->pushToGPU(); } );
}


std::vector<RenderPass*> Framebuffer::getAllRenderPasses(){
    return std::vector<RenderPass*>({
        this->allLayersRenderPassDiscard,
        this->allLayersRenderPassKeep,
        this->allLayersRenderPassClear,
        this->singleLayerRenderPassDiscard,
        this->singleLayerRenderPassKeep,
        this->singleLayerRenderPassClear
    });
}

void Framebuffer::pushToGPU()
{
    if( this->pushedToGPU )
        return;

    for(int chainImageIndex=0;chainImageIndex<ctx->numSwapchainImages;++chainImageIndex){
        Image* img = this->colorBuffers[chainImageIndex];
        std::vector<VkImageView> views;

        if(this->isDefaultFB){
            views.push_back( ctx->swapchainImageViews[chainImageIndex] );   //FIXME: should we create view for our own Image object instead?
        } else {
            for(unsigned j=0;j<img->numLayers;++j){
                if(j >= 8){
                    //FIXME: determine this dynamically
                    break;
                }
                views.push_back(  img->layers[j].view() );
            }
        }

        if(this->isDefaultFB){
            views.push_back( ctx->depthbufferViews[chainImageIndex] );  //FIXME: Should we use our own Image's view?
        } else {
            views.push_back(this->depthBuffers[chainImageIndex]->layers[0].view());
        }

        VkFramebuffer fb;
        check(vkCreateFramebuffer(
            ctx->dev,
            VkFramebufferCreateInfo{
                .sType=VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO,
                .pNext=nullptr,
                .flags=0,
                //VK spec 8.2: Load and store op's don't affect
                //renderpass compatibility
                .renderPass = this->allLayersRenderPassDiscard->renderPass,
                .attachmentCount = (unsigned)views.size(),
                .pAttachments = views.data(),
                .width = this->width,
                .height = this->height,
                .layers = 1
            },
            nullptr,
            &(fb)
        ));
        this->allLayersFramebuffers.push_back(fb);

        //numLayers = 1 if this->isDefaultFB else img.numLayers
        assert((int)this->singleLayerFramebuffers.size() == chainImageIndex);
        this->singleLayerFramebuffers.push_back( std::vector<VkFramebuffer>(numLayers) );

        //now make views for one layer at a time
        for(unsigned j=0;j<numLayers;++j){
            std::vector<VkImageView> tmpviews(2);
            if(this->isDefaultFB){
                tmpviews[0] = ctx->swapchainImageViews[chainImageIndex];   //FIXME: Use own image's tmpviews instead?
                tmpviews[1] = ctx->depthbufferViews[chainImageIndex];
            } else {
                tmpviews[0] = img->layers[j].view();
                tmpviews[1] = this->depthBuffers[chainImageIndex]->layers[0].view();
            }

            VkFramebuffer fb1;
            check(vkCreateFramebuffer(
                this->ctx->dev,
                VkFramebufferCreateInfo{
                    .sType=VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO,
                    .pNext=nullptr,
                    .flags=0,
                    //VK spec 8.2: This will be compatible with either
                    //clear or noclear renderpasses
                    .renderPass = this->singleLayerRenderPassDiscard->renderPass,
                    .attachmentCount = (unsigned)tmpviews.size(),
                    .pAttachments = tmpviews.data(),
                    .width = this->width,
                    .height = this->height,
                    .layers = 1
                },
                nullptr,
                &(fb1)
            ));
            this->singleLayerFramebuffers[chainImageIndex][j] = fb1;
        } //end for j in img.numLayers

    } //end for chain image index

    //make depth buffer views
    this->depthBufferViews.resize(this->depthBuffers.size());
    for(int i=0;i<(int)this->depthBuffers.size();++i){
        Image* D = this->depthBuffers[i];
        this->depthBufferViews[i] = D->createView(
            D->viewType, D->format, VK_IMAGE_ASPECT_DEPTH_BIT,
            0,1,0,1,"depth aspect view");
    }


    this->pushedToGPU=true;
}


void Framebuffer::_cleanup()
{
    for(VkFramebuffer F : this->allLayersFramebuffers){
        vkDestroyFramebuffer(this->ctx->dev,F,nullptr);
    }
    for( std::vector<VkFramebuffer>& L : this->singleLayerFramebuffers  ){
        for(VkFramebuffer F : L ){
            vkDestroyFramebuffer(this->ctx->dev,F,nullptr);
        }
    }
    for( VkImageView v : this->depthBufferViews ){
        vkDestroyImageView(ctx->dev,v,nullptr);
    }
}

void Framebuffer::beginRenderPassDiscardContents(VkCommandBuffer cmd)
{
    /**
        Begin a renderpass that draws into all layers of this framebuffer.
        The initial contents of the framebuffer are undefined.
        @param cmd Command buffer
    */
    int imageIndex = Framebuffer::_currentSwapchainIndex();
    this->beginRenderPassHelper(imageIndex,-1,cmd,VK_ATTACHMENT_LOAD_OP_DONT_CARE, 0.0 ,0.0 , 0.0 , 0.0 );
}

void Framebuffer::beginRenderPassKeepContents( VkCommandBuffer cmd)
{
    /**
        Begin a renderpass that draws into all layers of this framebuffer.
        The initial contents of the framebuffer are retained as-is.
        @param cmd Command buffer
    */
    int imageIndex = Framebuffer::_currentSwapchainIndex();
    this->beginRenderPassHelper(imageIndex,-1,cmd,VK_ATTACHMENT_LOAD_OP_LOAD, 0.0 ,0.0 , 0.0 , 0.0 );
}


void Framebuffer::beginRenderPassClearContents(VkCommandBuffer cmd, float r, float g, float b, float a)
{
    /**
    """ Begin a renderpass that draws into all layers of this framebuffer.
        The framebuffer is cleared before rendering starts.
        The depth buffer is cleared to 1.0 and the stencil buffer is cleared to 0.
        @param cmd Command buffer
        @param r,g,b,a The clear color
    """
    */
    int imageIndex = Framebuffer::_currentSwapchainIndex();
    this->beginRenderPassHelper(imageIndex,-1,cmd,
        VK_ATTACHMENT_LOAD_OP_CLEAR,
        r,g,b,a );
}

void Framebuffer::beginOneLayerRenderPassDiscardContents(int layerIndex, VkCommandBuffer cmd)
{
    /**
    """ Begin a renderpass that draws into one layer of this framebuffer.
     The initial contents of the framebuffer are undefined.
     @param layerIndex The layer to draw to
     @param cmd Command buffer
    """
    */
    int imageIndex = Framebuffer::_currentSwapchainIndex();
    this->beginRenderPassHelper(imageIndex,layerIndex,cmd,VK_ATTACHMENT_LOAD_OP_DONT_CARE, 0.0 ,0.0 , 0.0 , 0.0 );
}



void Framebuffer::beginOneLayerRenderPassKeepContents(int layerIndex, VkCommandBuffer cmd)
{
    /**
     Begin a renderpass that draws into one layer of this framebuffer.
     The initial contents of the framebuffer are retained as-is.
     @param layerIndex The layer to draw to
     @param cmd Command buffer
    */
    int imageIndex = Framebuffer::_currentSwapchainIndex();
    this->beginRenderPassHelper(imageIndex,layerIndex,cmd,VK_ATTACHMENT_LOAD_OP_LOAD, 0.0 ,0.0 , 0.0 , 0.0 );
}



void Framebuffer::beginOneLayerRenderPassClearContents(
         int layerIndex,  VkCommandBuffer cmd,
         float r, float g, float b, float a)
{
    /**
    """ Begin a renderpass that draws into one layer of this framebuffer.
    The framebuffer is cleared before rendering starts. The depth buffer is cleared to 1.0 and the stencil buffer is cleared to 0.
    @param layerIndex The layer to draw to
    @param cmd Command buffer
    @param r,g,b,a The clear color
    """
    */
    int imageIndex = Framebuffer::_currentSwapchainIndex();
    this->beginRenderPassHelper(imageIndex,layerIndex,cmd,VK_ATTACHMENT_LOAD_OP_CLEAR, r,g,b,a );
}


void Framebuffer::endRenderPass(VkCommandBuffer cmd)
{
    /**
    """
    End a render pass and build mipmaps.
    @param cmd The command buffer
    """
    */

    if( !Framebuffer::renderPass )
        fatal("endRenderPass() called while not in a renderpass");
    if(! this->insideRenderpass )
        fatal("endRenderPass() called while not in a renderpass");

    this->ctx->endCmdRegion(cmd);


    vkCmdEndRenderPass(cmd);
    this->insideRenderpass = false;
    Framebuffer::renderPass=nullptr;
    Framebuffer::current = nullptr;

    this->completedRenderIndex = this->currentRenderIndex;
    this->currentRenderIndex = -1;

    if( ! this->isDefaultFB){
        ctx->beginCmdRegion(cmd,"Computing mipmaps index="+str(this->completedRenderIndex));

        //build mipmaps
        Image* img = this->colorBuffers[this->completedRenderIndex];

        //process each layer
        for(unsigned layerNum=0;layerNum<img->numLayers;++layerNum){
            //read from layer 0
            img->layoutTransition(
                VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
                {},
                cmd,
                layerNum,
                0
            );

            for(unsigned mipLevel=1; mipLevel < len(img->layers[0].mips); ++mipLevel ){
                //write to layer mipLevel
                img->layoutTransition(
                    VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
                    {},
                    cmd,
                    layerNum, mipLevel
                );

                VkImageBlit region{
                    .srcSubresource = VkImageSubresourceLayers{
                        .aspectMask=VK_IMAGE_ASPECT_COLOR_BIT,
                        .mipLevel=mipLevel-1,
                        .baseArrayLayer=layerNum,
                        .layerCount=1
                    },
                    .srcOffsets = {
                        VkOffset3D{
                            .x=0, .y=0, .z=0
                        },
                        VkOffset3D{
                            .x=(int)img->layers[layerNum].mips[mipLevel-1].width,
                            .y=(int)img->layers[layerNum].mips[mipLevel-1].height,
                            .z=1
                        }
                    },
                    .dstSubresource = VkImageSubresourceLayers{
                        .aspectMask=VK_IMAGE_ASPECT_COLOR_BIT,
                        .mipLevel=mipLevel,
                        .baseArrayLayer=layerNum,
                        .layerCount=1
                    },
                    .dstOffsets = {
                        VkOffset3D{
                            .x=0,
                            .y=0,
                            .z=0
                        },
                        VkOffset3D{
                            .x=(int)img->layers[layerNum].mips[mipLevel].width,
                            .y=(int)img->layers[layerNum].mips[mipLevel].height,
                            .z=1
                        }
                    }
                };

                vkCmdBlitImage(
                    cmd,
                    img->image,
                    VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
                    img->image,
                    VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
                    1,
                    &(region),
                    VK_FILTER_LINEAR
                );

                //prepare level for reading on the next round
                img->layoutTransition(
                    VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
                    {},
                    cmd,
                    layerNum, mipLevel
                );
            } //end for mip level
        } //end for layer num

        this->colorBuffers[this->completedRenderIndex]->layoutTransition(
            VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
            {},
            cmd,
            {},{}
        );

        this->depthBuffers[this->completedRenderIndex]->layoutTransition(
            VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
            {},
            cmd,
            {},{}
        );

        ctx->endCmdRegion(cmd);
    } //end if not default fb
} //end function

void Framebuffer::endRenderPassNoMipmaps(VkCommandBuffer cmd )
{
    /**
    """End a render pass and do not build mipmaps.
    This should be used if you intend to start another
    renderpass with the same framebuffer without
    reading the framebuffer's contents
    @param cmd The command buffer
    """
    */

    if(! Framebuffer::renderPass ){
        fatal("endRenderPass() called while not in a renderpass");
    }

    if(! this->insideRenderpass ){
        fatal("endRenderPass() called while not in a renderpass");
    }

    ctx->endCmdRegion(cmd);

    vkCmdEndRenderPass(cmd);
    this->insideRenderpass = false;
    Framebuffer::renderPass = nullptr;
    Framebuffer::current = nullptr;

    this->completedRenderIndex = this->currentRenderIndex;
    this->currentRenderIndex = -1;

    if( ! this->isDefaultFB){
        this->colorBuffers[this->completedRenderIndex]->layoutTransition(
            VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
            {},
            cmd,
            {},{}
        );
        this->depthBuffers[this->completedRenderIndex]->layoutTransition(
            VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
            {},
            cmd,
            {},{}
        );
    }
}


Image* Framebuffer::currentImage()
{
    /**
        Return the most recently rendered image. This requires that at least one
        render pass has been completed on this Framebuffer (it will use whatever
        image was last rendered in a renderpass).
    */
    if( this->completedRenderIndex == -1 ){
        fatal("Framebuffer has not been rendered to");
    }
    if(this->isDefaultFB){
        fatal("Cannot get image from default framebuffer");
    }
    return this->colorBuffers[this->completedRenderIndex];
}

VkImageView Framebuffer::currentDepthBufferView()
{
    /**
     Returns view of most recently rendered depth buffer.
     This requires that at least one
     render pass has been completed on this Framebuffer (it will use whatever
     image was last rendered in a renderpass).
    */
    if( this->completedRenderIndex == -1 )
        fatal("Framebuffer has not been rendered to");
    if(this->isDefaultFB)
        fatal("Cannot get image from default framebuffer");
    return this->depthBufferViews[this->completedRenderIndex];
}

void Framebuffer::beginOneLayerRenderPassDiscardContentsWithIndex(
    int imageIndex,
    int layerIndex,
    VkCommandBuffer cmd)
{
    this->beginRenderPassHelper(imageIndex,layerIndex,cmd,VK_ATTACHMENT_LOAD_OP_DONT_CARE, 0.0 ,0.0 , 0.0 , 0.0 );
}

void Framebuffer::beginOneLayerRenderPassKeepContentsWithIndex(int imageIndex,  int layerIndex, VkCommandBuffer cmd)
{
    this->beginRenderPassHelper(imageIndex,layerIndex,cmd,VK_ATTACHMENT_LOAD_OP_LOAD, 0.0 ,0.0 , 0.0 , 0.0 );
}


void Framebuffer::beginRenderPassHelper(
    int imageIndex,  int layerIndex ,  VkCommandBuffer cmd,
    VkAttachmentLoadOp loadOp,
    float clearR, float clearG, float clearB, float clearA
){

    if(! this->pushedToGPU )
        this->pushToGPU();

    std::string s = (
        "renderPass: " +
        this->name + ", imageIndex=" + str(imageIndex)
    );

    if( layerIndex == -1 )
        s += "all layers";
    else
        s += "layer "+str(layerIndex);

    this->ctx->beginCmdRegion(cmd, s);

    assert(imageIndex >= 0 );
    assert(imageIndex < this->ctx->numSwapchainImages);

    if(layerIndex == -1){
        ;        //all layers
    } else {
        if(layerIndex < 0 or layerIndex >= (int)this->numLayers){
            if( this->numLayers == 1 ){
                fatal("Bad layer (" + str(layerIndex)+
                    ") for Framebuffer::beginRenderPass ["+this->name+"]; must be -1 or 0");
            } else {
                fatal("Bad layer (" + str(layerIndex)+
                    ") for Framebuffer::beginRenderPass ["+this->name+"]; must be -1 or "+
                    "in range [0..."+str(this->numLayers-1)+"]");
            }
        }
    }
    int numLayersToDraw = ( (layerIndex == -1) ? this->numLayers : 1 );

    if( this->insideRenderpass)
        fatal("Cannot begin a new renderpass while we're still in another renderpass");

    this->insideRenderpass = true;
    this->currentRenderIndex = imageIndex;
    //will set Framebuffer::renderPass later

    if( ! this->isDefaultFB) {

        if( !len(this->allLayersFramebuffers) || ! this->allLayersFramebuffers[0]){
            fatal("Framebuffer::beginRenderPass() called, but Framebuffer hasn't been pushed to GPU yet");
        }

        this->colorBuffers[imageIndex]->layoutTransition(
            VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
            {},
            cmd,
            {},{}
        );

        this->depthBuffers[imageIndex]->layoutTransition(
            VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL,
            {},
            cmd,
            {},{}
        );
    } //end if not default fb

    RenderPass* rp=nullptr;
    std::vector<VkClearValue> clearValues;
    if( loadOp == VK_ATTACHMENT_LOAD_OP_CLEAR ){
        VkClearValue tmp;
        tmp.color.float32[0] = clearR;
        tmp.color.float32[1] = clearG;
        tmp.color.float32[2] = clearB;
        tmp.color.float32[3] = clearA;
        while(len(clearValues) < (unsigned)numLayersToDraw){
            clearValues.push_back(tmp);
        }
        tmp.depthStencil.depth=1.0;
        tmp.depthStencil.stencil=0;
        clearValues.push_back(tmp);
    } //#end if
    if( layerIndex == -1 ){
        //draw all layers
        if(loadOp == VK_ATTACHMENT_LOAD_OP_DONT_CARE)
            rp = this->allLayersRenderPassDiscard;
        else if( loadOp == VK_ATTACHMENT_LOAD_OP_LOAD )
            rp = this->allLayersRenderPassKeep;
        else if (loadOp == VK_ATTACHMENT_LOAD_OP_CLEAR)
            rp = this->allLayersRenderPassClear;
        else
            fatal("Internal error: Bad loadOp");
    } else {
        if(loadOp == VK_ATTACHMENT_LOAD_OP_DONT_CARE)
            rp = this->singleLayerRenderPassDiscard;
        else if( loadOp == VK_ATTACHMENT_LOAD_OP_LOAD)
            rp = this->singleLayerRenderPassKeep;
        else if( loadOp ==  VK_ATTACHMENT_LOAD_OP_CLEAR)
            rp = this->singleLayerRenderPassClear;
        else
            fatal("Internal error: Bad loadOp");
    }

    VkFramebuffer FF;
    if(layerIndex == -1)
        FF=this->allLayersFramebuffers[imageIndex];
    else
        FF=this->singleLayerFramebuffers[imageIndex][layerIndex];

    vkCmdBeginRenderPass(
        cmd,
        VkRenderPassBeginInfo{
            .sType=VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO,
            .pNext=nullptr,
            .renderPass = rp->renderPass,
            .framebuffer = FF,
            .renderArea = VkRect2D{
                .offset = VkOffset2D{
                    .x=0,
                    .y=0
                },
                .extent = VkExtent2D{
                    .width=this->width,
                    .height=this->height
                }
            },
            .clearValueCount = (unsigned)clearValues.size(),
            .pClearValues = (clearValues.empty() ? nullptr : clearValues.data() )
        },
        VK_SUBPASS_CONTENTS_INLINE
    );

    Framebuffer::renderPass = rp;
    Framebuffer::current = this;
}


void Framebuffer::_doLayoutTransition(VkImage image,  VkImageAspectFlagBits aspect,
         VkImageLayout oldLayout, VkImageLayout newLayout,  VkCommandBuffer cmd)
{
    VkImageMemoryBarrier B{
        .sType=VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER,
        .pNext=nullptr,
        .srcAccessMask=VK_ACCESS_MEMORY_READ_BIT|VK_ACCESS_MEMORY_WRITE_BIT,
        .dstAccessMask=VK_ACCESS_MEMORY_READ_BIT|VK_ACCESS_MEMORY_WRITE_BIT,
        .oldLayout=oldLayout,
        .newLayout=newLayout,
        .srcQueueFamilyIndex=VK_QUEUE_FAMILY_IGNORED,
        .dstQueueFamilyIndex=VK_QUEUE_FAMILY_IGNORED,
        .image=image,
        .subresourceRange = VkImageSubresourceRange{
            .aspectMask=(VkImageAspectFlags)aspect, //VK_IMAGE_ASPECT_COLOR_BIT,
            .baseMipLevel=0,
            .levelCount=1,
            .baseArrayLayer=0,
            .layerCount=1
        }
    };

    //FIXME: Check stage flags
    vkCmdPipelineBarrier(
        cmd,
        VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT,
        VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT,
        0,
        0, nullptr,    //memory barriers
        0, nullptr,    //buffer barriers
        1,
        &(B)
    );
}
