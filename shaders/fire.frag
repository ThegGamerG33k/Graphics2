#version 450 core

#extension GL_GOOGLE_include_directive : enable

#include "uniforms.txt"
#include "perlin.h"

layout(set=0,binding=BASE_TEXTURE_SLOT) uniform texture2DArray baseColorTexture;
layout(set=0,binding=CLAMPING_MIPMAP_SAMPLER_SLOT) uniform sampler texSampler;

layout(location=VS_OUTPUT_TEXCOORD) in vec2 texcoord;

layout(location=0) out vec4 color;

void main(){

    vec3 v = vec3(texcoord*8.0, elapsed*2.0);
    v.y += elapsed*7.0;
    float n = noise(v);
    n += 0.5 * noise(2.0*v);
    n += 0.25 * noise(4.0*v);
    n = 0.5*(n + 1.0);
    float s = smoothstep(0.0, texcoord.y, n*0.5 );
    s=smoothstep(0.0,1.0,s);
    color = texture(
        sampler2DArray(baseColorTexture,texSampler),
        vec3( 0.5, 1.0-s, 0 )
    );

}