#include "BlitSquare.h"
#include "Framework.h"
#include <vector>
#include "shaders/importantconstants.h"

using namespace math2801;

BlitSquare::BlitSquare(VertexManager* vertexManager) {
    this->drawinfo = vertexManager->addIndexedData(
        "positions",
        std::vector<vec3>{vec3{-1, -1, 0}, vec3{ 1,-1,0 },
        vec3{ 1,1,0 }, vec3{ -1,1,0 }},

        "texcoords",
        std::vector<vec2>{ {0, 0}, { 1,0 }, { 1,1 }, { 0,1 } },

        "normals",
        std::vector<vec3>{ {0, 0, 1}, { 0,0,1 }, { 0,0,1 },
        { 0,0,1 } },

        "tangents",
        std::vector<vec4>{ {0, 0, 0, 0}, { 0,0,0,0 },
        { 0,0,0,0 }, { 0,0,0,0 } },

        "texcoords2",
        std::vector<vec2>{ {0, 0}, { 1,0 }, { 1,1 }, { 0,1 } },

        "indices",
        std::vector<std::uint32_t>{ 0, 1, 2, 0, 2, 3 }
    );
}

void BlitSquare::draw(VulkanContext* ctx, VkCommandBuffer cmd,
    DescriptorSet* descriptorSet, Image* img)
{
    ctx->beginCmdRegion(cmd, "BlitSquare");

    if (img != nullptr) {
        descriptorSet->setSlot(BASE_TEXTURE_SLOT, img->view());
        //these three are dummy
        descriptorSet->setSlot(EMISSIVE_TEXTURE_SLOT, img->view());
        descriptorSet->setSlot(NORMAL_TEXTURE_SLOT, img->view());
        descriptorSet->setSlot(METALLICROUGHNESS_TEXTURE_SLOT, img->view());

        descriptorSet->bind(cmd);
    }
    vkCmdDrawIndexed(
        cmd,
        6,              //index count
        1,              //instance count
        this->drawinfo.indexOffset,
        this->drawinfo.vertexOffset,
        0               //first instance
    );

    ctx->endCmdRegion(cmd);
}

void BlitSquare::drawInstanced(VkCommandBuffer cmd,
    unsigned numInstances)
{
    vkCmdDrawIndexed(
        cmd,
        6,              //index count
        numInstances,              //instance count
        this->drawinfo.indexOffset,
        this->drawinfo.vertexOffset,
        0               //first instance
    );
}