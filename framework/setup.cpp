#include "Globals.h"   
#include "Framework.h"
#include "BillboardCollection.h"
#include "shaders/importantconstants.h"
#include "Camera.h"
#include "Light.h"
#include <SDL.h>
#include "gltf.h"
#include "Text.h"

void setup(Globals* globs)
{
    ShaderManager::initialize(globs->ctx);
    ImageManager::initialize(globs->ctx);
    globs->envmapImage = ImageManager::loadCube({
        "assets/px.jpg",
        "assets/nx.jpg",
        "assets/py.jpg",
        "assets/ny.jpg",
        "assets/pz.jpg",
        "assets/nz.jpg"
        });

    globs->skyboxImage = ImageManager::loadCube({
        "assets/nebula1_0.jpg",
        "assets/nebula1_1.jpg",
        "assets/nebula1_2.jpg",
        "assets/nebula1_3.jpg",
        "assets/nebula1_4.jpg",
        "assets/nebula1_5.jpg"
        });
    

    globs->keepLooping=true;
    globs->framebuffer = new Framebuffer(globs->ctx);
    


    globs->nearestSampler = new Sampler( globs->ctx,
            VK_FILTER_NEAREST,
            VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE,
            false
    );
    globs->linearSampler = new Sampler( globs->ctx,
            VK_FILTER_LINEAR,
            VK_SAMPLER_ADDRESS_MODE_REPEAT,
            false
    );
    globs->mipSampler = new Sampler( globs->ctx,
            VK_FILTER_LINEAR,
            VK_SAMPLER_ADDRESS_MODE_REPEAT,
            true
    );

    globs->camera = new Camera(
        vec3(0,1,2),    //eye
        vec3(0,0,0),    //coi
        vec3(0,1,0),    //up
        radians(35.0f), //field of view
        1.0f,           //aspect ratio
        0.1f,           //hither
        100.0f          //yon
    );

    globs->vertexManager = new VertexManager(
        globs->ctx,
        {
            VertexManager::Input{
                .name="positions",
                .format=VK_FORMAT_R32G32B32_SFLOAT,
                .location=VS_INPUT_POSITION,
                .rate=VK_VERTEX_INPUT_RATE_VERTEX
            },
            VertexManager::Input{
                .name="texcoords",
                .format=VK_FORMAT_R32G32_SFLOAT,
                .location=VS_INPUT_TEXCOORD,
                .rate=VK_VERTEX_INPUT_RATE_VERTEX
            },
            VertexManager::Input{
                .name = "texcoords2",
                .format = VK_FORMAT_R32G32_SFLOAT,
                .location = VS_INPUT_TEXCOORD2,
                .rate = VK_VERTEX_INPUT_RATE_VERTEX
            },
            VertexManager::Input{
                .name="normals",
                .format=VK_FORMAT_R32G32B32_SFLOAT,
                .location=VS_INPUT_NORMAL,
                .rate=VK_VERTEX_INPUT_RATE_VERTEX
            },
            VertexManager::Input{
                .name = "tangents",
                .format = VK_FORMAT_R32G32B32A32_SFLOAT,
                .location = VS_INPUT_TANGENT,
                .rate = VK_VERTEX_INPUT_RATE_VERTEX
            }
        }
    );


    globs->descriptorSetLayout = new DescriptorSetLayout(
        globs->ctx,
        {
            DescriptorSetEntry( VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
                                UNIFORM_SLOT ),
            DescriptorSetEntry( VK_DESCRIPTOR_TYPE_SAMPLER,
                                NEAREST_SAMPLER_SLOT ),
            DescriptorSetEntry( VK_DESCRIPTOR_TYPE_SAMPLER,
                                LINEAR_SAMPLER_SLOT ),
            DescriptorSetEntry( VK_DESCRIPTOR_TYPE_SAMPLER,
                                MIPMAP_SAMPLER_SLOT ),
            DescriptorSetEntry( VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE,
                                BASE_TEXTURE_SLOT ),
            DescriptorSetEntry( VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE,
                                EMISSIVE_TEXTURE_SLOT ),
            DescriptorSetEntry(VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE,
                                NORMAL_TEXTURE_SLOT),
            DescriptorSetEntry(VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE,
                                METALLICROUGHNESS_TEXTURE_SLOT),
            DescriptorSetEntry(VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE,
                                ENVMAP_TEXTURE_SLOT),
            DescriptorSetEntry(VK_DESCRIPTOR_TYPE_UNIFORM_TEXEL_BUFFER,
                                BILLBOARD_TEXTURE_SLOT),

        }
    );


    globs->pushConstants = new PushConstants(
        "shaders/pushconstants.txt"
    );

    globs->pipelineLayout = new PipelineLayout(
        globs->ctx,
        globs->pushConstants->byteSize,
        globs->descriptorSetLayout,
        "default layout");

    globs->pipeline = new GraphicsPipeline(
        globs->ctx,
        "main pipe",
        globs->pipelineLayout,
        PipelineOption{.shader=ShaderManager::load("shaders/main.vert")},
        PipelineOption{.shader=ShaderManager::load("shaders/main.frag")},
        PipelineOption{.vertexInputState=globs->vertexManager->inputState}
    );

    globs->skyboxPipeline = new GraphicsPipeline(
        globs->ctx,
        "skybox pipe",
        globs->pipelineLayout,
        PipelineOption{ .shader = ShaderManager::load("shaders/sky.vert") },
        PipelineOption{ .shader = ShaderManager::load("shaders/sky.frag") },
        PipelineOption{ .vertexInputState = globs->vertexManager->inputState }
    );

    globs->descriptorSetFactory = new DescriptorSetFactory(
        globs->ctx,
       "main",
        0,      //binding point
        globs->pipelineLayout
    );
    globs->descriptorSet = globs->descriptorSetFactory->make();

    globs->uniforms = new Uniforms(
        globs->ctx,
        UNIFORM_SLOT,
        "shaders/uniforms.txt",
        "main uniforms"
    );

    std::vector<vec4> rand_pos;
    for (int i = 0; i < 1024; i++)
    {
        rand_pos.push_back(vec4(-5 + static_cast <float> (rand()) / static_cast <float> (RAND_MAX/(5-(-5))), -5 + static_cast <float> (rand()) / static_cast <float> (RAND_MAX / (5 - (-5))), -5 + static_cast <float> (rand()) / static_cast <float> (RAND_MAX / (5 - (-5))), 1));
    }

    globs->billboardCollection = new BillboardCollection(
        globs->ctx,
        globs->vertexManager,
        rand_pos,
        ImageManager::load("assets/nova.png")
    );

    globs->pipelineDrawBillboards = new GraphicsPipeline(
        globs->ctx,
        "draw billboards",
        globs->pipelineLayout,
        PipelineOption{ .shader = ShaderManager::load("shaders/billboard.vert") },
        PipelineOption{ .shader = ShaderManager::load("shaders/billboard.frag") },
        PipelineOption{ .depthWriteEnable = VK_FALSE },
        PipelineOption{ .blendEnable = VK_TRUE },
        PipelineOption{ .srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA },
        PipelineOption{ .dstColorBlendFactor = VK_BLEND_FACTOR_ONE },   //<---
        PipelineOption{ .vertexInputState = globs->vertexManager->inputState }
    );

    globs->room = gltf::load("assets/room6.glb",globs->vertexManager);
    auto lights = gltf::getLights("assets/room6.glb");
    for( Light L : lights ){

        //xyz=position if positional; direction if directional. w=flag
        globs->lightPositionAndDirectionalFlag.push_back(
            vec4( L.position.x, L.position.y, L.position.z,
                  (L.positional ? 1.0f : 0.0f )
            )
        );

        //xyz=color, w=intensity
        globs->lightColorAndIntensity.push_back(
            vec4( L.color, L.intensity )
        );

        //x=inner, y=outer, zw=ignored
        globs->cosSpotAngles.push_back(
            vec4(L.cosInnerSpotAngle, L.cosOuterSpotAngle, 0,0 )
        );

        //xyz=direction, w=ignored
        globs->spotDirection.push_back(
            vec4(L.direction.xyz(), 0)
        );
    }

    globs->text = new Text(globs->ctx, globs->framebuffer,
        "assets/writing36.png",
        "assets/writing36.txt"
    );

    globs->skyboxCube = gltf::load("assets/cube.glb", globs->vertexManager)[0];

    globs->vertexManager->pushToGPU();
    ImageManager::pushToGPU();
    globs->descriptorSet->setSlot( NEAREST_SAMPLER_SLOT, globs->nearestSampler->sampler );
    globs->descriptorSet->setSlot( LINEAR_SAMPLER_SLOT, globs->linearSampler->sampler );
    globs->descriptorSet->setSlot( MIPMAP_SAMPLER_SLOT, globs->mipSampler->sampler );

    SDL_SetRelativeMouseMode(SDL_TRUE);
    globs->mouseLook = true;
}
