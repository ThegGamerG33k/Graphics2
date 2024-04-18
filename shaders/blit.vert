#version 450 core

#extension GL_GOOGLE_include_directive : enable

#include "importantconstants.h"

layout(location=VS_INPUT_POSITION) in vec3 position;
layout(location=VS_INPUT_TEXCOORD) in vec2 texcoord;

layout(location=0) out vec2 v_texcoord;

void main(){
    vec4 p = vec4(position,1.0);
    gl_Position = p;
    v_texcoord = texcoord;
}