#include "imagedecode.h"
#include <cstring>
#include <stdexcept>
#include <iostream>
#include <fstream>
#include "consoleoutput.h"

#define STB_IMAGE_IMPLEMENTATION
#include "ext/stb_image.h"

static std::tuple<int,int,std::string,std::vector<char>> decodeIt(
    const char* data, unsigned size)
{
    std::tuple<int,int,std::string,std::vector<char>> rv;
    int w,h,nc;
    stbi_uc* pix = stbi_load_from_memory( (stbi_uc*) data, (unsigned)size,
        &w,&h,&nc,4);
    std::get<0>(rv) = w;
    std::get<1>(rv) = h;
    std::get<2>(rv) = "RGBA8";
    std::get<3>(rv).resize(w*h*4);
    std::memcpy(std::get<3>(rv).data(), pix, w*h*4 );
    stbi_image_free(pix);
    return rv;
}

std::tuple<int,int,std::string,std::vector<char>> imagedecode::decode(const std::vector<char>& data)
{
    return decodeIt(data.data(),(int)data.size());
}
std::tuple<int,int,std::string,std::vector<char>> imagedecode::decode(std::span<const char> data)
{
    return decodeIt(data.data(),(unsigned)data.size());
}

std::tuple<int,int,std::string,std::vector<std::uint16_t>> imagedecode::decode16(const std::vector<char>& data)
{
    std::tuple<int,int,std::string,std::vector<std::uint16_t>> rv;
    int w,h,nc;
    stbi_us* pix = stbi_load_16_from_memory( (stbi_uc*) data.data(), (unsigned)data.size(),
        &w,&h,&nc,4);
    std::get<0>(rv) = w;
    std::get<1>(rv) = h;
    std::get<2>(rv) = "RGBA16";
    std::get<3>(rv).resize(w*h*4);
    std::memcpy(std::get<3>(rv).data(), pix, w*h*4 );
    stbi_image_free(pix);
    return rv;
}

std::tuple<int,int,std::string,std::vector<char>> imagedecode::decode(std::string filename)
{
    std::ifstream in(filename.c_str(),std::ios::binary);
    if(!in.good()){
        fatal("Cannot load file "+filename);
    }
    in.seekg(0,std::ios::end);
    auto sz = in.tellg();
    in.seekg(0);
    std::vector<char> cdata(sz);
    in.read( cdata.data(), sz );
    if(in.fail()){
        fatal("I/O error");
    }
    in.close();
    return decode(cdata);
}

std::tuple<int,int,std::string,std::vector<char>> imagedecode::decode(const char* filename)
{
    return decode(std::string(filename));
}
