#include "BlurrableFramebuffer.h"
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

static void _initBlurCommon(VulkanContext* ctx);



typedef std::tuple<int,int,VkFormat> FBKey;

//helper FB's for blurring. To save memory,
//we share helpers: They are keyed by
//width, height, and format. If two FB's have
//same w,h,fmt then they will share the same helper
static std::map<FBKey,Framebuffer*> blurHelpers;

//private vertex manager for blurring quads
static VertexManager* fbVertexManager;

//private push constants for blurring
static PushConstants* fbBlurPushConstants;

//private DSL for blur operations
static DescriptorSetLayout* fbBlurDescriptorSetLayout;

//private factory for blur operations
static DescriptorSetFactory* fbBlurDescriptorSetFactory;

//shaders for blurring. We could have used a compute shader
//too, but this works and we don't have to worry about switching
//which queue/pipe we're using. blurddfs = distance dependent blur fs
static VkPipelineShaderStageCreateInfo blurvs, blurfs, blurddfs;

//layout for blurring
static PipelineLayout* fbBlurPipelineLayout;

//vertex/index info for blur quad
static VertexManager::Info blurQuadInfo;

static DescriptorSet* blurDescriptorSet;

Sampler* clampingMipSampler;
Sampler* nearestSampler;


//128 bytes for push constants = 32 floats = 8 vec4's
static std::string blurPushConstantSrc =
    "layout(push_constant,row_major) uniform pushConstants{\n"
    "    uvec4 blurWeights[7];\n"
    "    vec2 blurDelta;\n"
    "    float blurMultiplier;\n"
    "    uint blurLayerAndIterations;\n"
    "};";

static std::string blurvsSrc = std::string(
    "#version 450 core\n"
    "layout(location=0) in vec2 position;\n"
    "layout(location=0) out vec2 v_texcoord;\n"
    "void main(){\n"
    "    gl_Position = vec4( 2.0*position-vec2(1.0), 0.0, 1.0 );\n"
    "    v_texcoord = position;\n"
    "}");


//this code is a bit more complex than it could be
//since we want to pack two weights into one 32 bit float
//so we can get twice the maximum radius
//This code is used for both distance dependent and non-distance dependent
//blur, with a conditional #define
static std::string blurfsSrc = std::string(
    "layout(location=0) in vec2 texcoord;                               \n"
    "layout(location=0) out vec4 color;                                 \n"
    "layout(set=0,binding=0) uniform sampler samp; //clamp,mip          \n"
    "layout(set=0,binding=1) uniform texture2DArray tex;                \n"
    "layout(set=0,binding=2) uniform sampler nsamp; //clamp, nearest    \n"
    "layout(set=0,binding=3) uniform texture2DArray depthBuffer;        \n"
    ) + blurPushConstantSrc + std::string(
    "                                                                   \n"
    "#line 84                                                           \n"
    "#if USE_DEPTH != 0                                                 \n"
    "void addPoint(                                                     \n"
    "        float weight, vec3 texc,                                   \n"
    "        float centerDepth, inout vec4 total,                       \n"
    "        inout float totalWeight )                                  \n"
    "{                                                                  \n"
    "    float d = texture(                                             \n"
    "        sampler2DArray(depthBuffer,nsamp),texc                     \n"
    "    ).r;                                                           \n"
    "    if( d < centerDepth )                                          \n"
    "        return;                                                    \n"
    "    total += weight * texture(                                     \n"
    "             sampler2DArray(tex,samp),                             \n"
    "             texc                                                  \n"
    "    );                                                             \n"
    "    totalWeight += weight;                                         \n"
    "}                                                                  \n"
    "#else                                                              \n"
    "void addPoint(float weight, vec3 texc,inout vec4 total)            \n"
    "{                                                                  \n"
    "    total += weight * texture(                                     \n"
    "             sampler2DArray(tex,samp),                             \n"
    "             texc                                                  \n"
    "    );                                                             \n"
    "}                                                                  \n"
    "#endif                                                             \n"
    "void main(){                                                       \n"
    "                                                                   \n"
    "   uint blurLayer = blurLayerAndIterations>>16;                    \n"
    "   uint blurIterations = blurLayerAndIterations & 0xffff;          \n"
    "                                                                   \n"
    "   vec3 t = vec3(texcoord,float(blurLayer));                       \n"
    "   vec4 total = vec4(0);                                           \n"
    "   vec3 t1 = t;  //tex coord to the right                          \n"
    "   vec3 t2 = t;  //tex coord to the left                           \n"
    "                                                                   \n"
    "   ivec2 texSize = textureSize(sampler2DArray(tex,samp),0).xy;     \n"
    "   vec2 delta = 1.0/texSize;                                       \n"
    "   delta *= blurDelta;                                             \n"
    "                                                                   \n"
    "   #if USE_DEPTH == 1                                              \n"
    "       float totalWeight=0.0;                                      \n"
    "       float centerDepth = texture(                                \n"
    "           sampler2DArray(depthBuffer,nsamp), t                    \n"
    "       ).r;                                                        \n"
    "       gl_FragDepth=centerDepth;                                   \n"
    "   #endif                                                          \n"
    "                                                                   \n"
    "   for(int i=0;i<7;++i){                                           \n"
    "       if( blurWeights[i] == uvec4(0) )                            \n"
    "           break;                                                  \n"
    "       for(int j=0;j<4;++j){                                       \n"
    "           uint tmp = blurWeights[i][j];                           \n"
    "           //sample point at center of kernel twice, but the       \n"
    "           //weight has already been scaled                        \n"
    "           //to be half of what it should be (cpu side), so it     \n"
    "           //all works out                                         \n"
    "           float w1 = float( tmp>>16 )/65535.0;                    \n"
    "           float w2 = float( tmp & 0xffff ) / 65535.0;             \n"
    "           #if USE_DEPTH == 1                                      \n"
    "               addPoint(w1,t1,centerDepth,total,totalWeight);      \n"
    "               addPoint(w1,t2,centerDepth,total,totalWeight);      \n"
    "           #else                                                   \n"
    "               addPoint(w1,t1,total);                              \n"
    "               addPoint(w1,t2,total);                              \n"
    "           #endif                                                  \n"
    "           t1.xy += delta;                                         \n"
    "           t2.xy -= delta;                                         \n"
    "           #if USE_DEPTH                                           \n"
    "               addPoint(w2,t1,centerDepth,total,totalWeight);      \n"
    "               addPoint(w2,t2,centerDepth,total,totalWeight);      \n"
    "           #else                                                   \n"
    "               addPoint(w2,t1,total);                              \n"
    "               addPoint(w2,t2,total);                              \n"
    "           #endif                                                  \n"
    "           t1.xy += delta;                                         \n"
    "           t2.xy -= delta;                                         \n"
    "       }                                                           \n"
    "   }                                                               \n"
    "   #if USE_DEPTH                                                   \n"
    "       total *= 1.0/totalWeight;                                   \n"
    "   #endif                                                          \n"
    "   color = total*blurMultiplier;                                   \n"
    "}                                                                  \n"
);



BlurrableFramebuffer::BlurrableFramebuffer(
    VulkanContext* ctx_,
    unsigned width_, unsigned height_, unsigned layers_,
    VkFormat format_, std::string name_) : Framebuffer(ctx_,width_,height_,layers_,format_,name_)
{
    _initBlurCommon(ctx_);

    FBKey k{ this->width, this->height, this->format };
    if( !blurHelpers.contains(k) ){
        blurHelpers[k] = new Framebuffer(
            this->ctx,
            this->width, this->height, 1,
            this->format,
            "blur helper "+std::to_string(this->width)+"x"+
                std::to_string(this->height)
        );
    }
    this->blurHelper = blurHelpers[k];



    //use blur shaders, use render pass discard, don't check or change depth buffer
    this->blurPipeline = new GraphicsPipeline(
        ctx_,
        "blurPipeline for "+this->name+" "+std::to_string(this->width)+"x"+std::to_string(this->height),
        fbBlurPipelineLayout,
        //~ {this,this->blurHelper},
        PipelineOption{.vertexInputState=fbVertexManager->inputState},
        PipelineOption{.shader=blurvs},
        PipelineOption{.shader=blurfs},
        PipelineOption{.depthStencilState =
            VkPipelineDepthStencilStateCreateInfo{
                .sType=VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO,
                .pNext=nullptr,
                .flags=0,
                .depthTestEnable=VK_FALSE,
                .depthWriteEnable=VK_FALSE,
                .depthCompareOp=VK_COMPARE_OP_LESS_OR_EQUAL,
                .depthBoundsTestEnable=VK_FALSE,
                .stencilTestEnable=VK_FALSE,
                .front=VkStencilOpState{
                    .failOp=VK_STENCIL_OP_KEEP,
                    .passOp=VK_STENCIL_OP_KEEP,
                    .depthFailOp=VK_STENCIL_OP_KEEP,
                    .compareOp=VK_COMPARE_OP_ALWAYS,
                    .compareMask=0xff,
                    .writeMask=0xff,
                    .reference=0
                },
                .back=VkStencilOpState{
                    .failOp=VK_STENCIL_OP_KEEP,
                    .passOp=VK_STENCIL_OP_KEEP,
                    .depthFailOp=VK_STENCIL_OP_KEEP,
                    .compareOp=VK_COMPARE_OP_ALWAYS,
                    .compareMask=0xff,
                    .writeMask=0xff,
                    .reference=0
                },
                .minDepthBounds=0.0,
                .maxDepthBounds=1.0
            }
        }
    );

    //use blur shaders, use render pass discard,
    //depth test always passes, depth writes enabled so
    //we can copy depth buffer to helper
    this->blurDDPipelineWriteToHelper = new GraphicsPipeline(
        ctx_,
        "blurDDPipelineWriteToHelper for "+this->name,
        fbBlurPipelineLayout,
        //~ {this,this->blurHelper},
        PipelineOption{.vertexInputState=fbVertexManager->inputState},
        PipelineOption{.shader=blurvs},
        PipelineOption{.shader=blurddfs},
        PipelineOption{.depthStencilState =
            VkPipelineDepthStencilStateCreateInfo{
                .sType=VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO,
                .pNext=nullptr,
                .flags=0,
                .depthTestEnable=VK_TRUE,
                .depthWriteEnable=VK_TRUE,
                .depthCompareOp=VK_COMPARE_OP_ALWAYS,
                .depthBoundsTestEnable=VK_FALSE,
                .stencilTestEnable=VK_FALSE,
                .front=VkStencilOpState{
                    .failOp=VK_STENCIL_OP_KEEP,
                    .passOp=VK_STENCIL_OP_KEEP,
                    .depthFailOp=VK_STENCIL_OP_KEEP,
                    .compareOp=VK_COMPARE_OP_ALWAYS,
                    .compareMask=0xff,
                    .writeMask=0xff,
                    .reference=0
                },
                .back=VkStencilOpState{
                    .failOp=VK_STENCIL_OP_KEEP,
                    .passOp=VK_STENCIL_OP_KEEP,
                    .depthFailOp=VK_STENCIL_OP_KEEP,
                    .compareOp=VK_COMPARE_OP_ALWAYS,
                    .compareMask=0xff,
                    .writeMask=0xff,
                    .reference=0
                },
                .minDepthBounds=0.0,
                .maxDepthBounds=1.0
            }
        }
    );

    //use blur shaders, use render pass discard,
    //depth buffer not changed
    this->blurDDPipelineWriteToFB =new GraphicsPipeline(
        ctx_,
        "blurPipeline for "+this->name,
        fbBlurPipelineLayout,
        //~ {this,this->blurHelper},
        PipelineOption{.vertexInputState=fbVertexManager->inputState},
        PipelineOption{.shader=blurvs},
        PipelineOption{.shader=blurddfs},
        PipelineOption{.depthStencilState =
            VkPipelineDepthStencilStateCreateInfo{
                .sType=VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO,
                .pNext=nullptr,
                .flags=0,
                .depthTestEnable=VK_FALSE,
                .depthWriteEnable=VK_FALSE,
                .depthCompareOp=VK_COMPARE_OP_LESS_OR_EQUAL,
                .depthBoundsTestEnable=VK_FALSE,
                .stencilTestEnable=VK_FALSE,
                .front=VkStencilOpState{
                    .failOp=VK_STENCIL_OP_KEEP,
                    .passOp=VK_STENCIL_OP_KEEP,
                    .depthFailOp=VK_STENCIL_OP_KEEP,
                    .compareOp=VK_COMPARE_OP_ALWAYS,
                    .compareMask=0xff,
                    .writeMask=0xff,
                    .reference=0
                },
                .back=VkStencilOpState{
                    .failOp=VK_STENCIL_OP_KEEP,
                    .passOp=VK_STENCIL_OP_KEEP,
                    .depthFailOp=VK_STENCIL_OP_KEEP,
                    .compareOp=VK_COMPARE_OP_ALWAYS,
                    .compareMask=0xff,
                    .writeMask=0xff,
                    .reference=0
                },
                .minDepthBounds=0.0,
                .maxDepthBounds=1.0
            }
        }
    );


    CleanupManager::registerCleanupFunction([this](){
    });


}

static void _initBlurCommon(VulkanContext* ctx)
{

    static bool initializedCommon=false;

    if(initializedCommon)
        return;

    assert(ctx);

    initializedCommon = true;

    nearestSampler = new Sampler( ctx,
            VK_FILTER_NEAREST,
            VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE,
            false
    );
    clampingMipSampler = new Sampler( ctx,
            VK_FILTER_LINEAR,
            VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE,
            true
    );

    ShaderManager::initialize(ctx);
    fbVertexManager = new VertexManager(
        ctx,
        {
            VertexManager::Input{
                .name="positions",
                .format=VK_FORMAT_R32G32_SFLOAT,
                .location=0,
                .rate=VK_VERTEX_INPUT_RATE_VERTEX
            }
        }
    );

    blurQuadInfo = fbVertexManager->addIndexedData(
        "positions", std::vector<math2801::vec2>{ {0,0}, {0,1}, {1,1}, {1,0} },
        "indices", std::vector<std::uint32_t>{0,1,2, 0,2,3}
    );
    fbVertexManager->pushToGPU();

    fbBlurPushConstants = new PushConstants(blurPushConstantSrc);

    fbBlurDescriptorSetLayout = new DescriptorSetLayout(ctx,
        {
            { .type=VK_DESCRIPTOR_TYPE_SAMPLER,         .slot=0   },
            { .type=VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE,   .slot=1   },
            { .type=VK_DESCRIPTOR_TYPE_SAMPLER,         .slot=2   },
            { .type=VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE,   .slot=3   }
        }
    );

    fbBlurPipelineLayout = new PipelineLayout(
        ctx,
        fbBlurPushConstants->byteSize,
        fbBlurDescriptorSetLayout,
        "fbBlurPipelineLayout"
    );

    fbBlurDescriptorSetFactory = new DescriptorSetFactory(
        ctx, "FBOBlurDSFactory",
        0,      //binding point
        fbBlurPipelineLayout
    );

    blurDescriptorSet = fbBlurDescriptorSetFactory->make();

    blurvs = ShaderManager::loadFromString(blurvsSrc,"vert","blurvs");
    blurfs = ShaderManager::loadFromString(  "#version 450 core\n#define USE_DEPTH 0\n"+blurfsSrc,"frag","blurfs");
    blurddfs = ShaderManager::loadFromString("#version 450 core\n#define USE_DEPTH 1\n"+blurfsSrc,"frag","blurddfs");

}

//FIXME: Also need to restore per-frame descriptor set
void BlurrableFramebuffer::blur(unsigned radius, unsigned layer,
    float multiplier, VkCommandBuffer cmd, VertexManager* oldVM)
{
    this->blur(radius,layer,multiplier,cmd,oldVM,false);
}

void BlurrableFramebuffer::blur(unsigned radius, unsigned layer,
    float multiplier, VkCommandBuffer cmd, VertexManager* oldVM,
    bool doDistanceDependentBlur)
{

    ctx->beginCmdRegion(cmd,"Blur");
    fbVertexManager->bindBuffers(cmd);

    if( layer >= this->numLayers ){
        if( this->numLayers == 1 )
            fatal("Bad layer number for blur (" + std::to_string(layer)+"): This Framebuffer only has 1 layer\n");
        else
            fatal("Bad layer number for blur (" + std::to_string(layer)+"): Must be between 0 and " + std::to_string(this->numLayers));
    }

    if( this->insideRenderpass ){
        fatal("Cannot blur a framebuffer while it is the render destination");
    }

    if( this->completedRenderIndex == -1 ){
        fatal("Cannot blur framebuffer: You have not yet rendered to it");
    }

    if( !this->blurHelper ){
        fatal("Cannot blur the window's framebuffer");
    }

    //we pack two weights per uint, so
    //n uvec4's give us n*8 weights
    //This gives us a maximum blur radius of 56
    //since we have 7 uvec4's available in the push constants
    static std::map<int, std::vector<uvec4> > weights;
    std::map<int, std::vector<uvec4> >::iterator witer;
    witer = weights.find(radius);
    if( witer == weights.end() ){
        std::vector<uvec4> wi;
        wi.resize(7, {0,0,0,0} );
        std::vector<float> F(wi.size()*4*2);
        if( radius > (unsigned)F.size() ){
            fatal("Blur radius (" + std::to_string(radius)+") is too large");
        }
        float sigma = float(radius) / 3.0f;
        float sum = 0.0f;
        for(unsigned i=0;i<radius;++i){
            float Q = float((i * i) / (-2.0 * sigma * sigma));
            float wt = float(exp(Q) / (sigma * sqrt(2.0 * 3.14159265358979323)));
            //special case; we process kernel center twice in shader so we halve the weight
            if( i == 0 )
                wt *= 0.5f;
            assert(wt < 1.0f);
            sum += wt+wt;       //double slot 0 too since we apply it twice and we halved it above
            F[i]=wt;
        }
        float s = 1.0f/sum;
        for(int i=0,j=0,k=0;i<(int)F.size();i+=2){
            float f1 = F[i]*s;
            float f2 = F[i+1]*s;
            unsigned u1 = unsigned( f1*65535 + 0.5 );
            assert(u1 < 65536);
            unsigned u2 = unsigned( f2*65535 + 0.5 );
            assert(u2 < 65536);
            wi[j][k] = unsigned( (u1<<16) | u2 ) ;
            k++;
            j += (k/4);
            k %= 4;
        }
        double ss=0.0;
        for(auto u : wi ){
            for(int xx=0;xx<4;++xx){
                double ff = (u[xx]>>16)/65535.0*2;
                ss += ff;
                ff = (u[xx]&0xffff)/65535.0*2;
                ss += ff;
            }
        }
        weights[radius] = wi;
        witer = weights.find(radius);
    }

    ctx->insertCmdLabel(cmd,"Begin blur");

    assert(this->completedRenderIndex != -1 );
    this->blurHelper->beginOneLayerRenderPassDiscardContentsWithIndex(
        this->completedRenderIndex,0,cmd);

    //write to the blurHelper FBO
    if( doDistanceDependentBlur ){
        this->blurDDPipelineWriteToHelper->use(cmd);
    } else {
        this->blurPipeline->use(cmd);
    }

    //kernel width = 1 + blurWidth*4*2
    //in other words, we always have at least one element
    //and then we do blurWidth*4 elements to the left & right.
    //To get kernel of arbitrary size, pad blurWeights with zeros.
    fbBlurPushConstants->set(cmd,"blurMultiplier", multiplier );
    //FIXME: iteration count is too pessimistic
    fbBlurPushConstants->set(cmd,"blurLayerAndIterations", (layer<<16) | (radius/2+1) );
    fbBlurPushConstants->set(cmd,"blurWeights", witer->second);
    fbBlurPushConstants->set(cmd,"blurDelta",vec2(1.0f,0.0f) );


    //read from this FBO
    blurDescriptorSet->setSlot(0,clampingMipSampler->sampler);
    blurDescriptorSet->setSlot(1,this->colorBuffers[this->completedRenderIndex]->view());
    blurDescriptorSet->setSlot(2,nearestSampler->sampler);
    blurDescriptorSet->setSlot(3,this->depthBufferViews[this->completedRenderIndex]);
    blurDescriptorSet->bind(cmd);

    //draw the quad
    vkCmdDrawIndexed(
        cmd,
        blurQuadInfo.numIndices,
        1,              //instance count
        blurQuadInfo.indexOffset,
        blurQuadInfo.vertexOffset,
        0               //first instance
    );

    this->blurHelper->endRenderPass(cmd);

    //this->beginOneLayerRenderPassDiscardContentsWithIndex(this->completedRenderIndex,layer,cmd);
    this->beginOneLayerRenderPassKeepContentsWithIndex(this->completedRenderIndex,layer,cmd);
    fbBlurPushConstants->set(cmd,"blurDelta",vec2(0.0f,1.0f) );
    //FIXME: iterations is too pessimistic
    fbBlurPushConstants->set(cmd,"blurLayerAndIterations", radius/2+1 );

    //write to this fbo
    if( doDistanceDependentBlur ){
        this->blurDDPipelineWriteToFB->use(cmd);
    } else {
        this->blurPipeline->use(cmd);
    }

    //~ this->blurDescriptorSet->setSlot(0,Samplers::clampingMipSampler);
    blurDescriptorSet->setSlot(1,this->blurHelper->colorBuffers[this->completedRenderIndex]->view());
    //~ this->blurDescriptorSet->setSlot(2,Samplers::nearestSampler);
    blurDescriptorSet->setSlot(3,this->blurHelper->depthBufferViews[this->completedRenderIndex]);
    blurDescriptorSet->bind(cmd);


    //read from the blur helper
    //~ this->blurHelper->blurDescriptorSets[this->lastRenderedImage]->bind(cmd);

    vkCmdDrawIndexed(
        cmd,
        blurQuadInfo.numIndices,
        1,              //instance count
        blurQuadInfo.indexOffset,
        blurQuadInfo.vertexOffset,
        0               //first instance
    );


    ctx->insertCmdLabel(cmd,"End blur");

    this->endRenderPass(cmd);


    if(oldVM)
        oldVM->bindBuffers(cmd);

    ctx->endCmdRegion(cmd);

}
