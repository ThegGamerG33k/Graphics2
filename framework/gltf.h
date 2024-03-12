
#include <vector>
#include <string>
#include "math2801.h"
#include "vk.h"
#include "Light.h"


class Primitive;
class DescriptorSet;
class PushConstants;
class VertexManager;


namespace gltf{

class Primitive;

class Mesh{
  public:
    std::vector<Primitive*> primitives;
    std::string name;
    math2801::mat4 worldMatrix;
    Mesh(std::string name);
    void draw(
              VulkanContext* ctx,
              VkCommandBuffer cmd,
              DescriptorSet* dset,
              PushConstants* pushc);
};

std::vector<Light> getLights(std::string filename);
std::vector<Mesh*> load( std::string filename, VertexManager* vertexManager);


}; //namespace gltf