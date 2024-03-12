#include "Buffers.h"
#include "CommandBuffer.h"
#include <cstring>
#include <assert.h>
#include <stdexcept>
#include "CleanupManager.h"
#include "consoleoutput.h"
#include "mischelpers.h"

Buffer::Buffer(VulkanContext* ctx_, VkBufferUsageFlags usage, VkDeviceSize size, const std::string& name_)
{
    assert( ctx_->dev != nullptr );

    this->ctx=ctx_;
    this->name=name_;
    if( size == 0 ){
        error("Buffer size is zero");
    }
    check(vkCreateBuffer(
        ctx->dev,
        VkBufferCreateInfo{
            .sType=VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
            .pNext=nullptr,
            .flags=0,
            .size = (unsigned)size,
            .usage = usage,
            .sharingMode = VK_SHARING_MODE_EXCLUSIVE,
            .queueFamilyIndexCount=0,
            .pQueueFamilyIndices=nullptr,
        },
        nullptr,
        &(this->buffer)
    ));
    this->byteSize=size;
    vkGetBufferMemoryRequirements( this->ctx->dev, this->buffer, &(this->memoryRequirements));
    ctx->setObjectName(this->buffer,name_);
}

void Buffer::memoryBarrier(VkCommandBuffer cmd)
{
    Buffer::memoryBarrier(cmd,this->buffer);
}

void Buffer::bindMemory(VkDeviceMemory memory_, VkDeviceSize offset, bool ownsMemory_)
{

    this->memory = memory_;
    this->ownsMemory = ownsMemory_;

    //safety check
    if(offset % this->memoryRequirements.alignment != 0)
        fatal("Buffer alignment error");
    vkBindBufferMemory(ctx->dev,this->buffer,memory,offset);
}

VkBufferView Buffer::makeView(VkFormat format, std::string name_)
{
    VkBufferViewCreateInfo cinfo{
        .sType = VK_STRUCTURE_TYPE_BUFFER_VIEW_CREATE_INFO,
        .pNext = nullptr,
        .flags = 0,
        .buffer= this->buffer,
        .format= format,
        .offset= 0,
        .range = VK_WHOLE_SIZE
    };
    VkBufferView v;
    check(
        vkCreateBufferView(
            this->ctx->dev,
            &cinfo,
            nullptr,    //allocators
            &v
        )
    );
    this->ctx->setObjectName(v,name_);
    return v;
}


void Buffer::cleanup()
{
    vkDestroyBuffer(ctx->dev,this->buffer,nullptr);
    if( this->ownsMemory ){
        verbose( "Free memory for buffer " + this->name );
        vkFreeMemory( ctx->dev, this->memory, nullptr);
    }
}

void Buffer::copyTo(
    std::uint64_t srcOffset, std::uint64_t srcSize,
    Buffer* otherBuffer, std::uint64_t otherOffset,
    VkCommandBuffer cmd_
){

    bool own=false;
    VkCommandBuffer cmd;
    if( cmd_ == VK_NULL_HANDLE ){
        own=true;
        cmd = CommandBuffer::beginImmediateCommands();
    } else {
        cmd = cmd_;
    }

    vkCmdCopyBuffer(
        cmd,
        this->buffer,
        otherBuffer->buffer,
        1,
        VkBufferCopy{
            .srcOffset=srcOffset,
            .dstOffset=otherOffset,
            .size=srcSize
        }
    );

    Buffer::memoryBarrier(cmd,otherBuffer->buffer);

    if(own)
        CommandBuffer::endImmediateCommands(cmd);

}

void Buffer::copyTo(Buffer* otherBuffer,VkCommandBuffer cmd)
{
    this->copyTo(0,this->byteSize, otherBuffer, 0, cmd );
}


StagingBuffer::StagingBuffer(VulkanContext* ctx_,
            VkBufferUsageFlags usage,
            const void* initialData,
            VkDeviceSize size, const std::string& name_):
    Buffer(
        ctx_,
        usage, //VK_BUFFER_USAGE_TRANSFER_SRC_BIT|VK_BUFFER_USAGE_TRANSFER_DST_BIT,
        size,
        name_
    )
{
    //get memory requirements for the buffer
    //This tells us three things:
    //The size, alignment, and memory types
    //If bit i of memoryTypeBits is set,
    //then type i is OK to use

    //several types of allocations can be tried
    std::vector<VkMemoryPropertyFlags> propertiesToAttempt({
        VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT | VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
        VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
        VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT
    });

    this->memory = VK_NULL_HANDLE;
    VkMemoryPropertyFlags properties;
    for(auto props : propertiesToAttempt){
        properties=props;
        this->memory = Buffer::allocateMemory(
            ctx,
            this->memoryRequirements.memoryTypeBits,
            this->memoryRequirements.size,
            props,
            true,
            "Memory for buffer{"+name+"}"
        );
        if( this->memory != VK_NULL_HANDLE )
            break;
        verbose("Staging buffer: Could not allocate with properties=",properties);
    }

    if( this->memory == VK_NULL_HANDLE ){
        fatal("Could not allocate memory");
    }

    this->isCoherent = ( ( properties & VK_MEMORY_PROPERTY_HOST_COHERENT_BIT ) ? true:false );
    bool isDeviceLocal = ( ( properties & VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT ) ? true:false );

    verbose("Staging buffer: DeviceLocal=",isDeviceLocal,"Coherent=",this->isCoherent);

    this->bindMemory(this->memory,0,true);

    if(initialData != nullptr){
        auto p = this->map();
        std::memcpy( p, initialData, size );
        this->unmap();

        //https://stackoverflow.com/questions/48667439/should-i-syncronize-an-access-to-a-memory-with-host-visible-bit-host-coherent
        auto cmd = CommandBuffer::beginImmediateCommands();
        Buffer::memoryBarrier(cmd,this->buffer);
        CommandBuffer::endImmediateCommands(cmd);
    }
}


void* StagingBuffer::map()
{
    void* p;
    check( vkMapMemory(
        ctx->dev,
        this->memory,
        0,                  //offset
        this->byteSize,
        0,                  //flags
        &(p)
    ));

    if(!this->isCoherent){
        VkMappedMemoryRange mmr{
            .sType=VK_STRUCTURE_TYPE_MAPPED_MEMORY_RANGE,
            .pNext=nullptr,
            .memory=this->memory,
            .offset=0,
            .size=VK_WHOLE_SIZE
        };
        vkInvalidateMappedMemoryRanges(
            this->ctx->dev,
            1,
            &mmr
        );
    }

    return p;
}

void StagingBuffer::unmap()
{
    vkUnmapMemory(
        ctx->dev,
        this->memory
    );

    if(!this->isCoherent){
        VkMappedMemoryRange mmr{
            .sType=VK_STRUCTURE_TYPE_MAPPED_MEMORY_RANGE,
            .pNext=nullptr,
            .memory=this->memory,
            .offset=0,
            .size=VK_WHOLE_SIZE
        };
        vkFlushMappedMemoryRanges(
            ctx->dev,
            1,
            &mmr
        );
    }


}


void StagingBuffer::cleanup()
{
    Buffer::cleanup();
}

DeviceLocalBuffer::DeviceLocalBuffer( VulkanContext* ctx_,
                    const void* initialData,VkDeviceSize size,
                    VkBufferUsageFlags usage, const std::string& name_)
    : Buffer(ctx_,
             usage|VK_BUFFER_USAGE_TRANSFER_SRC_BIT|VK_BUFFER_USAGE_TRANSFER_DST_BIT,
             size,
             name_)
{

    this->memory = DeviceLocalBuffer::allocateMemory(this->ctx,
        this->memoryRequirements.memoryTypeBits,
        std::max(this->memoryRequirements.size,size), "memory for "+name);
    this->bindMemory(this->memory,0,true);

    if(initialData != nullptr ){
        StagingBuffer* staging = new StagingBuffer(ctx,
            VK_BUFFER_USAGE_TRANSFER_SRC_BIT|VK_BUFFER_USAGE_TRANSFER_DST_BIT,
            initialData, size, "temporary staging buffer for "+name );
        staging->copyTo(this);
        staging->cleanup();
        delete staging;
    }

}


DeviceLocalBuffer::DeviceLocalBuffer( VulkanContext* ctx_, VkDeviceMemory memory_, VkDeviceSize offset,
                    VkDeviceSize size, VkBufferUsageFlags usage, const std::string& name_)
    : Buffer(
             ctx_,
             usage|VK_BUFFER_USAGE_TRANSFER_SRC_BIT|VK_BUFFER_USAGE_TRANSFER_DST_BIT,
             size,
             name_)
{
    this->bindMemory(memory_,offset,false);
}

VkDeviceMemory DeviceLocalBuffer::allocateMemory(VulkanContext* ctx,
        std::uint32_t memoryTypeBits, VkDeviceSize size, std::string name)
{
    auto properties = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;

    //memory must be device local
    return Buffer::allocateMemory(ctx,memoryTypeBits,size,properties,false,name);
}

void DeviceLocalBuffer::cleanup()
{
    Buffer::cleanup();
}


VkDeviceMemory Buffer::allocateMemory(
    VulkanContext* ctx,
    std::uint32_t memoryTypeBits,
    VkDeviceSize size,
    VkMemoryPropertyFlags properties,
    bool failureOK,
    const std::string& name)
{

    verbose("Allocate buffer memory (name=" +name+ "): " + std::to_string(size) + " bytes");

    //properties is usually either HOST_VISIBLE | HOST_COHERENT
    //           or else it's DEVICE_LOCAL

    //Get properties of the various memory types the GPU has
    //This returns a list of memory types. Each memory type
    //has propertyFlags and a heapIndex. The propertyFlags
    //is a bitwise OR of DEVICE_LOCAL_BIT, HOST_VISIBLE_BIT, etc.
    VkPhysicalDeviceMemoryProperties memprops;
    vkGetPhysicalDeviceMemoryProperties(
        ctx->physdev,
        &(memprops)
    );

    bool ok=false;

    //look at each memory type...
    unsigned i;
    for(i=0;i<memprops.memoryTypeCount;++i){

        //if this is not one of the types that
        //the buffer permits, then skip it
        if( !( (1<<i) & memoryTypeBits ) )
            continue;

        //if type i meets all the requested property bits,
        //we can choose it
        auto masked = properties & memprops.memoryTypes[i].propertyFlags;
        if(properties == masked){
            ok=true;
            break;
        }
    }

    if(!ok){
        if( failureOK ){
            return VK_NULL_HANDLE;
        } else {
            fatal("No memory with desired type & properties");
        }
    }


    VkDeviceMemory memory;
    VkResult res = vkAllocateMemory(
        ctx->dev,
        VkMemoryAllocateInfo{
            .sType=VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO,
            .pNext=nullptr,
            .allocationSize=size,
            .memoryTypeIndex=i
        },
        nullptr,
        &(memory)
    );

    if( res == VK_ERROR_OUT_OF_HOST_MEMORY || res == VK_ERROR_OUT_OF_DEVICE_MEMORY ){
        if( failureOK ){
            return VK_NULL_HANDLE;
        } else {
            fatal("Out of memory");
        }
    }

    ctx->setObjectName(memory,name);
    return memory;
}


void Buffer::memoryBarrier(VkCommandBuffer cmd, VkBuffer buffer)
{
    vkCmdPipelineBarrier(
        cmd,
        VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT,      //source stage mask
        VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT,      //destination stage mask
        0,      //dependency flags
        1,
        VkMemoryBarrier{
            .sType=VK_STRUCTURE_TYPE_MEMORY_BARRIER,
            .pNext=nullptr,
            .srcAccessMask=VK_ACCESS_MEMORY_READ_BIT|VK_ACCESS_MEMORY_WRITE_BIT,
            .dstAccessMask=VK_ACCESS_MEMORY_READ_BIT|VK_ACCESS_MEMORY_WRITE_BIT
        },
        1,

        VkBufferMemoryBarrier{
            .sType=VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER,
            .pNext=nullptr,
            .srcAccessMask=VK_ACCESS_MEMORY_READ_BIT|VK_ACCESS_MEMORY_WRITE_BIT,
            .dstAccessMask=VK_ACCESS_MEMORY_READ_BIT|VK_ACCESS_MEMORY_WRITE_BIT,
            .srcQueueFamilyIndex=VK_QUEUE_FAMILY_IGNORED,
            .dstQueueFamilyIndex=VK_QUEUE_FAMILY_IGNORED,
            .buffer=buffer,
            .offset=0,
            .size=VK_WHOLE_SIZE
        },
        0,      //image memory barriers
        VkImageMemoryBarrier{}
    );
}

void DeviceLocalBuffer::copyToCPUMemory(void* ptr, VkDeviceSize offset, VkDeviceSize count)
{
    if( DeviceLocalBuffer::copyHelper == nullptr ){
        DeviceLocalBuffer::makeCopyHelper();
    }
    char* destp = (char*) ptr;

    while(count > 0 ){
        VkDeviceSize numToCopy = std::min( count, DeviceLocalBuffer::copyHelper->byteSize);
        this->copyTo(
            offset, numToCopy,
            DeviceLocalBuffer::copyHelper,
            0
        );
        void* m = DeviceLocalBuffer::copyHelper->map();
        std::memcpy(destp, m, numToCopy );
        DeviceLocalBuffer::copyHelper->unmap();
        destp += numToCopy;
        count -= numToCopy;
        offset += numToCopy;
    }
}


void DeviceLocalBuffer::copyFromCPUMemory(void* ptr, VkDeviceSize offset, VkDeviceSize count)
{
    if( DeviceLocalBuffer::copyHelper == nullptr ){
        DeviceLocalBuffer::makeCopyHelper();
    }

    char* srcp = (char*) ptr;

    while(count > 0 ){

        VkDeviceSize numToCopy = std::min( VkDeviceSize(count), DeviceLocalBuffer::copyHelper->byteSize);

        void* m = DeviceLocalBuffer::copyHelper->map();
        std::memcpy(m, srcp, numToCopy );
        DeviceLocalBuffer::copyHelper->unmap();

        DeviceLocalBuffer::copyHelper->copyTo(
            0, numToCopy,
            this,
            offset );

        srcp += numToCopy;
        count -= numToCopy;
        offset += numToCopy;
    }
}


void DeviceLocalBuffer::makeCopyHelper()
{
    DeviceLocalBuffer::copyHelper = new StagingBuffer(this->ctx,
        VK_BUFFER_USAGE_TRANSFER_SRC_BIT|
        VK_BUFFER_USAGE_TRANSFER_DST_BIT,
        nullptr,
        16*1024*1024,
        "copyHelper"
    );
    CleanupManager::registerCleanupFunction( [] () {
        DeviceLocalBuffer::copyHelper->cleanup();
    });

}

StagingBuffer* DeviceLocalBuffer::copyHelper;
