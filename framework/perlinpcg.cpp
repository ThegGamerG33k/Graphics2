#define USE_PCG 1
#include "perlin.h"


#include <vector>
#include <cstdint>
#include "math2801.h"
#include <fstream>
#include <iostream>
#include "imageencode.h"

using math2801::vec2;

int main(int , char** )
{
    int w=512;
    int h=512;
    float scale=16.0f;

    std::vector<char> pixels;
    pixels.reserve(w*h*4);
    for(int y=0;y<h;++y){
        float yf = float(y/(h-1.0))*scale;
        for(int x=0;x<w;++x){
            float xf = float(x/(w-1.0))*scale;
            float n = noise(vec2(xf,yf));
            n += 1.0f;
            n *= 0.5f;
            char p = char( (signed char)(n*255.0f) );
            pixels.push_back(p);
            pixels.push_back(p);
            pixels.push_back(p);
            pixels.push_back((char)255);
        }
    }
    auto img = imageencode::encodeJPEG(w,h,"RGBA8",pixels,85);
    std::ofstream out("perlinpcg.jpg",std::ios::binary);
    out.write(img.data(), img.size() );
    out.close();
    return 0;
}


