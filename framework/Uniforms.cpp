#include "Uniforms.h"
#include "Descriptors.h"
#include "CleanupManager.h"
#include "Buffers.h"
#include "consoleoutput.h"
#include "utils.h"
#include <cassert>
#include <cstring>
#include <array>
#include "mischelpers.h"
#include "CommandBuffer.h"
#include <sstream>

template<typename T>
static void _set(
    std::map<std::string,parseMembers::Item>& items,
    std::vector<char>& shadowBuffer,
    const std::string& name, const T& value,
    std::set<std::string>& uninitialized)
{
    auto tmp = items.find(name);
    if( tmp == items.end() ){
        std::ostringstream oss;
        oss << "No such uniform '" << name << "'\n";
        oss << "Known uniforms:\n";
        for(auto it : items ){
            oss << it.second.name << " (" <<
                it.second.typeAsString;
            if( !it.second.arraySizeAsString.empty() )
                oss << " " << it.second.arraySizeAsString;
            oss << ")\n";
        }
        error(oss.str());
        fatal("No such uniform '"+name+"'");
    }
    auto& info = tmp->second;
    std::vector<char> convertedValue = info.convert(value);
    std::memcpy( shadowBuffer.data()+info.offset, convertedValue.data(), info.byteSize );
    uninitialized.erase(name);
}

//~ Uniforms::Uniforms(VulkanContext* ctx, PerFrameDescriptorSet* perFrameDescriptorSet, int slotNumber, std::string uniformFile) :
    //~ Uniforms(ctx,std::vector<PerFrameDescriptorSet*>{perFrameDescriptorSet},slotNumber,uniformFile)
//~ {}

Uniforms::Uniforms(VulkanContext* ctx_, int slot_, std::string uniformFile, std::string name_)
{
    this->name=name_;
    this->ctx=ctx_;
    this->slot=slot_;

    auto info = parseMembers::parse(uniformFile);
    this->byteSize=info.byteSize;
    this->items = info.variables;
    this->defines = info.defines;

    verbose("Uniforms: Discovered these uniforms:");
    for(const auto& it : this->items){
        std::string uname = it.first;
        this->uninitialized.insert(uname);
        verbose(it.second.typeAsString,it.second.arraySizeAsString,it.second.name);
    }

    verbose("Uniforms: Original byteSize is",this->byteSize);

    //create a dummy so we can get the memory requirements
    DeviceLocalBuffer* buff = new DeviceLocalBuffer(
        this->ctx, nullptr, this->byteSize,
        VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
        "dummy for uniforms"
    );
    this->memoryTypeBits = buff->memoryRequirements.memoryTypeBits;

    //ensure byteSize is a multiple of alignment
    VkDeviceSize extra = this->byteSize % buff->memoryRequirements.alignment;
    this->byteSize += (buff->memoryRequirements.alignment - extra );


    verbose("Uniforms: Alignment is:",buff->memoryRequirements.alignment,"size:",buff->memoryRequirements.size);
    verbose("Uniforms: Adjusted byteSize to",this->byteSize);

    buff->cleanup();
    buff=nullptr;


    this->shadowBuffer.resize(this->byteSize);

    utils::registerFrameCompleteCallback( [this](int frameNumber){
        this->_frameCompleteCallback(frameNumber);
    });

    utils::registerFrameEndCallback( [this](int){
        this->_frameEndCallback();
    });

    CleanupManager::registerCleanupFunction( [this](){
        this->_cleanupFunc();
    });

    utils::registerFrameEndCallback( [this](int ){
        this->_update();
    });

}


void Uniforms::set(const std::string& name_, std::int32_t value){    _set(this->items,this->shadowBuffer,name_,value,this->uninitialized ); }
void Uniforms::set(const std::string& name_, std::uint32_t value){   _set(this->items,this->shadowBuffer,name_,value,this->uninitialized ); }
void Uniforms::set(const std::string& name_, float value){           _set(this->items,this->shadowBuffer,name_,value,this->uninitialized ); }
void Uniforms::set(const std::string& name_, math2801::ivec2 value){ _set(this->items,this->shadowBuffer,name_,value,this->uninitialized ); }
void Uniforms::set(const std::string& name_, math2801::vec2 value){  _set(this->items,this->shadowBuffer,name_,value,this->uninitialized ); }
void Uniforms::set(const std::string& name_, math2801::vec3 value){  _set(this->items,this->shadowBuffer,name_,value,this->uninitialized ); }
void Uniforms::set(const std::string& name_, math2801::vec4 value){  _set(this->items,this->shadowBuffer,name_,value,this->uninitialized ); }
void Uniforms::set(const std::string& name_, math2801::mat4 value){  _set(this->items,this->shadowBuffer,name_,value,this->uninitialized ); }
void Uniforms::set(const std::string& name_, const std::vector<math2801::vec4>& value){
    _set(this->items,this->shadowBuffer,name_,value,this->uninitialized );
}
void Uniforms::set(const std::string& name_, const std::vector<math2801::uvec4>& value){
    _set(this->items,this->shadowBuffer,name_,value,this->uninitialized );
}
void Uniforms::set(const std::string& name_, const std::vector<math2801::ivec4>& value){
    _set(this->items,this->shadowBuffer,name_,value,this->uninitialized );
}
void Uniforms::set(const std::string& name_, const std::vector<math2801::mat4>& value){
    _set(this->items,this->shadowBuffer,name_,value,this->uninitialized );
}
void Uniforms::set(const std::string& name_, double value){
    this->set(name_,(float)value);
}


DeviceLocalBuffer* Uniforms::getBufferForSetSlot()
{
    int frameID = utils::getCurrentFrameIdentifier();
    DeviceLocalBuffer* B = this->_getBuffer(frameID);
    this->needToBePushed.push_back( std::make_pair(B,this->shadowBuffer) ); //copies shadowBuffer
    return B;
}

void Uniforms::bind( VkCommandBuffer cmd, DescriptorSet* descriptorSet, int slot_)
{
    if( slot_ == -1 )
        slot_ = this->slot;
    DeviceLocalBuffer* B = this->getBufferForSetSlot();
    descriptorSet->setSlot(slot_,B->buffer);
    descriptorSet->bind(cmd);
}

void Uniforms::_update()
{
    if(!(this->uninitialized.empty() )){
        std::string s;
        if(this->uninitialized.size() == 1)
            s = "";
        else
            s = "s";
        warn("Uninitialized uniform"+s+" for "+this->name+": "+ join(",", this->uninitialized));
    }

    if(this->needToBePushed.empty() )
        return;


    VkCommandBufferBeginInfo binfo{
        .sType=VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
        .pNext=nullptr,
        .flags=0,
        .pInheritanceInfo=nullptr
    };

    auto cmd = CommandBuffer::allocate();
    vkBeginCommandBuffer(cmd,&binfo);

    VkAccessFlags pipeStages = (
        VK_PIPELINE_STAGE_DRAW_INDIRECT_BIT |
        VK_PIPELINE_STAGE_VERTEX_INPUT_BIT |
        VK_PIPELINE_STAGE_VERTEX_SHADER_BIT |
	VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT |   //new
        VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT |
        VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT |
        VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT |
        VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT |
        VK_PIPELINE_STAGE_TRANSFER_BIT |
        VK_PIPELINE_STAGE_HOST_BIT
    );

    if( this->ctx->hasGeometryShader )
        pipeStages |= VK_PIPELINE_STAGE_GEOMETRY_SHADER_BIT;
    if(this->ctx->hasTessellationShader){
        pipeStages |= VK_PIPELINE_STAGE_TESSELLATION_CONTROL_SHADER_BIT;
        pipeStages |= VK_PIPELINE_STAGE_TESSELLATION_EVALUATION_SHADER_BIT;
    }


    VkMemoryBarrier mbarrier{
        .sType=VK_STRUCTURE_TYPE_MEMORY_BARRIER,
        .pNext=nullptr,
        .srcAccessMask=pipeStages,
        .dstAccessMask=pipeStages
    };

    vkCmdPipelineBarrier(
        cmd,
        pipeStages,
        pipeStages,
        0,
        1, &(mbarrier),   //global memory barriers
        0,nullptr,     //buffer barriers
        0,nullptr      //image barriers
    );

    for(auto pr : this->needToBePushed){
        DeviceLocalBuffer* bufferobj = pr.first;
        std::vector<char>& shadowBuffer_ = pr.second;
        vkCmdUpdateBuffer( cmd, bufferobj->buffer,
            0, shadowBuffer_.size(), shadowBuffer_.data()
        );
    }

    vkCmdPipelineBarrier(
        cmd,
        pipeStages,
        pipeStages,
        0,
        1, &(mbarrier),   //global memory barriers
        0,nullptr,     //buffer barriers
        0,nullptr      //image barriers
    );

    vkEndCommandBuffer(cmd);

    this->needToBePushed.clear();

    utils::addCommandBufferPreSubmit(cmd);
}

void Uniforms::_frameEndCallback()
{
    if( !this->needToBePushed.empty() )
        this->_update();
}


int Uniforms::getDefine(const std::string& name_)
{
    if( this->defines.contains(name_) )
        return this->defines[name_];
    else
        fatal("No such #define constant '"+name_+"' in uniforms");
}

bool Uniforms::hasDefine(const std::string& name_)
{
    return this->defines.contains(name_);
}







DeviceLocalBuffer* Uniforms::_getBuffer(int frameID)
{
    /**
    Return a buffer that can be used for uniform data.
    The buffer's contents are not initialized, and the buffer is added
    to activeBuffers.
    First, try to retrieve a buffer from availableBuffers.
    If there is no buffer in availableBuffers,
    allocate a new chunk of memory and allocate buffers from there.
    The allocated buffer is removed from availableBuffers before this function returns
    */

    //first, try to take a buffer from availableBuffers.
    //if there isn't one, need to manufacture another buffer
    if( this->availableBuffers.empty() ){
        //if there's no free memory, allocate some
        if( this->availableMemory.empty() || this->availableMemory.back() < this->byteSize ){

            auto numBuffers=int((4*1024*1024)/this->byteSize);
            if( numBuffers == 0 )
                fatal("Size of uniforms is too large");

            auto bufferBytes = numBuffers * this->byteSize;

            VkDeviceMemory mem = DeviceLocalBuffer::allocateMemory(
                this->ctx,
                this->memoryTypeBits,
                bufferBytes,
                "memory for uniforms");
            this->memories.push_back(mem);
            this->availableMemory.push_back(bufferBytes);
            this->offsets.push_back(0);
            verbose("Allocated memory for uniforms:",bufferBytes,"; each uniform buffer is",this->byteSize,"bytes");
        }

        //create new buffer
        this->availableBuffers.push_back( new DeviceLocalBuffer(
            this->ctx,
            this->memories.back(),
            this->offsets.back(),
            this->byteSize,
            VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
            "uniform buffer")
        );
        this->offsets.back() += this->byteSize;
        this->availableMemory.back() -= this->byteSize;
        verbose("Allocated new uniform buffer");
    } //end if availableBuffers is empty

    DeviceLocalBuffer* B = this->availableBuffers.back();
    this->availableBuffers.pop_back();
    this->activeBuffers[frameID].push_back(B);
    return B;
}

void Uniforms::_cleanupFunc()
{
    for(DeviceLocalBuffer* abuff : this->availableBuffers){
        abuff->cleanup();
    }
    for(auto& it: this->activeBuffers ){
        //~ int frameID = it.first;
        for(DeviceLocalBuffer* b : it.second ){
            b->cleanup();
        }
    }
    for(VkDeviceMemory mem : this->memories ){
        vkFreeMemory(this->ctx->dev,mem,nullptr);
    }
}

void Uniforms::_frameCompleteCallback(int frameNumber)
{
    if( this->activeBuffers.contains(frameNumber)){
        for(DeviceLocalBuffer* b : this->activeBuffers[frameNumber] ){
            this->availableBuffers.push_back(b);
        }
        this->activeBuffers.erase(frameNumber);
    }
}

void Uniforms::_noSuchUniform(std::string name_)
{
    error("No such uniform '"+name_+"'");
    error("Known uniforms:");
    for(auto it : this-> items ){
        error(it.second.name,"(",
            it.second.typeAsString+
            it.second.arraySizeAsString,
            ") "
        );
    }
    fatal("No such uniform '"+name_+"'");
}
