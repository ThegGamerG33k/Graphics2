#define USE_PCG 1

#include "perlin.h"
#include <vector>
#include <cstdint>
#include "math2801.h"
#include <fstream>
#include <iostream>
#include "imageencode.h"

using math2801::vec2;
using math2801::vec3;

#define WATER_MIN -0.5f
#define WATER_MAX -0.05f

#define GRASS_MIN WATER_MAX
#define GRASS_MAX 0.3f

#define DIRT_MIN GRASS_MAX
#define DIRT_MAX 0.45f

#define SNOW_MIN DIRT_MAX
#define SNOW_MAX 1.0f

#define WATER_MIN_COLOR vec3(37,53,150)
#define WATER_MAX_COLOR vec3(55,79,223)

#define GRASS_MIN_COLOR vec3(83,121,77)
#define GRASS_MAX_COLOR vec3(128,187,119)

#define DIRT_MIN_COLOR vec3(210,197,141)
#define DIRT_MAX_COLOR vec3(241,232,196)

#define SNOW_MIN_COLOR vec3(227,248,255)
#define SNOW_MAX_COLOR vec3(255,255,255)


int main(int , char** )
{
    int w=512;
    int h=512;
    float scale=0.008f;

    float minn=100;
    float maxn=-100;

    std::vector<char> pixels;
    pixels.reserve(w*h*4);
    for(int y=0;y<h;++y){
        float yf = float(y)*scale;
        for(int x=0;x<w;++x){
            float xf = float(x)*scale;
            float n = noise(vec2(xf,yf));

            minn = std::min(n,minn);
            maxn = std::max(n,maxn);

            vec3 color;
            if( n < WATER_MAX ){
                color = mix(WATER_MIN_COLOR, WATER_MAX_COLOR, (n-WATER_MIN)/(WATER_MAX-WATER_MIN) );
            } else if( n < GRASS_MAX ){
                color = mix(GRASS_MIN_COLOR, GRASS_MAX_COLOR, (n-GRASS_MIN)/(GRASS_MAX-GRASS_MIN) );
            } else if( n < DIRT_MAX ){
                color = mix(DIRT_MIN_COLOR, DIRT_MAX_COLOR, (n-DIRT_MIN)/(DIRT_MAX-DIRT_MIN) );
            } else {
                color = mix(SNOW_MIN_COLOR, SNOW_MAX_COLOR, (n-SNOW_MIN)/(SNOW_MAX-SNOW_MIN) );
            }

            pixels.push_back( (char)(color.x) );
            pixels.push_back( (char)(color.y) );
            pixels.push_back( (char)(color.z) );
            pixels.push_back( (char)255);
        }
    }
    auto img = imageencode::encodeJPEG(w,h,"RGBA8",pixels,85);
    std::ofstream out("terrain.jpg",std::ios::binary);
    out.write(img.data(), img.size() );
    out.close();
    std::cout << minn << "," << maxn << "\n";
    return 0;
}


