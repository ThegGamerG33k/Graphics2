#version 450 core

#extension GL_GOOGLE_include_directive : enable

#include "importantconstants.h"
#include "pushconstants.txt"
#include "uniforms.txt"

layout(location=VS_INPUT_TEXCOORD) in vec2 texcoord;
layout(location=VS_INPUT_POSITION) in vec3 indexInfo;

layout(set=0,binding=BILLBOARD_TEXTURE_SLOT) uniform
                    textureBuffer billboardPositions;

layout(location=0) out vec2 v_texcoord;
layout(location=1) out float lifeLeft;

void main(){

    int idx = int(indexInfo.x);
    vec2 size = indexInfo.yz;

    vec4 p = texelFetch( billboardPositions,
        gl_InstanceIndex * BILLBOARD_BATCH_SIZE + idx );

    lifeLeft = p.w;

    float scalefactor = -lifeLeft + 5;

    size = size * scalefactor;


    p.w = 1.0;

    p = p * worldMatrix;

    p = p * viewMatrix;
    p.xy += size * (2.0 * texcoord - vec2(1.0));
    p = p * projMatrix;
    gl_Position = p;
    v_texcoord = texcoord;
}