#include "Framework.h"
#include "Text.h"
#include <iostream>
#include <fstream>
#include <cassert>
#include <string>
#include <cstring>
#include "json.h"
#include "math2801.h"
#include "vk.h"
using namespace math2801;

//maximum number of characters on screen
#define MAX_CHARS 1000

#pragma pack(push,1)
class CharData{
  public:
  std::uint32_t screenXY;
  std::uint32_t atlasXY;
  std::uint32_t ascentDescentWidthLeftBearing;
  std::uint32_t color;
};
#pragma pack(pop)


Text::Text( VulkanContext* ctx_, Framebuffer* framebuffer, std::string fontfile, std::string metricsfile)
{
    this->ctx=ctx_;
    std::ifstream fp(metricsfile);
    if(!fp.good())
        assert(0);
    std::string mdata;
    std::getline(fp, mdata, '\0');

    auto M = json11::Json::parse(mdata);
    for( std::string key : M ){
        auto m = M[key];
        Metrics mm;
        mm.ascent = m["ascent"];
        mm.descent = m["descent"];
        mm.width = m["width"];
        mm.advance = m["advance"];
        mm.x = m["x"];
        mm.y = m["y"];
        mm.leftbearing = m["leftbearing"];
        this->metrics[key[0]] = mm;
    }

    this->textureAtlas = ImageManager::load(fontfile);
    this->vertexManager = new VertexManager(ctx,
        {
            VertexManager::Input{
                .name="positions",
                .format=VK_FORMAT_R32G32B32_SFLOAT,
                .location=0,
                .rate=VK_VERTEX_INPUT_RATE_VERTEX

            }
        }
    );

    std::vector<vec3> P;
    std::vector<std::uint32_t> I;
    int idx=0;
    for(int i=0;i<MAX_CHARS;++i){
        P.push_back(vec3( 0,0,i ) );    //top left
        P.push_back(vec3( 1,0,i ) );    //top right
        P.push_back(vec3( 1,1,i ) );    //bottom right
        P.push_back(vec3( 0,1,i ) );    //bottom left
        I.push_back(idx);
        I.push_back(idx+1);
        I.push_back(idx+2);
        I.push_back(idx);
        I.push_back(idx+2);
        I.push_back(idx+3);
        idx += 4;
    }

    this->drawinfo = this->vertexManager->addIndexedData(
        "positions", P,
        "indices", I
    );

    this->vertexManager->pushToGPU();


    this->textData.resize(MAX_CHARS);

    int x=100;
    int y=200;
    std::string text = "This is a test";
    this->numChars = 0;
    for(unsigned i=0;i<(unsigned)text.length();++i){
        char ch = text[i];
        this->appendCharacter(ch,x,y,0.5,0.25,0.125,0.75);
        x += this->metrics[ch].advance;
    }

    this->textBuffer = new DeviceLocalBuffer(
        ctx,
        this->textData,
        VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_UNIFORM_TEXEL_BUFFER_BIT,
        "textBuffer"
    );

    this->stagingBuffer = new StagingBuffer(
        ctx,
        VK_BUFFER_USAGE_TRANSFER_SRC_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT,
        nullptr, this->textBuffer->byteSize,
        "textstaging"
    );

    this->textView = this->textBuffer->makeView(VK_FORMAT_R32G32B32A32_UINT,"textView");

    this->descriptorSetLayout = new DescriptorSetLayout(
        ctx,
        {
            DescriptorSetEntry( VK_DESCRIPTOR_TYPE_SAMPLER,0 ),
            DescriptorSetEntry( VK_DESCRIPTOR_TYPE_UNIFORM_TEXEL_BUFFER,1 ),
            DescriptorSetEntry( VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE,2 )
        }
    );

    this->pushConstants = new PushConstants(
        "shaders/textpushconstants.txt"
    );

    this->screenSize = vec2(framebuffer->width,framebuffer->height);

    this->pipelineLayout = new PipelineLayout(
        ctx,
        this->pushConstants->byteSize,
        this->descriptorSetLayout,
        "text layout"
    );

    this->pipeline = new GraphicsPipeline(
        ctx,
        "text pipe",
        this->pipelineLayout,
        //~ {framebuffer},
        PipelineOption{.shader=ShaderManager::load("shaders/text.vert")},
        PipelineOption{.shader=ShaderManager::load("shaders/text.frag")},
        PipelineOption{.vertexInputState=vertexManager->inputState},
        PipelineOption{.blendEnable=true},
        PipelineOption{.srcColorBlendFactor=VK_BLEND_FACTOR_SRC_ALPHA},
        PipelineOption{.dstColorBlendFactor=VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA}
    );


    this->descriptorSetFactory = new DescriptorSetFactory(
        ctx,
        "text",
        0,  //binding point
        this->pipelineLayout
    );

    this->descriptorSet = this->descriptorSetFactory->make();

    this->sampler = new Sampler(
        ctx,
        VK_FILTER_NEAREST,
        VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE,
        false
    );
    this->descriptorSet->setSlot(0, this->sampler->sampler );
    this->descriptorSet->setSlot(1, this->textView );

    this->dirty=false;

    ImageManager::addCallback( [this](){
        this->descriptorSet->setSlot(2, this->textureAtlas->view());
    });
    CleanupManager::registerCleanupFunction( [this](){ this->cleanup(); } );
}




void Text::appendCharacter(char ch, int x, int y, float r, float g, float b, float a)
{
    int idx = this->numChars;
    auto M = this->metrics[(unsigned)ch];
    this->textData[idx].screenXY = (x << 16) | y;
    this->textData[idx].atlasXY = ( M.x <<16) | M.y;
    this->textData[idx].ascentDescentWidthLeftBearing = (
        ((M.ascent)<<24)   |
        ((M.descent)<<16)  |
        ((M.width)<<8)     |
        ((M.leftbearing)+128)      //make positive
    );
    this->textData[idx].color = ( int(r*255) << 24) | ( int(g*255) << 16) | (int(b*255)<<8) | (int(a*255));
    this->numChars+=1;
}

void Text::cleanup()
{
    this->textBuffer->cleanup();
    vkDestroyBufferView(this->ctx->dev,this->textView,nullptr);
    this->stagingBuffer->cleanup();
}

void Text::clear()
{
    this->numChars=0;
}

void Text::print(int x, int y, float r, float g, float b, float a, std::string args)
{
    for(char ch : args ){
        this->appendCharacter(ch,x,y,r,g,b,a);
        x += int(this->metrics[ch].advance);
    }
    this->dirty=true;
}


void Text::update(VkCommandBuffer cmd)
{
    if(!this->dirty)
        return;
    void* p;
    vkMapMemory(
        this->ctx->dev,
        this->stagingBuffer->memory,
        0,                  //offset
        this->stagingBuffer->byteSize,
        0,                  //flags
        &p
    );

    assert( p != nullptr );

    std::memcpy(
        p,
        this->textData.data(),
        MAX_CHARS * 16      //4 uint's, 4 bytes per uint
    );

    vkUnmapMemory(
        this->ctx->dev,
        this->stagingBuffer->memory
    );

    VkMemoryBarrier membar{
        .sType=VK_STRUCTURE_TYPE_MEMORY_BARRIER,
        .pNext=nullptr,
        .srcAccessMask=VK_ACCESS_MEMORY_WRITE_BIT,
        .dstAccessMask=VK_ACCESS_MEMORY_READ_BIT,
    };

    vkCmdPipelineBarrier(
        cmd,
        VK_PIPELINE_STAGE_HOST_BIT,
        VK_PIPELINE_STAGE_TRANSFER_BIT,
        0,      //flags
        1, &membar,
        0, nullptr,     //buffer barriers
        0, nullptr      //image barriers
    );

    VkBufferCopy region{
        .srcOffset=0,
        .dstOffset=0,
        .size=MAX_CHARS * 16      //4 uint's, 4 bytes per uint
    };

    vkCmdCopyBuffer(
        cmd,
        this->stagingBuffer->buffer,
        this->textBuffer->buffer,
        1,
        &region
    );


    vkCmdPipelineBarrier(
        cmd,
        VK_PIPELINE_STAGE_TRANSFER_BIT,
        VK_PIPELINE_STAGE_VERTEX_SHADER_BIT|VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
        0,              //flags
        1, &membar,
        0, nullptr,     //buffer barriers
        0, nullptr      //image barriers
    );


    this->dirty=false;
}

void Text::draw(VkCommandBuffer cmd)
{
    ctx->beginCmdRegion(cmd,"Drawing text");
    this->pipeline->use(cmd);
    this->descriptorSet->bind(cmd);
    this->vertexManager->bindBuffers(cmd);
    this->pushConstants->set(cmd,
        "screenSize",this->screenSize
    );
    vkCmdDrawIndexed( cmd,
        this->numChars*6,       //6 indices per character
        1,                      //one instance
        this->drawinfo.indexOffset,
        this->drawinfo.vertexOffset,
        0                       //first instance
    );
    ctx->endCmdRegion(cmd);

}
