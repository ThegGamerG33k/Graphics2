#include "Framework.h"
#include "math2801.h"
#include "Globals.h"
#include "Uniforms.h"
#include "gltf.h"
#include "Text.h"
#include "BillboardCollection.h"
#include "..\shaders\importantconstants.h"

void draw(Globals* globs)
{
    utils::beginFrame(globs->ctx);

    auto cmd = CommandBuffer::allocate();


    VkCommandBufferBeginInfo beginInfo {
        .sType=VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
        .pNext=nullptr,
        .flags=0,
        .pInheritanceInfo=nullptr
    };
    vkBeginCommandBuffer(cmd,&beginInfo);

    globs->text->update(cmd);

    globs->framebuffer->beginRenderPassClearContents(cmd, 0.2f, 0.4f, 0.6f, 1.0f );

    globs->descriptorSet->setSlot(
        ENVMAP_TEXTURE_SLOT,
        globs->envmapImage->view()
    );

    globs->descriptorSet->bind(cmd);
    globs->vertexManager->bindBuffers(cmd);

    globs->camera->setUniforms(globs->uniforms);

    globs->uniforms->set( "attenuation", vec3(1,0,500) );
    globs->uniforms->set( "lightPositionAndDirectionalFlag", globs->lightPositionAndDirectionalFlag );
    globs->uniforms->set( "lightColorAndIntensity", globs->lightColorAndIntensity );
    globs->uniforms->set( "cosSpotAngles", globs->cosSpotAngles );
    globs->uniforms->set( "spotDirection", globs->spotDirection );

    globs->uniforms->bind(cmd,globs->descriptorSet);

    globs->pipeline->use(cmd);

    for(auto& m : globs->room ){
        m->draw(globs->ctx,cmd,globs->descriptorSet,globs->pushConstants);
    }
    
    globs->skyboxPipeline->use(cmd);
    globs->descriptorSet->setSlot(
        ENVMAP_TEXTURE_SLOT,
        globs->skyboxImage->view()
    );
    globs->skyboxCube->draw(
        globs->ctx,
        cmd,
        globs->descriptorSet,
        globs->pushConstants
    );

    globs->pipelineDrawBillboards->use(cmd);
    //can translate entire billboard system around as a unit
    //with worldMatrix
    globs->pushConstants->set(cmd, "worldMatrix",
        mat4::identity());
    globs->billboardCollection->draw(cmd, globs->descriptorSet);

    globs->text->draw(cmd);

    globs->particleSystem->draw(cmd);

    globs->framebuffer->endRenderPass(cmd);
    vkEndCommandBuffer(cmd);

    utils::endFrame(globs->ctx,cmd);
}

