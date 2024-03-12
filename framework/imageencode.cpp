#define _CRT_SECURE_NO_WARNINGS

#include "imageencode.h"

#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "ext/stb_image_write.h"

#include <stdexcept>
#include <iostream>
#include <fstream>
#include "consoleoutput.h"

static void callback(void* ctx, void* data, int size)
{
    std::vector<char>* p = (std::vector<char>*)ctx;
    char* c = (char*)data;
    p->insert(p->end(),c,c+size);
}

std::vector<char> imageencode::encodePNG(int w, int h, std::string fmt, const std::vector<char>& data)
{
    std::vector<char> pngdata;
    int components;
    if( fmt == "RGBA8" )
        components = 4;
    else if( fmt == "RGB8" )
        components = 3;
    else
        fatal("Format must be 'RGBA8' or 'RGB8'");

    stbi_write_png_to_func(callback, &pngdata, w,h, components ,  data.data(), w*components);
    return pngdata;
}


std::vector<char> imageencode::encodeJPEG(int w, int h, std::string fmt, const std::vector<char>& data, int quality)
{
    std::vector<char> jpegdata;
    int components;
    if( fmt == "RGBA8" )
        components = 4;
    else if( fmt == "RGB8" )
        components = 3;
    else
        fatal("Format must be 'RGBA8' or 'RGB8'");

    stbi_write_jpg_to_func(callback, &jpegdata, w,h, components ,  data.data(), quality);
    return jpegdata;
}


void imageencode::writePNG(std::string filename, int w, int h, std::string fmt, const std::vector<char>& data)
{
    std::ofstream out(filename.c_str(), std::ios::binary);
    if(!out.good()){
        fatal("Cannot open file "+filename);
    }
    auto tmp = encodePNG(w,h,fmt,data);
    out.write(tmp.data(), tmp.size() );
    if(out.fail())
        fatal("I/O error");
    out.close();
    if(out.fail())
        fatal("I/O error");
}

void imageencode::writeJPEG(std::string filename, int w, int h, std::string fmt, const std::vector<char>& data)
{
    std::ofstream out(filename.c_str(), std::ios::binary);
    if(!out.good()){
        fatal("Cannot open file "+filename);
    }
    auto tmp = encodeJPEG(w,h,fmt,data,85);
    out.write(tmp.data(), tmp.size() );
    if(out.fail())
        fatal("I/O error");
    out.close();
    if(out.fail())
        fatal("I/O error");
}
