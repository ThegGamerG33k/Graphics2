#include "Framework.h"
#include "math2801.h"
#include "Globals.h"
#include "Uniforms.h"
#include "gltf.h"
#include "Text.h"
#include "shaders/importantconstants.h"
#include "BlitSquare.h"
#include "BillboardCollection.h"

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

    globs->uniforms->set( "attenuation", vec3(500,0,1) );
    globs->uniforms->set( "lightPositionAndDirectionalFlag", globs->lightPositionAndDirectionalFlag );
    globs->uniforms->set( "lightColorAndIntensity", globs->lightColorAndIntensity );
    globs->uniforms->set( "cosSpotAngles", globs->cosSpotAngles );
    globs->uniforms->set( "spotDirection", globs->spotDirection );
    globs->uniforms->set( "elapsed", globs->elapsed );
    globs->camera->setUniforms(globs->uniforms);
    globs->uniforms->bind(cmd,globs->descriptorSet);

    //globs->descriptorSet->bind(cmd);      //uniform bind does this already
    globs->vertexManager->bindBuffers(cmd);


    globs->descriptorSet->setSlot(
        ENVMAP_TEXTURE_SLOT,
        globs->interiorEnvironmentMap->view()
    );

    globs->pipeline->use(cmd);
    for(auto& m : globs->room ){
        m->draw(globs->ctx,cmd,globs->descriptorSet,globs->pushConstants);
    }

    globs->firePipe->use(cmd);
    globs->pushConstants->set(cmd, "worldMatrix",
            scaling(0.5f,0.5f,0.5f) * translation(-0.5,0.5,-0.5)
    );
    globs->blitSquare->draw(
        globs->ctx,
        cmd,
        globs->descriptorSet,
        globs->fireTexture
    );

    globs->skyboxPipeline->use(cmd);
    globs->descriptorSet->setSlot(
        ENVMAP_TEXTURE_SLOT,
        globs->skyboxEnvironmentMap->view()
    );
    globs->cubeMesh->draw(globs->ctx, cmd, globs->descriptorSet, globs->pushConstants);
     
    globs->pipelineDrawBillboards->use(cmd);
    //can translate entire billboard system around as a unit
    //with worldMatrix
    globs->pushConstants->set(cmd, "worldMatrix",
        mat4::identity());
    globs->billboardCollection->draw(cmd, globs->descriptorSet);

    globs->text->draw(cmd);

    globs->framebuffer->endRenderPass(cmd);
    vkEndCommandBuffer(cmd);

    utils::endFrame(globs->ctx,cmd);
}
