#include "BatchSquare.h" 
#include "Framework.h"
#include <vector>
#include "shaders/importantconstants.h"

using namespace math2801;

BatchSquare::BatchSquare(VertexManager* vertexManager) {
    std::vector<vec3> P;
    int x = 0;
    for (int i = 0; i < BILLBOARD_BATCH_SIZE; i++)
    {
        for (int j = 0; j < 4; j++)
        {
            P.push_back(vec3(x, 0, 0));
        }
        x++;

    }

    std::vector<vec2> T;
    for (int i = 0; i < BILLBOARD_BATCH_SIZE; i++)
    {
        T.push_back(vec2(0, 0));
        T.push_back(vec2(1, 0));
        T.push_back(vec2(1, 1));
        T.push_back(vec2(0, 1));
    }

    std::vector<std::uint32_t> I;
    int a = 0;
    int b = 1;
    int c = 2;
    int d = 3;
    for (int i = 0; i < BILLBOARD_BATCH_SIZE; i++)
    {
        I.push_back(a);
        I.push_back(b);
        I.push_back(c);
        I.push_back(a);
        I.push_back(c);
        I.push_back(d);
        a += 4;
        b += 4;
        c += 4;
        d += 4;
    }

    this->drawinfo = vertexManager->addIndexedData(
        "positions",
        P,

        "texcoords",
        T,

        "normals",
        P,

        "tangents",
        std::vector<vec4> (P.size()),

        "texcoords2",
        T,

        "indices",
        I
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