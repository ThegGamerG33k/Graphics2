#include "RenderPass.h"
#include "CleanupManager.h"
#include <cassert>
#include <stdexcept>

RenderPass::RenderPass(
    VulkanContext* ctx_,
    VkFormat colorFormat,
    int numLayers_,
    VkFormat depthFormat,
    VkImageLayout initialLayoutColor,
    VkImageLayout initialLayoutDepth,
    VkImageLayout finalLayoutColor,
    VkImageLayout finalLayoutDepth,
    VkAttachmentLoadOp lop,
    std::string name_)
{

    //FIXME: if we want to support multiple subpasses with LOAD_OP_LOAD, we must change
    //the initiallayout from UNDEFINED to PRESENT_SRC_KHR/DEPTH_STENCIL_ATTACHMENT_OPTIMAL

    this->ctx=ctx_;
    this->numLayers=numLayers_;
    this->name=name_;

    std::vector<VkAttachmentReference> colorAttachments(numLayers);
    for(int i=0;i<numLayers;++i){
        colorAttachments[i] = VkAttachmentReference{
            .attachment=(unsigned)i,
            //layout during the render
            .layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL
        };
    }

    VkAttachmentReference depthStencilAttachment{
        .attachment=(unsigned)numLayers,
        //layout during the render
        .layout=VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL
    };

    VkSubpassDescription subpassDescription{
        .flags=0,
        .pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS,
        .inputAttachmentCount=0,
        .pInputAttachments=nullptr,
        .colorAttachmentCount = (unsigned)colorAttachments.size(),
        .pColorAttachments = colorAttachments.data(),
        .pResolveAttachments=nullptr,
        .pDepthStencilAttachment=&depthStencilAttachment,
        .preserveAttachmentCount=0,
        .pPreserveAttachments=nullptr
    };

    VkSubpassDependency subpassDependency{
        .srcSubpass = VK_SUBPASS_EXTERNAL,
        .dstSubpass=0,
        .srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT | VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT | VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT ,
        .dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT | VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT | VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT,
        .srcAccessMask = VK_ACCESS_MEMORY_READ_BIT | VK_ACCESS_MEMORY_WRITE_BIT ,
        .dstAccessMask = VK_ACCESS_MEMORY_READ_BIT | VK_ACCESS_MEMORY_WRITE_BIT,
        .dependencyFlags=0
    };

    //+1 for depth buffer
    std::vector<VkAttachmentDescription> attachmentDescriptions(numLayers+1);

    for(unsigned i=0;i<(unsigned)numLayers;++i){
        attachmentDescriptions[i] = VkAttachmentDescription{
            .flags=0,
            .format = colorFormat,
            .samples = VK_SAMPLE_COUNT_1_BIT,
            .loadOp = lop,
            .storeOp = VK_ATTACHMENT_STORE_OP_STORE,
            .stencilLoadOp = lop,
            .stencilStoreOp = VK_ATTACHMENT_STORE_OP_STORE,
            .initialLayout = initialLayoutColor,
            .finalLayout = finalLayoutColor
        };
    }

    attachmentDescriptions[numLayers] = VkAttachmentDescription{
        .flags=0,
        .format=depthFormat,
        .samples=VK_SAMPLE_COUNT_1_BIT,
        .loadOp=lop,
        .storeOp=VK_ATTACHMENT_STORE_OP_STORE,
        .stencilLoadOp=lop,
        .stencilStoreOp=VK_ATTACHMENT_STORE_OP_STORE,
        .initialLayout=initialLayoutDepth,
        .finalLayout=finalLayoutDepth
    };


    check(vkCreateRenderPass(
        ctx->dev,
        VkRenderPassCreateInfo{
            .sType=VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO,
            .pNext=nullptr,
            .flags=0,
            .attachmentCount=(unsigned)attachmentDescriptions.size(),
            .pAttachments = attachmentDescriptions.data(),
            .subpassCount = 1,
            .pSubpasses = &subpassDescription,
            .dependencyCount= 1,
            .pDependencies = &subpassDependency
        },
        nullptr,
        &this->renderPass
    ));

    this->ctx->setObjectName(renderPass,name);

    CleanupManager::registerCleanupFunction([this](){
        vkDestroyRenderPass(this->ctx->dev,this->renderPass,nullptr);
    });

}
