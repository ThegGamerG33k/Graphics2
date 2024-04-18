#version 450 core
#extension GL_GOOGLE_include_directive : enable

#include "pushconstants.txt"
#include "uniforms.txt"


layout(location=VS_OUTPUT_TEXCOORD) in vec2 texcoord;
layout(location=VS_OUTPUT_TEXCOORD2) in vec2 texcoord2;
layout(location=VS_OUTPUT_NORMAL) in vec3 normal;
layout(location=VS_OUTPUT_WORLDPOS) in vec3 worldPosition;
layout(location=VS_OUTPUT_TANGENT) in vec4 tangent; 

layout(location=0) out vec4 color;

layout(set=0,binding=NEAREST_SAMPLER_SLOT) uniform sampler nearestSampler;
layout(set=0,binding=LINEAR_SAMPLER_SLOT) uniform sampler linearSampler;
layout(set=0,binding=MIPMAP_SAMPLER_SLOT) uniform sampler mipSampler;
layout(set=0,binding=BASE_TEXTURE_SLOT) uniform texture2DArray baseTexture;
layout(set=0,binding=EMISSIVE_TEXTURE_SLOT) uniform texture2DArray emitTexture;
layout(set=0,binding=NORMAL_TEXTURE_SLOT) uniform texture2DArray normalTexture;
layout(set=0,binding=METALLICROUGHNESS_TEXTURE_SLOT) uniform texture2DArray metallicRoughnessTexture;
layout(set=0,binding=ENVMAP_TEXTURE_SLOT) uniform textureCube environmentMap;


#define AMBIENT_ABOVE vec3(0.3,0.3,0.3)
#define AMBIENT_BELOW vec3(0.1,0.1,0.1)
#define PI 3.14159265358979323

void computeLightContribution(int i, vec3 N,
                            out vec3 diff, out vec3 spec, float mu, float rho, vec3 F0, vec4 c, vec3 V){

    vec3 lightPosition = lightPositionAndDirectionalFlag[i].xyz;
    float positional = lightPositionAndDirectionalFlag[i].w;
    vec3 lightColor = lightColorAndIntensity[i].xyz;
    float intensity = lightColorAndIntensity[i].w;
    vec3 spotDir = spotDirection[i].xyz ;
    float cosSpotInnerAngle = cosSpotAngles[i].x;
    float cosSpotOuterAngle = cosSpotAngles[i].y;

          


    
    //lamberts
    //float dp = dot(L,N);
    //dp = clamp(dp, 0.0, 1.0);
    

    //phong specular
    //float sp = dot(V,R);
    //sp = clamp(sp, 0.0, 1.0);
    //sp = pow(sp,16.0);
    //sp = sp * sign(dp);

    vec3 L = normalize( lightPosition - worldPosition );         

                
    vec3 R = reflect(-L,N);     

    vec3 H = normalize(L + V);         

        
    float cosNV = dot(N, V);
    cosNV = clamp(cosNV, 0.0, 1.0);

    float cosNL = dot(N, L);
    cosNL = clamp(cosNL, 0.0, 1.0);

    float sinNV = sqrt(1-pow(cosNV, 2));
    sinNV = clamp(sinNV, 0.0, 1.0);

    float sinNL = sqrt(1-pow(cosNL, 2));
    sinNL = clamp(sinNL, 0.0, 1.0);

    float cosVH = dot(V, H);
    cosVH = clamp(cosVH, 0.0, 1.0);

    float cosNH = dot(N, H);
    cosNH = clamp(cosNH, 0.0, 1.0);


    //NEW Schlick
    //specular = cos θNL ⋅ F ⋅ ψ ⋅ D
    
    vec3 F = (pow(1 - cosVH, 5))* (1-F0) + F0;










    //float p2 = pow(rho, 2);
    //float psi = 1.0 / (2 *(cosNL * sqrt(p2 + (1-p2) * (cosNV * cosNV))    +    cosNV * sqrt(p2 + (1-p2) * (cosNL * cosNL))));
    //float Dis = (1.0/PI) * pow(rho/(1+ (cosNH * cosNH) * (p2-1))    ,2);
    //vec3 sp = cosNL * F * psi * Dis;




    float rho2 = rho*rho;
    float disc1 = max(0.0,
        rho2 + (1.0-rho2) * cosNV * cosNV );

    float disc2 = max(0.0,
        rho2 + (1.0-rho2) * cosNL * cosNL );

    float denom = max(0.0001,
        cosNL * sqrt( disc1 ) +
        cosNV * sqrt( disc2 )
        );
        
    float Vis = 1.0 / (2.0 * denom );   //microfacet visibility
    float tmp = rho / (1.0 + cosNH*cosNH * (rho2-1.0) );
    float Dis = 1.0/PI * tmp*tmp;       //microfacet distribution
    vec3 sp = cosNL * F * Vis * Dis;     //specular amount

    vec3 d = mix( 0.96*c.xyz , vec3(0), mu);
    vec3 dp = cosNL * (vec3(1.0)-F) * (d/PI);
    ////



    

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


//b color from doBumpMapping
vec3 doBumpMapping(vec3 b, vec3 N)
{
    if( tangent.w == 0.0 )
        return N;

    vec3 T = tangent.xyz;
    T = T - dot(T,N) * N;
    T = normalize(T);

    vec3 B = cross(N, T);
    B = B * tangent.w;

    vec3 beta = vec3((b.x - 0.5) * 2, (b.y - 0.5)* 2, (b.z - 0.5) * 2);
    beta = beta * normalFactor;

    mat3 M = mat3(T.x, B.x, N.x,
                  T.y, B.y, N.y,
                  T.z, B.z, N.z);

    vec3 N_prime = beta * M;

    return N_prime;

}

void main(){
    if( doingReflections == 1 ){
        if( dot(vec4(worldPosition,1.0),reflectionPlane) < 0 ){
            discard;
            return;
        }
    }
    
   
    vec3 tc = vec3(texcoord,0.0);

    vec4 c = texture(
        sampler2DArray(baseTexture,mipSampler), tc
    );
    c = c * baseColorFactor;


    vec3 mr = texture(
        sampler2DArray(metallicRoughnessTexture, mipSampler), tc 
    ).rgb;

    float mu = mr.b * metallicFactor;
    float rho = mr.g * roughnessFactor;


    vec3 b = texture( sampler2DArray(normalTexture, mipSampler),
                    vec3(texcoord2,0.0) ).xyz;

    vec3 N = normalize(normal);
    N = doBumpMapping(b.xyz, N);
    N = (vec4(N,0) * worldMatrix).xyz;
    N = normalize(N);
 
    vec3 V = normalize(eyePosition - worldPosition);      

    float mappedY = 0.5 * (N.y+1.0);
    vec3 ambient = c.rgb * mix( AMBIENT_BELOW, AMBIENT_ABOVE, mappedY );

    if( doingShadow != 0 ){
        color = vec4(ambient,1);
        return;
    }


    vec3 totaldp = vec3(0.0);
    vec3 totalsp = vec3(0.0);


    //...fresnel stuff independent of light
    vec3 F0 = mix( vec3(0.04), c.xyz, mu);


    for(int i=0;i<MAX_LIGHTS;++i){
        vec3 dp, sp;
        computeLightContribution(i,N,dp,sp,mu,rho,F0,c, V);

        totaldp += dp ;
        totalsp += sp ;
    }
    
    vec3 ec = texture(
        sampler2DArray(emitTexture,mipSampler), tc
    ).rgb;
    ec = ec * emissiveFactor;

    c.rgb = c.rgb * (ambient + totaldp) + totalsp ;


    //reflected view vector
    vec3 reflectedView = reflect(-V,N);


    vec3 reflectionColor = texture(
        samplerCube(environmentMap, mipSampler),
        reflectedView,rho * 8.0).rgb;

    
    //c.rgb += mu * reflectionColor;

    if (doingReflections != 2)
        c.rgb += pow(1.0-rho,4.0) * mu * reflectionColor;


    c.rgb += ec.rgb;
    c.rgb = clamp(c.rgb, vec3(0.0), vec3(1.0) );
    color = c;

    //if mirror, blend with reflections
    if( doingReflections == 2 )
    color.a *= 0.85;
}
