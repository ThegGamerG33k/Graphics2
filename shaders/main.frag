#version 450 core
#extension GL_GOOGLE_include_directive : enable

#include "pushconstants.txt"
#include "uniforms.txt"


layout(location=VS_OUTPUT_TEXCOORD) in vec2 texcoord;
layout(location=VS_OUTPUT_TEXCOORD2) in vec2 texcoord2;
layout(location=VS_OUTPUT_NORMAL) in vec3 normal;
layout(location=VS_OUTPUT_WORLDPOS) in vec3 worldPosition;
layout(location=VS_OUTPUT_TANGENT) in vec4 tangent;
layout(set=0,binding=ENVMAP_TEXTURE_SLOT) uniform textureCube environmentMap;


layout(location=0) out vec4 color;

layout(set=0,binding=NEAREST_SAMPLER_SLOT) uniform sampler nearestSampler;
layout(set=0,binding=LINEAR_SAMPLER_SLOT) uniform sampler linearSampler;
layout(set=0,binding=MIPMAP_SAMPLER_SLOT) uniform sampler mipSampler;
layout(set=0,binding=BASE_TEXTURE_SLOT) uniform texture2DArray baseTexture;
layout(set=0,binding=EMISSIVE_TEXTURE_SLOT) uniform texture2DArray emitTexture;
layout(set=0,binding=NORMAL_TEXTURE_SLOT) uniform texture2DArray normalTexture;
layout(set=0,binding=METALLICROUGHNESS_TEXTURE_SLOT) uniform texture2DArray metallicRoughnessTexture;

#define AMBIENT_ABOVE vec3(0.3,0.3,0.3)
#define AMBIENT_BELOW vec3(0.1,0.1,0.1)
#define PI 3.14159265358979323


void computeLightContribution(int i, vec3 N,
                            out vec3 diff, out vec3 spec, in vec3 objectColor, float mu, float rho){

    vec3 lightPosition = lightPositionAndDirectionalFlag[i].xyz;
    float positional = lightPositionAndDirectionalFlag[i].w;
    vec3 lightColor = lightColorAndIntensity[i].xyz;
    float intensity = lightColorAndIntensity[i].w;
    vec3 spotDir = spotDirection[i].xyz ;
    float cosSpotInnerAngle = cosSpotAngles[i].x;
    float cosSpotOuterAngle = cosSpotAngles[i].y;

    vec3 L = normalize( lightPosition - worldPosition );
    vec3 V = normalize(eyePosition - worldPosition);
    vec3 H = normalize(L + V);


    //lambert
    //float dp = dot(L,N);
    //dp = clamp(dp, 0.0, 1.0);

    float cos_theta_NH = clamp(dot(N,H), 0.0, 1.0);
    float cos_theta_VH = clamp(dot(V,H), 0.0, 1.0);
    float cos_theta_NV = clamp(dot(N,V), 0.0, 1.0);
    float cos_theta_NL = clamp(dot(N,L), 0.0, 1.0);

    //phong
    vec3 F0 = mix( vec3(0.04), objectColor, metallicFactor );
    vec3 one_minus_F0 = vec3(1.0)-F0;
    vec3 F = F0 + one_minus_F0 * pow(1.0 - cos_theta_VH,5.0);

    vec3 d = mix( 0.96*objectColor , vec3(0), mu );
    vec3 dp =  cos_theta_NL * ( vec3(1.0)-F) * d ;


    float rho2 = rho*rho;
    float disc1 = max(0.0,
            rho2 + (1.0-rho2) * cos_theta_NV * cos_theta_NV );
    float disc2 = max(0.0,
            rho2 + (1.0-rho2) * cos_theta_NL * cos_theta_NL );
    float denom = max(0.0001,
            cos_theta_NL * sqrt( disc1 ) +
            cos_theta_NV * sqrt( disc2 )
    );
    float Vis = 1.0 / (2.0 * denom );   //microfacet visibility
    float tmp = rho / (1.0 + cos_theta_NH*cos_theta_NH * (rho2-1.0) );
    float Dis = 1.0/PI * tmp*tmp;       //microfacet distribution
    vec3 sp = cos_theta_NL * F * Vis * Dis;     //specular amount


    
    vec3 R = reflect(-L,N);
    //float sp = dot(V,R);
    //sp = clamp(sp, 0.0, 1.0);
    //sp = pow(sp,16.0);
    //sp = sp * sign(dp);

    float D = distance( lightPosition, worldPosition );
    float A = 1.0/( D*(attenuation[2] * D   +
                       attenuation[1]     ) +
                       attenuation[0]     );
    A = clamp(A, 0.0, 1.0);

    dp *= A;
    sp *= A;

    float cosineTheta = dot(-L,spotDir);
    float spotAttenuation = smoothstep(
                    cosSpotOuterAngle,
                    cosSpotInnerAngle,
                    cosineTheta);
    dp *= spotAttenuation;
    sp *= spotAttenuation;

    diff = dp * lightColor * intensity;
    spec = sp * lightColor * intensity;
}


//b = color of bumpmapping
//N = unperturbed normal
//returns perturbed normal

vec3 doBumpMapping(vec3 b, vec3 N)
{
    if( tangent.w == 0.0 )
        return N;

    vec3 T = tangent.xyz;
    T = T - dot(T,N) * N;
    vec3 B = cross(N,T);
    B *= tangent.w;

    mat3 E = mat3(T.x, B.x, N.x,
             T.y, B.y, N.y,
             T.z, B.z, N.z);
    
    vec3 Beta = 2 * (b - 0.5);
    Beta *= normalFactor;
    
    N = Beta * E;

    return N;       //bump mapped normal

}

void main(){
    vec3 V = normalize(eyePosition - worldPosition);
    vec3 tc = vec3(texcoord,0.0);
    vec4 c = texture(
        sampler2DArray(baseTexture,mipSampler), tc
    );
    c = c * baseColorFactor;
    vec3 N = normalize(normal);

    // b = kinda of periwinkle color
    vec3 b = texture( sampler2DArray(normalTexture, mipSampler),
                    vec3(texcoord2,0.0) ).xyz;

    vec4 mr = texture(sampler2DArray(metallicRoughnessTexture, mipSampler), tc);
    float mu = mr.b * metallicFactor;
    float rho = mr.g * roughnessFactor;
    
    

    N = doBumpMapping(b.xyz, N);
    N = (vec4(N, 0.0) * worldMatrix).xyz;
    N = normalize(N);
    

    float mappedY = 0.5 * (N.y+1.0);
    vec3 ambient = c.rgb * mix( AMBIENT_BELOW, AMBIENT_ABOVE, mappedY );

    vec3 totaldp = vec3(0.0);
    vec3 totalsp = vec3(0.0);

    for(int i=0;i<MAX_LIGHTS;++i){
        vec3 dp, sp;
        computeLightContribution(i,N,dp,sp,c.rgb, mu, rho);  //void computeLightContribution(int i, vec3 N, out vec3 diff, out vec3 spec, in vec3 objectColor)

        totaldp += dp ;
        totalsp += sp ;
    }

    vec3 ec = texture(
        sampler2DArray(emitTexture,mipSampler), tc
    ).rgb;
    ec = ec * emissiveFactor;

    //reflected view vector
    vec3 reflectedView = reflect(-V,N);

    vec3 reflectionColor = texture(
        samplerCube(environmentMap, mipSampler),
        reflectedView,
        rho*8.0
    ).rgb;

    c.rgb = c.rgb * (ambient + totaldp) + totalsp ;
    c.rgb += ec.rgb;
    c.rgb += mu * reflectionColor;
    c.rgb = clamp(c.rgb, vec3(0.0), vec3(1.0) );
    color = c;

}
