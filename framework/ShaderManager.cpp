#include "vkhelpers.h"
#include "ShaderManager.h"
#include "CleanupManager.h"
#include "consoleoutput.h"
#include "utils.h"
#include "mischelpers.h"
#include <stdexcept>
#include <vector>
#include <glslang/Public/ShaderLang.h>
#include <glslang/SPIRV/GlslangToSpv.h>
#include <iostream>
#include <map>
#include <stdexcept>
#include <fstream>
#include <filesystem>
#include <cassert>

// ref: https://vulkan.lunarg.com/doc/sdk/1.3.236.0/windows/spirv_toolchain.html
// ref: https://github.com/KhronosGroup/glslang/blob/master/StandAlone/StandAlone.cpp
// ref: https://github.com/KhronosGroup/glslang/blob/master/StandAlone/ResourceLimits.cpp

static VulkanContext* ctx;
static std::vector<VkPipelineShaderStageCreateInfo> _shaders;
static std::list<std::vector<unsigned> > codes;




static std::map<std::string,EShLanguage> typeMap = {
    {"vert", EShLangVertex},
    {"frag", EShLangFragment},
    {"tesc", EShLangTessControl},
    {"tese", EShLangTessEvaluation},
    {"geom", EShLangGeometry},
    {"comp", EShLangCompute}
};
static std::map<std::string, VkShaderStageFlagBits> stages = {
    { "vert", VK_SHADER_STAGE_VERTEX_BIT },
    { "frag", VK_SHADER_STAGE_FRAGMENT_BIT },
    { "tesc", VK_SHADER_STAGE_TESSELLATION_CONTROL_BIT },
    { "tese", VK_SHADER_STAGE_TESSELLATION_EVALUATION_BIT },
    { "geom", VK_SHADER_STAGE_GEOMETRY_BIT },
    { "comp", VK_SHADER_STAGE_COMPUTE_BIT }
};

static VkPipelineShaderStageCreateInfo makeModule(std::vector<unsigned>& code,
            const std::string type, const std::string debuggingName )
{
    if(!ctx)
        fatal("ShaderManager::initialize() has not been called yet");

    codes.push_back(code);

    VkShaderModule module;
    check(vkCreateShaderModule(
        ctx->dev,
        VkShaderModuleCreateInfo{
            .sType=VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO,
            .pNext=nullptr,
            .flags=0,
            .codeSize = (unsigned)code.size()*4,
            .pCode = (std::uint32_t*)codes.back().data()
        },
        nullptr,
        &(module)
    ));

    ctx->setObjectName(module,debuggingName);

    assert(stages.contains(type));

    VkPipelineShaderStageCreateInfo info{
        .sType=VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
        .pNext=nullptr,
        .flags=0,
        .stage=stages[type],
        .module=module,
        .pName="main",
        .pSpecializationInfo=nullptr
    };

    _shaders.push_back(info);

    return info;
}

class MyIncluder : public glslang::TShader::Includer {
  public:
    //~ std::string directory;     //working directory for includes
    std::list<std::string> includedData;    //list so it doesn't move in RAM

    MyIncluder(){
    }
    glslang::TShader::Includer::IncludeResult* includeSystem(
        const char* ,
        const char* ,
        std::size_t ) override {
        return nullptr;
    }
    glslang::TShader::Includer::IncludeResult* includeLocal(const char* headerName,
            const char* includerName_, std::size_t inclusionDepth) override {

        verbose(
            std::string("Including '") +
                headerName + "' from '"+
                includerName_+"' (depth "+
                std::to_string(inclusionDepth)+")"
        );

        if( inclusionDepth > 100 ){
            includedData.push_back(
                std::string(
                    "Too many nested includes"
                )
            );
            error("Too many nested includes in ",headerName);
            return new IncludeResult(
                "",
                this->includedData.back().c_str(),
                this->includedData.back().length(),
                nullptr
            );
        }


        //headerName is the path of the thing that we want to include
        //includerName is the path of the file that is including it
        std::string fullpath;
        std::string includerName = includerName_;
        auto idx = includerName.rfind('/');
        if( idx == std::string::npos ){
            fullpath = "./";
        } else {
            fullpath = includerName.substr(0,idx)+"/";
        }
        fullpath += headerName;

        std::ifstream in(fullpath);
        if(!in.good() ){
            error("Could not read file '"+fullpath+"' included from '"+includerName_+"'");
            this->includedData.push_back( std::string(
                "Could not read file "+fullpath+" included from "+includerName_)
            );
            return new IncludeResult(
                "",
                this->includedData.back().c_str(),
                this->includedData.back().length(),
                nullptr
            );
        }

        std::string shaderData;
        std::getline( in, shaderData, '\0' );
        this->includedData.push_back( shaderData );

        std::filesystem::path pp(fullpath);
        pp = std::filesystem::canonical(pp);
        std::string p = pp.generic_string();
        return new IncludeResult(
            (std::string)(p),
            this->includedData.back().c_str(),
            this->includedData.back().length(),
            nullptr
        );
    }

    void releaseInclude(IncludeResult* r){
        if(r)
            delete r;
    }
};

static
VkPipelineShaderStageCreateInfo doCompile( const std::string& src,
        const std::string& filename, const std::string& type)
{
    EShLanguage lang = typeMap[type];
    glslang::TShader shader(lang);

    const char* S[1] = { src.c_str() };
    const int L[1] = { (int)src.length() };
    const char* N[1] = { filename.c_str() };
    shader.setStringsWithLengthsAndNames(S,L,N,1);

    shader.setEnvInput( glslang::EShSourceGlsl, lang, glslang::EShClientVulkan, 100 );
    shader.setEnvClient( glslang::EShClientVulkan, glslang::EShTargetVulkan_1_0 );
    shader.setEnvTarget( glslang::EShTargetSpv, glslang::EShTargetSpv_1_0 );

    MyIncluder incl;





    //some limits are from the defaults at
    //https://github.com/KhronosGroup/glslang/blob/master/StandAlone/ResourceLimits.cpp
    //New URL:https://github.com/KhronosGroup/glslang/blob/main/glslang/ResourceLimits/ResourceLimits.cpp
    TBuiltInResource builtins;
    std::memset(&builtins,0,sizeof(builtins));
    builtins.maxLights = 32;
    builtins.maxClipPlanes = 6;
    builtins.maxTextureUnits = 32;
    builtins.maxTextureCoords = 32;
    builtins.maxVertexAttribs = 64;
    builtins.maxVertexUniformComponents = 4096;
    builtins.maxVaryingFloats = 64;
    builtins.maxVertexTextureImageUnits = 32;
    builtins.maxCombinedTextureImageUnits = 80;
    builtins.maxTextureImageUnits = 32;
    builtins.maxFragmentUniformComponents = 4096;
    builtins.maxDrawBuffers = 32;
    builtins.maxVertexUniformVectors = 128;
    builtins.maxVaryingVectors = 8;
    builtins.maxFragmentUniformVectors = 16;
    builtins.maxVertexOutputVectors = 16;
    builtins.maxFragmentInputVectors = 15;
    builtins.minProgramTexelOffset = -8;
    builtins.maxProgramTexelOffset = 7;
    builtins.maxClipDistances = 8;
    builtins.maxComputeWorkGroupCountX = 65535;
    builtins.maxComputeWorkGroupCountY = 65535;
    builtins.maxComputeWorkGroupCountZ = 65535;
    builtins.maxComputeWorkGroupSizeX = 1024;
    builtins.maxComputeWorkGroupSizeY = 1024;
    builtins.maxComputeWorkGroupSizeZ = 64;
    builtins.maxComputeUniformComponents = 1024;
    builtins.maxComputeTextureImageUnits = 16;
    builtins.maxComputeImageUniforms = 8;
    builtins.maxComputeAtomicCounters = 8;
    builtins.maxComputeAtomicCounterBuffers = 1;
    builtins.maxVaryingComponents = 60;
    builtins.maxVertexOutputComponents = 64;
    builtins.maxGeometryInputComponents = 64;
    builtins.maxGeometryOutputComponents = 128;
    builtins.maxFragmentInputComponents = 128;
    builtins.maxImageUnits = 8;
    builtins.maxCombinedImageUnitsAndFragmentOutputs = 8;
    builtins.maxCombinedShaderOutputResources = 8;
    builtins.maxImageSamples = 0;
    builtins.maxVertexImageUniforms = 0;
    builtins.maxTessControlImageUniforms = 0;
    builtins.maxTessEvaluationImageUniforms = 0;
    builtins.maxGeometryImageUniforms = 0;
    builtins.maxFragmentImageUniforms = 8;
    builtins.maxCombinedImageUniforms = 8;
    builtins.maxGeometryTextureImageUnits = 16;
    builtins.maxGeometryOutputVertices = 256;
    builtins.maxGeometryTotalOutputComponents = 1024;
    builtins.maxGeometryUniformComponents = 64;
    builtins.maxGeometryVaryingComponents = 64;
    builtins.maxTessControlInputComponents = 128;
    builtins.maxTessControlOutputComponents = 128;
    builtins.maxTessControlTextureImageUnits = 16;
    builtins.maxTessControlUniformComponents = 1024;
    builtins.maxTessControlTotalOutputComponents = 4096;
    builtins.maxTessEvaluationInputComponents = 128;
    builtins.maxTessEvaluationOutputComponents = 128;
    builtins.maxTessEvaluationTextureImageUnits = 16;
    builtins.maxTessEvaluationUniformComponents = 1024;
    builtins.maxTessPatchComponents = 120;
    builtins.maxPatchVertices = 32;
    builtins.maxTessGenLevel = 64;
    builtins.maxViewports = 16;
    builtins.maxVertexAtomicCounters = 0;
    builtins.maxTessControlAtomicCounters = 0;
    builtins.maxTessEvaluationAtomicCounters = 0;
    builtins.maxGeometryAtomicCounters = 0;
    builtins.maxFragmentAtomicCounters = 8;
    builtins.maxCombinedAtomicCounters = 8;
    builtins.maxAtomicCounterBindings = 1;
    builtins.maxVertexAtomicCounterBuffers = 0;
    builtins.maxTessControlAtomicCounterBuffers = 0;
    builtins.maxTessEvaluationAtomicCounterBuffers = 0;
    builtins.maxGeometryAtomicCounterBuffers = 0;
    builtins.maxFragmentAtomicCounterBuffers = 1;
    builtins.maxCombinedAtomicCounterBuffers = 1;
    builtins.maxAtomicCounterBufferSize = 16384;
    builtins.maxTransformFeedbackBuffers = 4;
    builtins.maxTransformFeedbackInterleavedComponents = 64;
    builtins.maxCullDistances = 8;
    builtins.maxCombinedClipAndCullDistances = 8;
    builtins.maxSamples = 4;
    builtins.maxMeshOutputVerticesNV = 256;
    builtins.maxMeshOutputPrimitivesNV = 512;
    builtins.maxMeshWorkGroupSizeX_NV = 32;
    builtins.maxMeshWorkGroupSizeY_NV = 1;
    builtins.maxMeshWorkGroupSizeZ_NV = 1;
    builtins.maxTaskWorkGroupSizeX_NV = 32;
    builtins.maxTaskWorkGroupSizeY_NV = 1;
    builtins.maxTaskWorkGroupSizeZ_NV = 1;
    builtins.maxMeshViewCountNV = 4;
    builtins.maxDualSourceDrawBuffersEXT = 1;
    builtins.limits.nonInductiveForLoops = true;
    builtins.limits.whileLoops = true;
    builtins.limits.doWhileLoops = true;
    builtins.limits.generalUniformIndexing = true;
    builtins.limits.generalAttributeMatrixVectorIndexing = true;
    builtins.limits.generalVaryingIndexing = true;
    builtins.limits.generalSamplerIndexing = true;
    builtins.limits.generalVariableIndexing = true;
    builtins.limits.generalConstantMatrixVectorIndexing = true;

    bool ok = shader.parse(
            &builtins,
            450,        //default version
            ECoreProfile,
            true,       //forceDefaultVersionAndProfile
            false,      //forwardCompatible
            EShMsgEnhanced,     //EShMsgDefault
            incl
    );

    const char* tmp = shader.getInfoLog();
    std::string infoLog;
    if(tmp)
        infoLog = tmp;
    tmp = shader.getInfoDebugLog();
    if(tmp){
        if(infoLog.length())
            infoLog += "\n";
        infoLog += tmp;
    }

    infoLog = trim(infoLog);

    if( infoLog.length() ){
        if( !ok ){
            error("When compiling",type,"shader '"+filename+"':");
            error(infoLog);
        } else {
            warn("When compiling",type,"shader '"+filename+"':");
            warn(infoLog);
        }
    }

    infoLog="";

    if( !ok ){
        error("Shader compilation failed");
        std::exit(1);
    }

    glslang::TProgram prog;  //must declare after shader so it gets destructed first
    prog.addShader(&shader);
    ok = prog.link(EShMsgEnhanced); //EShMsgDefault

    tmp = prog.getInfoLog();
    if(tmp){
        infoLog += tmp;
    }

    tmp = prog.getInfoDebugLog();
    if(tmp){
        infoLog += tmp;
    }

    infoLog = trim(infoLog);

    if( infoLog.length() ){
        if(!ok){
            error("When compiling",type.substr(1),"shader '"+filename+"':");
            error(infoLog);
        } else {
            warn("When compiling",type.substr(1),"shader '"+filename+"':");
            warn(infoLog);
        }
    }

    if( !ok ){
        error("Shader link failed");
        std::exit(1);
    }

    glslang::TIntermediate* intermediate = prog.getIntermediate(lang);
    std::vector<unsigned int> spirv;
    glslang::GlslangToSpv( *intermediate, spirv );

    return makeModule(spirv,type,filename);
}

namespace ShaderManager {

bool initialized()
{
    return ctx != nullptr;
}

void initialize(VulkanContext* ctx_)
{
    if(initialized())
        return;

    ctx=ctx_;
    glslang::InitializeProcess();
    CleanupManager::registerCleanupFunction([](){
        for(auto& s : _shaders ){
            vkDestroyShaderModule(ctx->dev, s.module, nullptr);
        }
        glslang::FinalizeProcess();
    });
}

VkPipelineShaderStageCreateInfo load(std::string filename)
{

    std::string suffix;

    auto dotpos = filename.rfind('.');
    if(dotpos != std::string::npos ){
        suffix = filename.substr(dotpos+1);
    }

    if( !typeMap.contains(suffix) ){
        fatal("Shader filename must end with one of: .vert, .frag, .tesc, .tese, .geom, .comp");
    }

    return load(filename,suffix);
}

VkPipelineShaderStageCreateInfo load(std::string filename, std::string type)
{
    if( !typeMap.contains(type) ){
        fatal("Type must be one of: vert, frag, tesc, tese, geom, comp");
    }

    std::ifstream in(filename);
    if(!in.good())
        fatal("Cannot read shader file "+filename);

    std::string src;
    std::getline( in, src, '\0');

    return doCompile(src, filename, type );
}

VkPipelineShaderStageCreateInfo loadFromString(std::string src, std::string type, std::string name)
{
    if( !typeMap.contains(type) ){
        fatal("Shader type must be one of: vert, frag, tesc, tese, geom, comp");
    }

    return doCompile(src, name, type );
}

}; //namespace
