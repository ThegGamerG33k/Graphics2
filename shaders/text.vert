#version 450 core

#extension GL_GOOGLE_include_directive : enable
#include "textpushconstants.txt"


layout(location=0) in vec3 position;

layout(location=0) out vec2 uv;
layout(location=1) out vec4 textcolor;

layout(set=0,binding=0) uniform sampler samp;
layout(set=0,binding=1) uniform utextureBuffer charData;
layout(set=0,binding=2) uniform texture2DArray atlas;

void main(){
    int idx = int(position.z);
    uvec4 tmp = texelFetch(
        usamplerBuffer(charData,samp),
        idx
    );

    float screenX = float(tmp[0] >> 16);
    float screenY = float((tmp[0] & 0xffff));

    float atlasX = float(tmp[1] >> 16);
    float atlasY = float((tmp[1] & 0xffff ));
    vec2 atlasOrigin = vec2(atlasX,atlasY);

    float ascent      = float( (tmp[2] >> 24) & 0xff );
    float descent     = float( (tmp[2] >> 16) & 0xff);
    float width       = float( (tmp[2] >> 8 ) & 0xff);
    float leftBearing = float( (tmp[2]      ) & 0xff) - 128.0;

    float r = float( (tmp[3]>>24) & 0xff);
    float g = float( (tmp[3]>>16) & 0xff);
    float b = float( (tmp[3]>> 8) & 0xff);
    float a = float( (tmp[3]    ) & 0xff);
    textcolor = vec4(r/255.0, g/255.0, b/255.0, a/255.0 );

    float left = screenX + leftBearing;
    float right = left + width;
    float top = screenY - ascent;
    float bottom = screenY + descent ;

    float x = left + position.x * (right-left);
    float y = top + position.y * (bottom-top);

    gl_Position = vec4(
        -1.0 + 2.0 * x/screenSize.x,
        -1.0 + 2.0 * y/screenSize.y,
        0.0,
        1.0
    );

    vec2 texcoord = position.xy;

    float u = atlasOrigin.x + texcoord.x * width;
    float v = atlasOrigin.y - ascent + texcoord.y*(ascent+descent);
    uv = vec2(u,v);

}
