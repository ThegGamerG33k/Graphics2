#version 450 core

layout(location=0) in vec2 uv;
layout(location=1) in vec4 textcolor;

layout(set=0,binding=0) uniform sampler samp;
layout(set=0,binding=1) uniform utextureBuffer charData;
layout(set=0,binding=2) uniform texture2DArray atlas;

layout(location=0) out vec4 color;


void main()
{
    vec4 T = texelFetch( sampler2DArray(atlas,samp), ivec3(uv.x,uv.y,0), 0 );
    color = vec4( textcolor.rgb,textcolor.a*T.r );
}
