#include "BatchSquare.h"
#include "Framework.h"
#include <vector>
#include "shaders/importantconstants.h"

using namespace math2801;

BatchSquare::BatchSquare(VertexManager* vertexManager) {
    

    std::vector<vec3> Pos(BILLBOARD_BATCH_SIZE * 4);
    unsigned int iter = 0;
    for (unsigned int i = 0; i < BILLBOARD_BATCH_SIZE; i++)
    {
        Pos[iter * 4] = vec3(i, 0, 0);
        Pos[iter * 4 + 1] = vec3(i, 0, 0);
        Pos[iter * 4 + 2] = vec3(i, 0, 0);
        Pos[iter * 4 + 3] = vec3(i, 0, 0);
        iter++;
    }

    std::vector<vec2> Tex(BILLBOARD_BATCH_SIZE * 4);
    unsigned int iter2 = 0;
    for (unsigned int i = 0; i < BILLBOARD_BATCH_SIZE; i++)
    {
        Tex[iter2 * 4] = vec2(0, 0);
        Tex[iter2 * 4 + 1] = vec2(1, 0);
        Tex[iter2 * 4 + 2] = vec2(1, 1);
        Tex[iter2 * 4 + 3] = vec2(0, 1);
        iter2++;
    }

    std::vector<std::uint32_t> Ind(BILLBOARD_BATCH_SIZE * 6);
    unsigned int temp3 = 0;
    unsigned int iteration = 0;
    for (unsigned int i = 0; i < BILLBOARD_BATCH_SIZE; i++)
    {
        Ind[temp3 + i] = 0 + (iteration * 4);
        Ind[temp3 + i + 1] = 1 + (iteration * 4);
        Ind[temp3 + i + 2] = 2 + (iteration * 4);
        Ind[temp3 + i + 3] = 0 + (iteration * 4);
        Ind[temp3 + i + 4] = 2 + (iteration * 4);
        Ind[temp3 + i + 5] = 3 + (iteration * 4);
        temp3 += 5;
        iteration++;
    }

    std::vector<vec2> Tex2(BILLBOARD_BATCH_SIZE * 4);
    for (unsigned int i = 0; i < BILLBOARD_BATCH_SIZE * 4; i++)
    {
        Tex2[i] = vec2(0, 0);
    }

    std::vector<vec4> Tan(BILLBOARD_BATCH_SIZE * 4);
    for (unsigned int i = 0; i < BILLBOARD_BATCH_SIZE * 4; i++)
    {
        Tan[i] = vec4(0, 0, 0, 0);
    }

    std::vector<vec3> Norm(BILLBOARD_BATCH_SIZE * 4);
    for (unsigned int i = 0; i < BILLBOARD_BATCH_SIZE * 4; i++)
    {
        Norm[i] = vec3(0, 0, 0);
    }
    
    this->drawinfo = vertexManager->addIndexedData(
        "positions",
        Pos, 

        "texcoords",
        Tex,

        "normals",
        Norm,

        "tangents",
        Tan,

        "texcoords2",
        Tex2,

        "indices",
        Ind
    );
}

void BatchSquare::drawInstanced(VkCommandBuffer cmd,
    unsigned numInstances)
{
    vkCmdDrawIndexed(
        cmd,
        6 * BILLBOARD_BATCH_SIZE,              //index count
        numInstances,              //instance count
        this->drawinfo.indexOffset,
        this->drawinfo.vertexOffset,
        0               //first instance
    );
}