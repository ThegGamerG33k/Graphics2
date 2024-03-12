#version 450 core

#extension GL_GOOGLE_include_directive : enable
#include "pushconstants.txt"
#include "uniforms.txt"

layout(location=VS_INPUT_POSITION) in vec3 position;

layout(location=0) out vec3 v_objpos;

void main(){
    vec4 p = vec4(position,1.0);
    v_objpos = position.xyz;
    p.xyz += eyePosition.xyz;
    p = p * viewProjMatrix;
    p.z = p.w;
    gl_Position = p;
}