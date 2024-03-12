 
namespace json11{
    class Json;
};

#include "VertexManager.h"
#include "math2801.h"
#include <map>
using namespace math2801;

class DeviceLocalBuffer;
class StagingBuffer;
class Image;
class DescriptorSetLayout;
class PushConstants;
class PipelineLayout;
class GraphicsPipeline;
class DescriptorSetFactory;
class DescriptorSet;
class Sampler;


class Text{
  public:


    class Metrics{
      public:
        int ascent;
        int descent;
        int width;
        int advance;
        int x;
        int y;
        int leftbearing;
    };

    #pragma pack(push,1)
    class CharData{
      public:
        std::uint32_t screenXY;
        std::uint32_t atlasXY;
        std::uint32_t ascentDescentWidthLeftBearing;
        std::uint32_t color;
    };
    #pragma pack(pop)

    std::vector<CharData> textData;
    Text( VulkanContext* ctx, Framebuffer* framebuffer, std::string fontfile, std::string metricsfile);
    void appendCharacter(char ch, int x, int y, float r, float g, float b, float a);
    void cleanup();
    void clear();
    void print(int x, int y, float r, float g, float b, float a, std::string args);
    void update(VkCommandBuffer cmd);
    void draw(VkCommandBuffer cmd);

    VulkanContext* ctx;
    std::map<char,Metrics> metrics;
    VertexManager* vertexManager;
    VertexManager::Info drawinfo;
    DeviceLocalBuffer* textBuffer;  
    StagingBuffer* stagingBuffer;
    Image* textureAtlas;
    int numChars;
    VkBufferView textView;
    DescriptorSetLayout* descriptorSetLayout;
    PushConstants* pushConstants;
    vec2 screenSize;
    PipelineLayout* pipelineLayout;
    GraphicsPipeline* pipeline; 
    DescriptorSetFactory* descriptorSetFactory;
    DescriptorSet* descriptorSet;
    Sampler* sampler;
    bool dirty;

};
 