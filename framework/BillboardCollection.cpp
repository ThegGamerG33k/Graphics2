#include "BillboardCollection.h"
#include "VertexManager.h"
#include "Buffers.h"
#include "BatchSquare.h"
#include "Descriptors.h"
#include "Samplers.h"
#include "Images.h"
#include "shaders/importantconstants.h"
#include "CleanupManager.h"
#include "Globals.h"

BillboardCollection::BillboardCollection(
    VulkanContext* ctx,
    VertexManager* vertexManager,
    const std::vector<math2801::vec4>& positions_,
    Image* img_)
{
    if (positions_.size() % BILLBOARD_BATCH_SIZE) {
        //fatal() is in consoleoutput.h
        fatal("Bad position count:", positions_.size(),
            "is not a multiple of", BILLBOARD_BATCH_SIZE);
    }
    this->positions = new DeviceLocalBuffer(ctx, positions_,
        VK_BUFFER_USAGE_UNIFORM_TEXEL_BUFFER_BIT,
        "BillboardCollection positions"
    );
    this->positionView = this->positions->makeView(
        VK_FORMAT_R32G32B32A32_SFLOAT,
        "bb positionView"
    );
    this->img = img_;
    this->numBillboards = (unsigned)positions_.size();
    this->batchSquare = new BatchSquare(vertexManager);
    this->ctx = ctx;

    CleanupManager::registerCleanupFunction([this]() {
        this->positions->cleanup();
        vkDestroyBufferView(this->ctx->dev, this->positionView, nullptr);
        });
}

void BillboardCollection::draw(VkCommandBuffer cmd,
    DescriptorSet* descriptorSet)
{
    //for Renderdoc debugging
    ctx->beginCmdRegion(cmd, "Draw billboards");

    descriptorSet->setSlot(BASE_TEXTURE_SLOT, this->img->view());
    descriptorSet->setSlot(BILLBOARD_TEXTURE_SLOT, this->positionView);
    descriptorSet->bind(cmd);
    //we don't want blitSquare to set images, so we pass in null
    this->batchSquare->drawInstanced(cmd, this->numBillboards);

    //for Renderdoc debugging
    ctx->endCmdRegion(cmd);

}