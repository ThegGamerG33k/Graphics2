#include "Framework.h"
#include "Light.h"
#include "shaders/importantconstants.h"
#include "ext/json11.h"
#include <assert.h>
#include <iostream>
#include <fstream>
#include <numbers>
#include <cmath>
#include <cstring>
#include <cstdlib>
#include "gltf.h"
#include "Descriptors.h"
#include "PushConstants.h"

namespace gltf {

class Material{
  public:
    vec4 baseColorFactor;
    Image* baseColorTexture;
    Image* normalTexture;
    vec3 emissiveFactor;
    float normalFactor;
    Image* emissiveTexture;
    Image* metallicRoughnessTexture;
    float metallicFactor;
    float roughnessFactor;
};

class Primitive{
  public:
    VertexManager::Info drawinfo;
    Material material;

    Primitive( VertexManager* vertexManager,
               std::vector<vec3>& positions,
               std::vector<vec2>& texcoords,
               std::vector<vec2>& texcoords2,
               std::vector<vec3>& normals,
               std::vector<vec4>& tangents,
               std::vector<std::uint32_t>& indices,
               Material mtl)
    {

        this->drawinfo = vertexManager->addIndexedData(
            "positions",positions,
            "texcoords", texcoords,
            "texcoords2", texcoords2,
            "normals",normals,
            "tangents", tangents,
            "indices",indices
        );
        this->material = mtl;
    }

    void draw( VkCommandBuffer cmd, DescriptorSet* dset, PushConstants* pushc){
        dset->setSlot( BASE_TEXTURE_SLOT, this->material.baseColorTexture->view() );
        dset->setSlot( EMISSIVE_TEXTURE_SLOT, this->material.emissiveTexture->view() );
        dset->setSlot(NORMAL_TEXTURE_SLOT, this->material.normalTexture->view());
        dset->setSlot(METALLICROUGHNESS_TEXTURE_SLOT, this->material.metallicRoughnessTexture->view());
        dset->bind(cmd);
        pushc->set(cmd,"baseColorFactor",this->material.baseColorFactor);
        pushc->set(cmd,"emissiveFactor",this->material.emissiveFactor);
        pushc->set(cmd, "normalFactor", this->material.normalFactor);
        pushc->set(cmd, "metallicFactor", this->material.metallicFactor);
        pushc->set(cmd, "roughnessFactor", this->material.roughnessFactor);
        vkCmdDrawIndexed(
            cmd,
            this->drawinfo.numIndices,
            1,              //instance count
            this->drawinfo.indexOffset,
            this->drawinfo.vertexOffset,
            0               //first instance
        );
    }
};


Mesh::Mesh(std::string name_)
{
    this->name=name_;
    this->worldMatrix = math2801::mat4::identity();
}


void Mesh::draw(
            VulkanContext* ctx,
            VkCommandBuffer cmd,
            DescriptorSet* dset,
            PushConstants* pushc)
{
    ctx->beginCmdRegion(cmd,"Drawing "+this->name);

    pushc->set(cmd,"worldMatrix",this->worldMatrix);

    for(auto& p : this->primitives){
        p->draw(cmd,dset,pushc);
    }

    ctx->endCmdRegion(cmd);

}


std::tuple<std::span<char>, std::string, int> parseAttributeHelper(
        const json11::Json& J, std::vector<char>& binaryData, unsigned accessorIndex)
{
    json11::Json accessor = J["accessors"][accessorIndex];
    int ctype = accessor["componentType"];
    std::string atype = accessor["type"];
    // int count = accessor["count"];
    int viewIndex = accessor["bufferView"];
    json11::Json view = J["bufferViews"][viewIndex];
    int byteSize = view["byteLength"];
    int byteOffset = view.get("byteOffset",0);
    std::span<char> A( binaryData.data()+byteOffset, binaryData.data()+byteOffset+byteSize );
    return {A,atype,ctype};
}



void parseAttribute( const json11::Json& J, std::vector<char>& binaryData, unsigned accessorIndex, std::vector<vec4>& v)
{
    auto [S,typ,ctyp] = parseAttributeHelper(J,binaryData,accessorIndex);
    const int F32 = 5126;    //constants defined in GLTF/OpenGL/WebGL spec
    assert( typ == "VEC4" );
    assert( ctyp == F32);
    v.resize(S.size()/sizeof(vec4));
    float* f = (float*) S.data();
    vec4* p = v.data();
    for(unsigned i=0;i<(unsigned)v.size();++i){
        p->x = *f++;
        p->y = *f++;
        p->z = *f++;
        p->w = *f++;
        p++;
    }
}

void parseAttribute( const json11::Json& J, std::vector<char>& binaryData, unsigned accessorIndex, std::vector<vec3>& v)
{
    auto [S,typ,ctyp] = parseAttributeHelper(J,binaryData,accessorIndex);
    const int F32 = 5126;    //constants defined in GLTF/OpenGL/WebGL spec
    assert( typ == "VEC3" );
    assert( ctyp == F32);
    v.resize(S.size()/sizeof(vec3));
    float* f = (float*) S.data();
    vec3* p = v.data();
    for(unsigned i=0;i<(unsigned)v.size();++i){
        p->x = *f++;
        p->y = *f++;
        p->z = *f++;
        p++;
    }
}

void parseAttribute( const json11::Json& J, std::vector<char>& binaryData, unsigned accessorIndex, std::vector<vec2>& v)
{
    auto [S,typ,ctyp] = parseAttributeHelper(J,binaryData,accessorIndex);
    const int F32 = 5126;    //constants defined in GLTF/OpenGL/WebGL spec
    assert( typ == "VEC2" );
    assert( ctyp == F32);
    v.resize(S.size()/sizeof(vec2));
    float* f = (float*) S.data();
    vec2* p = v.data();
    for(unsigned i=0;i<(unsigned)v.size();++i){
        p->x = *f++;
        p->y = *f++;
        p++;
    }}

void parseAttribute( const json11::Json& J, std::vector<char>& binaryData, unsigned accessorIndex, std::vector<std::uint32_t>& v)
{
    auto [S,typ,ctyp] = parseAttributeHelper(J,binaryData,accessorIndex);
    assert( typ == "SCALAR" );
    const int U16 = 5123;    //constants defined in GLTF/OpenGL/WebGL spec
    const int U32 = 5125;
    if( ctyp == U16 ){
        unsigned numElements = (unsigned)S.size()/2;
        v.resize(S.size()/2);
        std::uint16_t* src = (std::uint16_t*) S.data();
        std::uint32_t* dst = (std::uint32_t*) v.data();
        for(unsigned i=0;i<numElements;++i){
            *dst++ = *src++;
        }
    } else if( ctyp == U32 ){
        v.resize(S.size()/4);
        std::memcpy(v.data(), S.data(), S.size() );
    } else {
        assert(0);
    }
}

Image* loadTexture(const json11::Json& J, std::vector<char>& binaryData, int textureIndex)
{
    json11::Json texture = J["textures"][textureIndex];
    int sourceIndex = texture["source"];
    json11::Json image = J["images"][sourceIndex];
    int bufferViewIndex = image["bufferView"];
    json11::Json bufferView = J["bufferViews"][bufferViewIndex];
    int start = bufferView["byteOffset"];
    int size = bufferView["byteLength"];
    std::span<char> bufferData(binaryData.data()+start, binaryData.data()+start+size );
    return ImageManager::loadFromData(bufferData,
        image.get("name","anonymous image")
    );
}

Material parseMaterial(const json11::Json& J, std::vector<char>& binaryData, int materialIndex)
{
    json11::Json material = J["materials"][materialIndex];
    json11::Json metallicRoughness = material["pbrMetallicRoughness"];

    Material M;
    if (metallicRoughness.hasKey("baseColorFactor")) {
        M.baseColorFactor.x = metallicRoughness["baseColorFactor"][0];
        M.baseColorFactor.y = metallicRoughness["baseColorFactor"][1];
        M.baseColorFactor.z = metallicRoughness["baseColorFactor"][2];
        M.baseColorFactor.w = metallicRoughness["baseColorFactor"][3];
    }
    else {
        M.baseColorFactor = vec4(1, 1, 1, 1);
    }

    if (metallicRoughness.hasKey("baseColorTexture")) {
        M.baseColorTexture = loadTexture(
            J, binaryData,
            metallicRoughness["baseColorTexture"]["index"]
        );
    }
    else {
        //default color = white
        M.baseColorTexture = ImageManager::createSolidColorImage(1, 1, 1, 1);
    }

    if (material.hasKey("emissiveFactor")) {
        M.emissiveFactor.x = material["emissiveFactor"][0];
        M.emissiveFactor.y = material["emissiveFactor"][1];
        M.emissiveFactor.z = material["emissiveFactor"][2];
    }
    else {
        M.emissiveFactor = vec3(0, 0, 0);
    }

    if (material.hasKey("emissiveTexture")) {
        M.emissiveTexture = loadTexture(
            J, binaryData, material["emissiveTexture"]["index"]
        );
    }
    else {
        M.emissiveTexture = ImageManager::createSolidColorImage(1, 1, 1, 1);
    }

    if (material.hasKey("normalTexture")) {
        int idx = material["normalTexture"]["index"];
        M.normalTexture = loadTexture(J, binaryData, idx);
        json11::Json texture = J["textures"][idx];
        int sourceIndex = texture["source"];
        json11::Json image = J["images"][sourceIndex];
        if (image.hasKey("scale"))
            M.normalFactor = image["scale"];
        else
            M.normalFactor = 1.0f;
    }
    else {
        M.normalTexture = ImageManager::createSolidColorImage(0.5, 0.5, 1.0, 1.0);
        M.normalFactor = 1.0f;
    }


    
    if (metallicRoughness.hasKey("metallicRoughnessTexture")) {
        M.metallicRoughnessTexture = loadTexture(
            J, binaryData,
            metallicRoughness["metallicRoughnessTexture"]["index"]
        );
    }
    else {
        //default color = white
        M.metallicRoughnessTexture = ImageManager::createSolidColorImage(1, 1, 1, 1);
    }

    if (metallicRoughness.hasKey("metallicFactor")) {
        M.metallicFactor = metallicRoughness["metallicFactor"];
    }
    else {
        //default color = white
        M.metallicFactor = 1.0f;
    }

    if (metallicRoughness.hasKey("roughnessFactor")) {
        M.roughnessFactor = metallicRoughness["roughnessFactor"];
    }
    else {
        //default color = white
        M.roughnessFactor = 1.0f;
    }

    return M;
    
}

Primitive* parsePrimitive(const json11::Json& J, std::vector<char>& binaryData, const json11::Json& primitive, VertexManager* vertexManager)
{
    auto mtl = parseMaterial( J, binaryData, primitive["material"] );
    std::vector<vec3> positionData;
    parseAttribute( J, binaryData, primitive["attributes"]["POSITION"], positionData );

    std::vector<vec2> texcoordData;
    parseAttribute( J, binaryData, primitive["attributes"]["TEXCOORD_0"], texcoordData );

    std::vector<vec2> texcoordData2;
    if (primitive["attributes"].hasKey("TEXCOORD_1")) {
        parseAttribute(J, binaryData, primitive["attributes"]["TEXCOORD_1"], texcoordData2);
    }
    else {
        texcoordData2 = texcoordData;
    }

    std::vector<vec3> normalData;
    parseAttribute(J, binaryData, primitive["attributes"]["NORMAL"], normalData);
    std::vector<vec4> tangentData;
    if (primitive["attributes"].hasKey("TANGENT"))
        parseAttribute(J, binaryData, primitive["attributes"]["TANGENT"], tangentData);
    else
        tangentData.resize(positionData.size());
    std::vector<std::uint32_t> indices;
    parseAttribute( J, binaryData, primitive["indices"], indices );
    return new Primitive(
        vertexManager,
        positionData,
        texcoordData,
        texcoordData2,
        normalData,
        tangentData,
        indices, 
        mtl);
}


Mesh* parseMesh(const json11::Json& J, std::vector<char>& binaryData, const json11::Json& node, VertexManager* vertexManager)
{
    int meshIndex = node["mesh"];
    json11::Json mesh = J["meshes"][meshIndex];
    std::string name = mesh["name"];
    Mesh* M = new Mesh(name);
    json11::Json primitives = mesh["primitives"];
    for( const json11::Json& primitive : primitives.array_items()){
        M->primitives.push_back( parsePrimitive(
            J,binaryData,primitive,vertexManager
        ));
    }
    return M;
}

std::vector<Mesh*> load( std::string filename, VertexManager* vertexManager)
{
    std::ifstream fp(filename, std::ios::binary);
    if(!fp.good()){
        std::cout << "Cannot open " << filename << "\n";
        abort();
    }
    char magic[4];
    std::uint32_t version, filelength, jsonlength;
    char jsonmagic[4];

    fp.read(magic,4);
    fp.read( (char*)&version, 4 );
    fp.read( (char*)&filelength, 4 );
    fp.read( (char*)&jsonlength, 4 );
    fp.read( (char*)&jsonmagic, 4 );


    assert(0 == std::memcmp(magic,"glTF",4));
    assert(version == 2 );
    assert(0 == std::memcmp(jsonmagic, "JSON", 4 ));

    std::vector<char> data(jsonlength);
    fp.read( data.data(), jsonlength );
    auto J = json11::Json::parse(std::string(data.data(),jsonlength));

    //8 byte header on binary data; skip it
    fp.seekg(8, std::ios::cur);

    std::vector<char> binaryData(filelength-fp.tellg());
    fp.read( binaryData.data(), binaryData.size() );

    json11::Json scene = J["scenes"][0];
    std::vector<Mesh*> allMeshes;
    json11::Json nodes = J["nodes"];
    for( int nodeIndex : scene["nodes"].array_items() ){
        json11::Json node = nodes[nodeIndex];
        mat4 M = mat4::identity();
        if(node.hasKey( "matrix" )){
            std::vector<float> f(16);
            for(int i=0;i<16;++i){
                f[i] = node["matrix"][i];
            }
            M = mat4( f[0],  f[1],  f[2],  f[3],
                      f[4],  f[5],  f[6],  f[7],
                      f[8],  f[9],  f[10], f[11],
                      f[12], f[13], f[14], f[15] );
            M = transpose(M);
        } else {
            vec3 T(0,0,0);
            if( node.hasKey("translation" )){
                T[0] = node["translation"][0];
                T[1] = node["translation"][1];
                T[2] = node["translation"][2];
            }
            vec4 R(0,0,0,1);
            if( node.hasKey("rotation")){
                R[0] = node["rotation"][0];
                R[1] = node["rotation"][1];
                R[2] = node["rotation"][2];
                R[3] = node["rotation"][3];
            }
            vec3 S(1,1,1);
            if( node.hasKey("scale" )){
                S[0] = node["scale"][0];
                S[1] = node["scale"][1];
                S[2] = node["scale"][2];
            }
            M = scaling(S) * quaternionToMat4(R) * translation(T);
        }

        if( node.hasKey("mesh" )){
            Mesh* m = parseMesh(J,binaryData,node,vertexManager);
            m->worldMatrix = M;
            allMeshes.push_back(m);
        }
    }
    return allMeshes;
}

Light parseLight(json11::Json light, mat4 M)
{
    std::string name=light.get("name","anonymous light");
    vec3 color(1,1,1);
    if( light.hasKey("color") ){
        color.x = light["color"][0];
        color.y = light["color"][1];
        color.z = light["color"][2];
    }
    float intensity=light.get("intensity",1.0f);
    std::string lightType=light["type"];
    float innerAngle,outerAngle;
    if(lightType == "directional"){
        innerAngle=float(std::numbers::pi);
        outerAngle=float(std::numbers::pi);
    } else if( lightType == "point" ){
        innerAngle=float(std::numbers::pi);
        outerAngle=float(std::numbers::pi);
    } else if( lightType == "spot" ){
        innerAngle = light["spot"].get("innerConeAngle",0.0f);
        outerAngle = light["spot"].get("outerConeAngle",float(std::numbers::pi/4.0));
    } else {
        assert(0);
    }
    float cosInnerAngle = (float)std::cos(innerAngle);
    float cosOuterAngle = (float)std::cos(outerAngle);
    vec4 lightPos = vec4(0,0,0,1) * M;
    vec4 lightDirection = vec4(0,0,-1,0) * M;
    return Light(
            name,
            lightPos.xyz(),
            (lightType == "point" || lightType == "spot" ),
            lightDirection.xyz(),
            cosInnerAngle,
            cosOuterAngle,
            color,
            intensity
    );
}

std::vector<Light> getLights(std::string filename)
{
    std::ifstream fp(filename, std::ios::binary);
    if(!fp.good()){
        std::cout << "Cannot open " << filename << "\n";
        abort();
    }
    char magic[4];
    std::uint32_t version, filelength, jsonlength;
    char jsonmagic[4];

    fp.read(magic,4);
    fp.read( (char*)&version, 4 );
    fp.read( (char*)&filelength, 4 );
    fp.read( (char*)&jsonlength, 4 );
    fp.read( (char*)&jsonmagic, 4 );


    assert(0 == std::memcmp(magic,"glTF",4));
    assert(version == 2 );
    assert(0 == std::memcmp(jsonmagic,"JSON",4));

    std::vector<char> data(filelength);
    fp.read( data.data(), filelength );
    auto J = json11::Json::parse(std::string(data.data(),jsonlength));

    json11::Json lights = (
        J.get("extensions")
         .get("KHR_lights_punctual")
         .get("lights")
    );

    std::vector<Light> allLights;

    json11::Json scene = J["scenes"][0];
    json11::Json nodes = J["nodes"];

    for( int nodeIndex : scene["nodes"].array_items() ){
        json11::Json node = nodes[nodeIndex];

        mat4 M = mat4::identity();
        if(node.hasKey( "matrix" )){
            std::vector<float> f(16);
            for(int i=0;i<16;++i){
                f[i] = node["matrix"][i];
            }
            M = mat4( f[0],  f[1],  f[2],  f[3],
                      f[4],  f[5],  f[6],  f[7],
                      f[8],  f[9],  f[10], f[11],
                      f[12], f[13], f[14], f[15] );
            M = transpose(M);
        } else {
            vec3 T(0,0,0);
            if( node.hasKey("translation" )){
                T[0] = node["translation"][0];
                T[1] = node["translation"][1];
                T[2] = node["translation"][2];
            }
            vec4 R(0,0,0,1);
            if( node.hasKey("rotation")){
                R[0] = node["rotation"][0];
                R[1] = node["rotation"][1];
                R[2] = node["rotation"][2];
                R[3] = node["rotation"][3];
            }
            vec3 S(1,1,1);
            if( node.hasKey("scale" )){
                S[0] = node["scale"][0];
                S[1] = node["scale"][1];
                S[2] = node["scale"][2];
            }
            M = scaling(S) * quaternionToMat4(R) * translation(T);
        }

        if( node.get("extensions").hasKey("KHR_lights_punctual") ){
            int lightIndex = node["extensions"]["KHR_lights_punctual"]["light"];
            allLights.push_back( parseLight(lights[lightIndex],M) );
        }
    }
    return allLights;
}

}; //namespace gltf
