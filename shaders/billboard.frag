#version 450 core

#extension GL_GOOGLE_include_directive : enable
#include "pushconstants.txt"
#include "uniforms.txt"

layout(location=0) in vec2 texcoord;
layout(location=0) out vec4 color;

layout(set=0,binding=NEAREST_SAMPLER_SLOT) uniform sampler nearestSampler;
layout(set=0,binding=LINEAR_SAMPLER_SLOT) uniform sampler linearSampler;
layout(set=0,binding=MIPMAP_SAMPLER_SLOT) uniform sampler mipSampler;
layout(set=0,binding=BASE_TEXTURE_SLOT) uniform texture2DArray baseTexture;

void main(){

vec2 t = texcoord;
color = texture( sampler2DArray(baseTexure, mipSampler), vec3(texcoord, 0));

}