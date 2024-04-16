#pragma once

#include <set>
#include <vector>

#include "math2801.h"
using namespace math2801;

class Text;
class Camera;
class VulkanContext;
class DescriptorSet;
class Sampler;
class VertexManager;
class Uniforms;
class Pipeline;
class PushConstants;
class Framebuffer;
class DescriptorSetLayout;
class DescriptorSet;
class DescriptorSetFactory;
class PipelineLayout;
class Image;
class BlitSquare;
class BlurrableFramebuffer;
class BillboardCollection;
class GraphicsPipeline;

namespace gltf{
  class Mesh;
};

class Globals{
  public:
    int width;
    int height;
    std::set<int> pressedKeys;
    bool keepLooping=true;
    VulkanContext* ctx;
    bool mouseLook = true;
    Camera* camera;
    Text* text;
    Framebuffer* framebuffer;
    DescriptorSet* descriptorSet;
    VertexManager* vertexManager;
    BillboardCollection* billboardCollection;
    Uniforms* uniforms;
    Pipeline* pipeline;
    GraphicsPipeline* pipelineDrawBillboards;
    PushConstants* pushConstants;
    std::vector<gltf::Mesh*> room;
    std::vector<vec4> lightPositionAndDirectionalFlag;
    std::vector<vec4> lightColorAndIntensity;
    std::vector<vec4> cosSpotAngles;
    std::vector<vec4> spotDirection;
    Sampler* nearestSampler;
    Sampler* linearSampler;
    Sampler* mipSampler;
    Sampler* clampingMipSampler;
    DescriptorSetLayout* descriptorSetLayout;
    PipelineLayout* pipelineLayout;
    DescriptorSetFactory* descriptorSetFactory;

    Pipeline* skyboxPipeline;
    Image* interiorEnvironmentMap;
    Image* skyboxEnvironmentMap;
    gltf::Mesh* cubeMesh;

    Image* fireTexture;
    Pipeline* firePipe;
    BlitSquare* blitSquare;
    float elapsed=0.0f;

};
