#include "ImageManager.h"
#include "Buffers.h"
#include "Images.h"
#include "utils.h"
#include "CleanupManager.h"
#include <cassert>
#include <fstream>
#include <sstream>
#include <iostream>
#include <array>
#include "mischelpers.h"

static std::vector<std::function<void(void)> > callbacks;       //to be called when pushToGPU complete
static std::map<std::string, Image*> _imgmap;
static VulkanContext* ctx;



bool ImageManager::initialized()
{
    return ctx != nullptr;
}

void ImageManager::initialize(VulkanContext* ctx_){
    if(initialized())
        return;
    ctx=ctx_;
}

Image* ImageManager::load(std::string filename)
{
    auto it = _imgmap.find(filename);
    if(it!=_imgmap.end()){
        if( it->second->format != VK_FORMAT_R8G8B8A8_UNORM ||
            it->second->viewType != VK_IMAGE_VIEW_TYPE_2D_ARRAY){
                fatal("Two images have the same name but different formats: "+filename);
        }
        return it->second;
    }

    std::vector<char> pix = utils::readFile(filename);
    auto tmp = imagedecode::decode(pix);
    int width = std::get<0>(tmp);
    int height = std::get<1>(tmp);
    if( std::get<2>(tmp) != "RGBA8" ){
        fatal("Bad image format: Got "+std::get<2>(tmp)+" but expected RGBA8");
    }

    std::vector< std::span<const char> > layerData{
        { std::get<3>(tmp).data(), std::get<3>(tmp).data()+std::get<3>(tmp).size()}
    };

    Image* img = new Image(
        ctx,
        width,height,(int)layerData.size(),
        VK_FORMAT_R8G8B8A8_UNORM,
        VK_IMAGE_USAGE_TRANSFER_DST_BIT|VK_IMAGE_USAGE_SAMPLED_BIT,
        0,      //flags
        VK_IMAGE_VIEW_TYPE_2D_ARRAY,
        VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
        VK_IMAGE_ASPECT_COLOR_BIT,
        layerData,
        nullptr,-1,
        filename
    );


    _imgmap[filename]=img;
    //~ _offset = img->startingOffset+img->byteSize;
    return img;
}




Image* ImageManager::loadLayers(std::vector<std::string> filenames, std::string name)
{
    /**
    """Load multiple images into a multi-layered texture image.
        Caching is not used.
    """
    */
    if(!ctx)
        fatal("ImageManager.initialize() has not been called");

    if( _imgmap.contains(name)){
        Image* it = _imgmap[name];
        //#FIXME: Might need to change this logic...
        if( it->format != VK_FORMAT_R8G8B8A8_UNORM ||
            it->viewType != VK_IMAGE_VIEW_TYPE_2D_ARRAY){
                fatal("Two images have the same name but different formats");
        }
        return it;
    }

    if( len(filenames) == 1 )
        return ImageManager::load(filenames[0]);

    assert(len(filenames) > 0);

    int width=-1;
    int height=-1;
    std::vector<std::vector<char>> layerData(filenames.size());

    for(unsigned fi=0;fi<filenames.size();++fi){
        std::string filename = filenames[fi];
        std::vector<char> pix = utils::readFile(filename);
        auto tmp = imagedecode::decode(pix);
        int w = std::get<0>(tmp);
        int h = std::get<1>(tmp);
        std::string fmt = std::get<2>(tmp);
        std::vector<char> decoded;
        decoded.swap(std::get<3>(tmp));

        if(width == -1 ){
            width=w;
            height=h;
        } else {
            if( w != width or h != height ){
                fatal("Image size mismatch: "+filename+" is "+str(w)+"x"+str(h)+" but "+
                    filenames[0]+" is "+str(width)+"x"+str(height) );
            }
        }
        if(fmt != "RGBA8"){
            fatal(filename+": Expected RGBA8 image, but it is "+str(fmt));
        }
        layerData[fi].swap(decoded);
    }

    std::vector<std::span<const char> > S;
    S.reserve(layerData.size());
    for(auto& ld : layerData ){
        S.push_back(std::span<const char>(ld));
    }

    Image* img = new Image(
        ctx,
        width,height,len(layerData),
        VK_FORMAT_R8G8B8A8_UNORM,
        VK_IMAGE_USAGE_TRANSFER_DST_BIT|VK_IMAGE_USAGE_SAMPLED_BIT,
        0,      //flags
        VK_IMAGE_VIEW_TYPE_2D_ARRAY,
        VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
        VK_IMAGE_ASPECT_COLOR_BIT,
        S,
        nullptr,-1,
        name
    );

    _imgmap[name]=img;
    return img;
}

Image* ImageManager::load16(std::string filename)
{
    auto it = _imgmap.find(filename);
    if(it!=_imgmap.end()){
        if( it->second->format != VK_FORMAT_R16G16B16A16_UNORM ||
            it->second->viewType != VK_IMAGE_VIEW_TYPE_2D_ARRAY){
                fatal("Two images have the same name but different formats: "+filename);
        }
        return it->second;
    }

    std::vector<char> pix = utils::readFile(filename);
    auto tmp = imagedecode::decode16(pix);
    int width = std::get<0>(tmp);
    int height = std::get<1>(tmp);
    if( std::get<2>(tmp) != "RGBA16" ){
        fatal("Bad image format: Got "+std::get<2>(tmp)+" but expected RGBA16");
    }

    auto& v = std::get<3>(tmp);
    std::uint16_t* start = v.data();
    std::uint16_t* end = v.data()+v.size();

    std::vector< std::span<const char> > layerData{
        { (char*)start, (char*) end }
    };

    Image* img = new Image(
        ctx,
        width,height,(int)layerData.size(),
        VK_FORMAT_R16G16B16A16_UNORM,
        VK_IMAGE_USAGE_TRANSFER_DST_BIT|VK_IMAGE_USAGE_SAMPLED_BIT,
        0,      //flags
        VK_IMAGE_VIEW_TYPE_2D_ARRAY,
        VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
        VK_IMAGE_ASPECT_COLOR_BIT,
        layerData,
        nullptr,-1,
        filename
    );

    _imgmap[filename]=img;
    //~ _offset = img->startingOffset+img->byteSize;
    return img;
}


Image* ImageManager::loadFromData(const std::vector<char>& data, std::string name)
{
    return loadFromData(std::span<const char>(data.data(), data.data()+data.size()), name );
}

Image* ImageManager::loadFromData(std::span<const char> data, std::string name)
{
    std::map<std::string,Image*>::iterator it = _imgmap.find(name);
    if(it!=_imgmap.end()){
        if( it->second->format != VK_FORMAT_R8G8B8A8_UNORM ||
            it->second->viewType != VK_IMAGE_VIEW_TYPE_2D_ARRAY){
                fatal("Two images have the same name but different formats: "+name);
        }
        return it->second;
    }

    auto tmp = imagedecode::decode(data);
    int width = std::get<0>(tmp);
    int height = std::get<1>(tmp);
    assert(std::get<2>(tmp) == "RGBA8");
    std::vector<std::span<const char>> layerData(1);
    layerData[0] = std::get<3>(tmp);

    Image* img = new Image(
        ctx,
        width,height,(int)layerData.size(),
        VK_FORMAT_R8G8B8A8_UNORM,
        VK_IMAGE_USAGE_TRANSFER_DST_BIT|VK_IMAGE_USAGE_SAMPLED_BIT,
        0,      //flags
        VK_IMAGE_VIEW_TYPE_2D_ARRAY,
        VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
        VK_IMAGE_ASPECT_COLOR_BIT,
        layerData,
        nullptr,-1,
        name
    );


    //~ auto img = new Images::BufferImage(data,_offset);
    //~ _offset = img->startingOffset+img->byteSize;
    _imgmap[name]=img;
    return img;
}

Image* ImageManager::createSolidColorImage(float r, float g, float b, float a){

    std::ostringstream oss;
    oss << r << "," << g << "," << b << "," << a;
    std::string name = oss.str();
    auto it = _imgmap.find(name);
    if(it!=_imgmap.end()){
        if( it->second->format != VK_FORMAT_R8G8B8A8_UNORM ||
            it->second->viewType != VK_IMAGE_VIEW_TYPE_2D_ARRAY){
                fatal("Two images have the same name but different formats: "+name);
        }
        return it->second;
    }

    std::uint8_t r_ = std::uint8_t(r*255);
    std::uint8_t g_ = std::uint8_t(g*255);
    std::uint8_t b_ = std::uint8_t(b*255);
    std::uint8_t a_ = std::uint8_t(a*255);
    std::vector<char> tmp{ char(r_), char(g_), char(b_), char(a_) };
    std::vector<std::span<const char>> T{
        {
            tmp.data(), tmp.data()+tmp.size()
        }
    };

    Image* img = new Image(
        ctx,
        1,1,1,
        VK_FORMAT_R8G8B8A8_UNORM,
        VK_IMAGE_USAGE_TRANSFER_DST_BIT|VK_IMAGE_USAGE_SAMPLED_BIT,
        0,      //flags
        VK_IMAGE_VIEW_TYPE_2D_ARRAY,
        VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
        VK_IMAGE_ASPECT_COLOR_BIT,
        T,
        nullptr,-1,
        name
    );

    //~ _offset = img->startingOffset+img->byteSize;
    _imgmap[name]=img;
    return img;
}



Image* ImageManager::createImageFromVkImage(
            int w, int h, int numLayers,
            VkFormat format, VkImageUsageFlags usage,
            VkImageViewType viewType,
            VkImageLayout finalLayout,
            VkImageAspectFlags aspect,
            VkImage image, int numMips,
            std::string name)
{

    if(!ctx){
        fatal("ImageManager.initialize() has not been called");
    }

    Image* img = new Image(
        ctx,
        w,h,numLayers,
        format,
        usage,
        (0),       //flags
        viewType,       //VK_IMAGE_VIEW_TYPE_2D_ARRAY,
        finalLayout,    //VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
        aspect,         //VK_IMAGE_ASPECT_COLOR_BIT,
        std::vector<std::span<const char>>(),
        &image,numMips,
        name
    );

    std::string unique = "$vkimage$" + str( len(_imgmap) );
    _imgmap[unique]=img;
    return img;
}



Image* ImageManager::loadCube(std::array<std::string,6> filenames)
{
    std::string name;
    for(auto& f: filenames)
        name += f+" / ";

    auto it = _imgmap.find(name);
    if(it!=_imgmap.end()){
        if( it->second->format != VK_FORMAT_R8G8B8A8_UNORM ||
            it->second->viewType != VK_IMAGE_VIEW_TYPE_CUBE){
                fatal("Two images have the same name but different formats: "+name);
        }
        return it->second;
    }
    std::vector< std::vector<char> > layerData(6);
    int width=-1,height=-1;
    for(int i=0;i<6;++i){
        std::vector<char> pix = utils::readFile(filenames[i]);
        auto tmp = imagedecode::decode(pix);
        int w = std::get<0>(tmp);
        int h = std::get<1>(tmp);
        assert(std::get<2>(tmp) == "RGBA8");

        if( w != h ){
            fatal("Non-square cubemap image: "+filenames[i]);
        }

        if( i == 0 ){
            width=w;
            height=h;
        } else{
            if(w!=width || h !=height){
                fatal("Mismatched cubemap image size: Expected "+
                    std::to_string(width)+"x"+std::to_string(height)+" but image "+
                    filenames[i]+" is size "+std::to_string(w)+"x"+std::to_string(h));
            }
        }
        layerData[i].swap(std::get<3>(tmp));
    }

    std::vector<std::span<const char> >layers;
    for(int i=0;i<6;++i){
        layers.push_back( {
            layerData[i].data(),
            layerData[i].data()+layerData[i].size()
        });
    }

    Image* img = new Image(
        ctx,
        width,height,(int)layers.size(),
        VK_FORMAT_R8G8B8A8_UNORM,
        VK_IMAGE_USAGE_TRANSFER_DST_BIT|VK_IMAGE_USAGE_SAMPLED_BIT,
        VK_IMAGE_CREATE_CUBE_COMPATIBLE_BIT,      //flags
        VK_IMAGE_VIEW_TYPE_CUBE,
        VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
        VK_IMAGE_ASPECT_COLOR_BIT,
        layers,
        nullptr,-1,
        name
    );

    _imgmap[name] = img;
    return img;
}


Image* ImageManager::createImage(
    int w, int h, int numLayers,
    VkFormat format, VkImageUsageFlags usage,
    VkImageViewType viewType,
    VkImageLayout finalLayout, VkImageAspectFlags aspect,
    const std::vector<std::span<const char> >& initialData,   //can be empty to create uninitialized image
    std::string name
){

    Image* img = new Image(
        ctx,
        w,h,numLayers,
        format,
        usage,
        VkImageCreateFlags(0),       //flags
        viewType,       //VK_IMAGE_VIEW_TYPE_2D_ARRAY,
        finalLayout,    //VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
        aspect,         //VK_IMAGE_ASPECT_COLOR_BIT,
        initialData,
        nullptr,-1,
        name
    );

    std::string unique = "$uninitialized" + std::to_string(_imgmap.size());
    _imgmap[unique]=img;
    return img;
}


void ImageManager::pushToGPU()
{

    //compute size parameters for all the images
    std::vector<VkDeviceSize> sizes;
    std::vector<VkDeviceSize> offsets;
    sizes.reserve(_imgmap.size());
    offsets.reserve(_imgmap.size());

    VkDeviceSize offset=0;
    VkDeviceSize largestImageSize=0;

    Image* img=nullptr;

    for(auto& it : _imgmap){
        img = it.second;

        if( img->pushedToGPU() )
            continue;

        int padding = utils::computePadding(
            offset,
            (int)img->memoryRequirements.alignment
        );
        offset += padding;

        offsets.push_back(offset);
        sizes.push_back(img->memoryRequirements.size);

        offset += sizes.back();
        largestImageSize = std::max( sizes.back(), largestImageSize );
    }

    if( img == nullptr ){
        //no images need to be transferred
        for(auto& f : callbacks ){
            f();
        }
        callbacks.clear();
        return;
    }

    VkDeviceMemory memory = Buffer::allocateMemory(
        ctx,
        img->memoryRequirements.memoryTypeBits,     //OK to use any image's memory requirements here
        offset,
        VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
        false,
        "ImageManager"
    );

    CleanupManager::registerCleanupFunction( [memory](){
        vkFreeMemory( ctx->dev, memory, nullptr );
    });

    StagingBuffer* stagingBuffer = new StagingBuffer(ctx,
        VK_BUFFER_USAGE_TRANSFER_SRC_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT,
        nullptr,largestImageSize,"ImageManager staging");
    int idx=0;
    for(auto& it : _imgmap){
        img = it.second;
        if(img->pushedToGPU())
            continue;
        img->copyDataToGPU(
            stagingBuffer,
            memory,
            offsets[idx]
        );
        idx++;
    }

    stagingBuffer->cleanup();
    delete stagingBuffer;

    for(auto& f : callbacks){
        f();
    }
    callbacks.clear();
}

void ImageManager::addCallback( std::function<void(void)> f)
{
    callbacks.push_back(f);
}
