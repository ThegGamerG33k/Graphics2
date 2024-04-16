#include "VertexManager.h"
#include "Buffers.h"
#include <cassert>
#include "utils.h"
#include "mischelpers.h"
#include "CleanupManager.h"
#include <set>
#include <algorithm>

static
unsigned sizeForFormat(VkFormat fmt)
{
    switch(fmt){
        case VK_FORMAT_R32_UINT:
        case VK_FORMAT_R32_SINT:
        case VK_FORMAT_R32_SFLOAT:          return 4;
        case VK_FORMAT_R32G32_UINT:
        case VK_FORMAT_R32G32_SINT:
        case VK_FORMAT_R32G32_SFLOAT:       return 8;
        case VK_FORMAT_R32G32B32_UINT:
        case VK_FORMAT_R32G32B32_SINT:
        case VK_FORMAT_R32G32B32_SFLOAT:    return 12;
        case VK_FORMAT_R32G32B32A32_UINT:
        case VK_FORMAT_R32G32B32A32_SINT:
        case VK_FORMAT_R32G32B32A32_SFLOAT: return 16;
        default:
            fatal("Bad format");
    }
}

VertexManager::VertexManager(VulkanContext* ctx_, const std::vector<VertexManager::Input>& inputs_)
{
    this->ctx=ctx_;
    this->inputs = inputs_;
    this->pushedToGPU=false;
    this->numVertices=0;
    this->numIndices=0;

    if( inputs.size() < 1 ){
        fatal("Must have at least one vertex input");
    }

    for(unsigned i=0;i<this->inputs.size();++i){
        std::string name = inputs[i].name;
        if( this->nameToIndex.contains(name) )
            fatal("Duplicate input: "+name);
        if( name == "indices" )
            fatal("'indices' is not a valid name");
        this->nameToIndex[name] = i;
        this->vertexData.push_back( std::vector<char>() );
    }

    this->nameToIndex["indices"] = (unsigned)this->vertexData.size();
    //~ this->inputs.push_back(Input{
        //~ .name="indices",
        //~ .format=VK_FORMAT_R32_UINT,
        //~ .location=(unsigned)this->inputs.size(),
        //~ .rate=VK_VERTEX_INPUT_RATE_VERTEX       //ignored
    //~ });
    this->vertexData.push_back( std::vector<char>() );

    unsigned numInputs = (unsigned)inputs.size();

    this->bindings.reserve(numInputs);
    this->attributes.reserve(numInputs);

    for(unsigned i=0;i<inputs.size();++i){
        attributes.push_back(VkVertexInputAttributeDescription{
            .location=inputs[i].location,
            .binding=i,
            .format=inputs[i].format,
            .offset=0
        });
        bindings.push_back(VkVertexInputBindingDescription{
            .binding=i,
            .stride=sizeForFormat(inputs[i].format),
            .inputRate=inputs[i].rate
        });
    }

    this->inputState = VkPipelineVertexInputStateCreateInfo{
        .sType=VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO,
        .pNext=nullptr,
        .flags=0,
        .vertexBindingDescriptionCount = len(bindings),
        .pVertexBindingDescriptions = bindings.data(),
        .vertexAttributeDescriptionCount = len(attributes),
        .pVertexAttributeDescriptions = attributes.data()
    };

}

VertexManager::Info VertexManager::addIndexedDataHelper(std::vector<AttribInfo>& V)
{
    if(this->pushedToGPU){
        fatal(
            "Cannot add data after pushing to GPU"
        );
    }

    //at least one vertex attribute + the indices
    assert( V.size() >= 2 );

    std::set<std::string> supplied;
    for(auto& d : V ){
        if( supplied.find(d.name) != supplied.end() ){
            fatal("Duplicate name: "+d.name);
        }
        supplied.insert(d.name);
    }

    std::set<std::string> expected;
    for(auto& d: this->inputs){
        expected.insert(d.name);
    }
    expected.insert("indices");

    std::vector<std::string> extras;
    std::set_difference(supplied.begin(),supplied.end(),
                        expected.begin(),expected.end(),
                        extras.begin()
    );
    if(!extras.empty()){
        auto tmp = join(", ",extras);
        fatal("Unexpected: "+tmp);
    }

    std::vector<std::string> missing;
    std::set_difference(expected.begin(),expected.end(),supplied.begin(),supplied.end(),missing.begin());
    if(!missing.empty()){
        auto tmp = join(", ",extras);
        fatal("Missing: "+tmp);
    }

    int ni=-1;
    int nv=-1;
    for(AttribInfo& ai : V ){
        int tmp = this->addDataForName(ai);
        if(ai.name == "indices")
            ni=tmp;
        else{
            if(nv == -1)
                nv=tmp;
            else{
                if( nv != tmp ){
                    fatal("Vertex element count mismatch");
                }
            }
        }
    }

    VertexManager::Info info{
        .vertexOffset = this->numVertices,
        .indexOffset = this->numIndices,
        .numIndices = (unsigned)ni
    };

    this->numVertices += nv;
    this->numIndices += ni;
    return info;
}


int VertexManager::addDataForName(AttribInfo& ai)
{
    if( !this->nameToIndex.contains(ai.name) ){
        fatal("Unexpected name: "+ai.name);
    }

    int idx = this->nameToIndex[ai.name];

    //unlike python, we can do strong type checking here
    if( ai.name == "indices" ){
        if( ai.format != VK_FORMAT_R32_UINT )
            fatal("Bad format for "+ai.name);
    } else {
        if( this->inputs[idx].format != ai.format ){
            fatal("Bad format for "+ai.name);
        }
    }

    this->vertexData[idx].insert( this->vertexData[idx].end(),
        ai.ptr, ai.ptr+ai.totalSizeInBytes );

    return ai.numElements;  //unlike Python, we know the element count directly
}

void VertexManager::pushToGPU()
{
    if(this->pushedToGPU)
        fatal("pushToGPU called twice");

    this->pushedToGPU=true;
    for(unsigned i=0;i<this->vertexData.size()-1;++i){
       this->buffers.push_back(new DeviceLocalBuffer(
            this->ctx,
            this->vertexData[i],
            VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
            this->inputs[i].name
        ));
    }

    this->buffers.push_back(new DeviceLocalBuffer(
        this->ctx,
        this->vertexData.back(),    //indices
        VK_BUFFER_USAGE_INDEX_BUFFER_BIT,
        "indices"
    ));

    CleanupManager::registerCleanupFunction(
        [this](){
            for(auto& b : this->buffers){
                b->cleanup();
            }
        }
    );
}

void VertexManager::bindBuffers(VkCommandBuffer cmd)
{
    if(! this->pushedToGPU )
        fatal("Need to pushToGPU() first");

    //minus 1 because we don't count index buffer
    std::vector<VkBuffer> buffers_(this->buffers.size()-1);
    std::vector<VkDeviceSize> offsets(this->buffers.size()-1);

    //skip index buffer
    for(unsigned i=0;i<this->buffers.size()-1;++i){
        buffers_[i] = this->buffers[i]->buffer;
        offsets[i] = 0;
    }

    vkCmdBindVertexBuffers(
        cmd,
        0,      //first binding
        (unsigned) buffers_.size(),
        buffers_.data(),
        offsets.data()
    );

    vkCmdBindIndexBuffer(
        cmd,
        this->buffers.back()->buffer,
        0,      //offset
        VK_INDEX_TYPE_UINT32
    );
}


void VertexManager::aai(std::vector<VertexManager::AttribInfo>& V, const std::string& name, const void* p, std::size_t ne, std::size_t es, VkFormat fmt)
{
    V.push_back( AttribInfo{
        .name=name,
        .ptr=(char*)p,
        .numElements=(unsigned)ne,
        .totalSizeInBytes=(unsigned)(es*ne),
        .format=fmt
    });
}

void VertexManager::addAttribInfo(std::vector<AttribInfo>& V, const std::string& name, const std::vector<float>& value)
{
    aai(V,name,value.data(),value.size(),sizeof(value[0]),VK_FORMAT_R32_SFLOAT);
}

void VertexManager::addAttribInfo(std::vector<AttribInfo>& V, const std::string& name, const std::vector<math2801::vec2>& value)
{
    aai(V,name,value.data(),value.size(),sizeof(value[0]),VK_FORMAT_R32G32_SFLOAT);
}

void VertexManager::addAttribInfo(std::vector<AttribInfo>& V, const std::string& name, const std::vector<math2801::vec3>& value)
{
    aai(V,name,value.data(),value.size(),sizeof(value[0]),VK_FORMAT_R32G32B32_SFLOAT);
}

void VertexManager::addAttribInfo(std::vector<AttribInfo>& V, const std::string& name, const std::vector<math2801::vec4>& value)
{
    aai(V,name,value.data(),value.size(),sizeof(value[0]),VK_FORMAT_R32G32B32A32_SFLOAT);
}

void VertexManager::addAttribInfo(std::vector<AttribInfo>& V, const std::string& name, const std::vector<std::int32_t>& value)
{
    aai(V,name,value.data(),value.size(),sizeof(value[0]),VK_FORMAT_R32_SINT);
}

void VertexManager::addAttribInfo(std::vector<AttribInfo>& V, const std::string& name, const std::vector<math2801::ivec2>& value)
{
    aai(V,name,value.data(),value.size(),sizeof(value[0]),VK_FORMAT_R32G32_SINT);
}

void VertexManager::addAttribInfo(std::vector<AttribInfo>& V, const std::string& name, const std::vector<math2801::ivec3>& value)
{
    aai(V,name,value.data(),value.size(),sizeof(value[0]),VK_FORMAT_R32G32B32_SINT);
}

void VertexManager::addAttribInfo(std::vector<AttribInfo>& V, const std::string& name, const std::vector<math2801::ivec4>& value)
{
    aai(V,name,value.data(),value.size(),sizeof(value[0]),VK_FORMAT_R32G32B32A32_SINT);
}

void VertexManager::addAttribInfo(std::vector<AttribInfo>& V, const std::string& name, const std::vector<std::uint32_t>& value)
{
    aai(V,name,value.data(),value.size(),sizeof(value[0]),VK_FORMAT_R32_UINT);
}

void VertexManager::addAttribInfo(std::vector<AttribInfo>& V, const std::string& name, const std::vector<math2801::uvec2>& value)
{
    aai(V,name,value.data(),value.size(),sizeof(value[0]),VK_FORMAT_R32G32_UINT);
}

void VertexManager::addAttribInfo(std::vector<AttribInfo>& V, const std::string& name, const std::vector<math2801::uvec3>& value)
{
    aai(V,name,value.data(),value.size(),sizeof(value[0]),VK_FORMAT_R32G32B32_UINT);
}

void VertexManager::addAttribInfo(std::vector<AttribInfo>& V, const std::string& name, const std::vector<math2801::uvec4>& value)
{
    aai(V,name,value.data(),value.size(),sizeof(value[0]),VK_FORMAT_R32G32B32A32_UINT);
}





void VertexManager::addAttribInfo(std::vector<AttribInfo>& V, const std::string& name, const std::span<float>& value)
{
    aai(V,name,value.data(),value.size(),sizeof(value[0]),VK_FORMAT_R32_SFLOAT);
}

void VertexManager::addAttribInfo(std::vector<AttribInfo>& V, const std::string& name, const std::span<math2801::vec2>& value)
{
    aai(V,name,value.data(),value.size(),sizeof(value[0]),VK_FORMAT_R32G32_SFLOAT);
}

void VertexManager::addAttribInfo(std::vector<AttribInfo>& V, const std::string& name, const std::span<math2801::vec3>& value)
{
    aai(V,name,value.data(),value.size(),sizeof(value[0]),VK_FORMAT_R32G32B32_SFLOAT);
}

void VertexManager::addAttribInfo(std::vector<AttribInfo>& V, const std::string& name, const std::span<math2801::vec4>& value)
{
    aai(V,name,value.data(),value.size(),sizeof(value[0]),VK_FORMAT_R32G32B32A32_SFLOAT);
}

void VertexManager::addAttribInfo(std::vector<AttribInfo>& V, const std::string& name, const std::span<std::int32_t>& value)
{
    aai(V,name,value.data(),value.size(),sizeof(value[0]),VK_FORMAT_R32_SINT);
}

void VertexManager::addAttribInfo(std::vector<AttribInfo>& V, const std::string& name, const std::span<math2801::ivec2>& value)
{
    aai(V,name,value.data(),value.size(),sizeof(value[0]),VK_FORMAT_R32G32_SINT);
}

void VertexManager::addAttribInfo(std::vector<AttribInfo>& V, const std::string& name, const std::span<math2801::ivec3>& value)
{
    aai(V,name,value.data(),value.size(),sizeof(value[0]),VK_FORMAT_R32G32B32_SINT);
}

void VertexManager::addAttribInfo(std::vector<AttribInfo>& V, const std::string& name, const std::span<math2801::ivec4>& value)
{
    aai(V,name,value.data(),value.size(),sizeof(value[0]),VK_FORMAT_R32G32B32A32_SINT);
}

void VertexManager::addAttribInfo(std::vector<AttribInfo>& V, const std::string& name, const std::span<std::uint32_t>& value)
{
    aai(V,name,value.data(),value.size(),sizeof(value[0]),VK_FORMAT_R32_UINT);
}

void VertexManager::addAttribInfo(std::vector<AttribInfo>& V, const std::string& name, const std::span<math2801::uvec2>& value)
{
    aai(V,name,value.data(),value.size(),sizeof(value[0]),VK_FORMAT_R32G32_UINT);
}

void VertexManager::addAttribInfo(std::vector<AttribInfo>& V, const std::string& name, const std::span<math2801::uvec3>& value)
{
    aai(V,name,value.data(),value.size(),sizeof(value[0]),VK_FORMAT_R32G32B32_UINT);
}

void VertexManager::addAttribInfo(std::vector<AttribInfo>& V, const std::string& name, const std::span<math2801::uvec4>& value)
{
    aai(V,name,value.data(),value.size(),sizeof(value[0]),VK_FORMAT_R32G32B32A32_UINT);
}
