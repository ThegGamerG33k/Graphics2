#pragma once
#include "vkhelpers.h"
#include "math2801.h"
#include <span>

class DeviceLocalBuffer;

/// Class to manage vertex and index data for all meshes.
class VertexManager{
  public:

    /// Structure identifying input for the VertexManager
    struct Input{
        std::string name;       ///< Name of item
        VkFormat format;        ///< Data format
        unsigned location;           ///< Location for shaders
        VkVertexInputRate rate; ///< Rate at which to advance the input
    };


    /// When data is added, an Info structure is returned
    /// so the caller can draw the geometry that has
    /// just been handed to the VertexManager.
    struct Info{
        /// Offset of the vertex data in the VertexManager's buffer
        unsigned vertexOffset;
        /// Offset of the index data in the VertexManager's buffer
        unsigned indexOffset;
        /// Number of indices to draw
        unsigned numIndices;
    };

    /// Create vertex manager
    /// @param ctx The context
    /// @param inputs Describes the inputs for each vertex
    VertexManager(VulkanContext* ctx, const std::vector<Input>& inputs);

    /// The vertex layout
    VkPipelineVertexInputStateCreateInfo inputState;

    //~ class Data{
      //~ public:
        //~ std::string name;

        //~ Data(std::string name, const std::vector<float>& v){}
        //~ Data(std::string name, const std::vector<math2801::vec2>& v){}
        //~ Data(std::string name, const std::vector<math2801::vec3>& v){}
        //~ Data(std::string name, const std::vector<math2801::vec4>& v){}
    //~ };


    template<typename ...U>
    VertexManager::Info addIndexedData( U... args ){
        std::vector< AttribInfo > V;
        return addIndexedDataHelper(V,args...); //std::forward_as_tuple(args...));
    }

    /// Transfer all vertex data to the GPU. This can only be called once.
    void pushToGPU();

    /// Bind the vertex buffers for rendering
    /// @param cmd The command buffer
    void bindBuffers(VkCommandBuffer cmd);

  private:

    VulkanContext* ctx;

    struct AttribInfo{
        std::string name;
        const char* ptr;
        unsigned numElements;
        unsigned totalSizeInBytes;
        VkFormat format;
    };

    Info addIndexedDataHelper(std::vector<AttribInfo>& V);

    template<typename T, typename ...U>
    Info addIndexedDataHelper(std::vector<AttribInfo>& V,
            std::string name, const std::vector<T>& values,
            U... args)
    {
        addAttribInfo(V,name,values);
        return addIndexedDataHelper(V, args... );
    }


    int addDataForName(AttribInfo& ai);

    void addAttribInfo(std::vector<AttribInfo>& V, const std::string& name, const std::vector<float>& value);
    void addAttribInfo(std::vector<AttribInfo>& V, const std::string& name, const std::vector<math2801::vec2>& value);
    void addAttribInfo(std::vector<AttribInfo>& V, const std::string& name, const std::vector<math2801::vec3>& value);
    void addAttribInfo(std::vector<AttribInfo>& V, const std::string& name, const std::vector<math2801::vec4>& value);
    void addAttribInfo(std::vector<AttribInfo>& V, const std::string& name, const std::vector<std::int32_t>& value);
    void addAttribInfo(std::vector<AttribInfo>& V, const std::string& name, const std::vector<math2801::ivec2>& value);
    void addAttribInfo(std::vector<AttribInfo>& V, const std::string& name, const std::vector<math2801::ivec3>& value);
    void addAttribInfo(std::vector<AttribInfo>& V, const std::string& name, const std::vector<math2801::ivec4>& value);
    void addAttribInfo(std::vector<AttribInfo>& V, const std::string& name, const std::vector<std::uint32_t>& value);
    void addAttribInfo(std::vector<AttribInfo>& V, const std::string& name, const std::vector<math2801::uvec2>& value);
    void addAttribInfo(std::vector<AttribInfo>& V, const std::string& name, const std::vector<math2801::uvec3>& value);
    void addAttribInfo(std::vector<AttribInfo>& V, const std::string& name, const std::vector<math2801::uvec4>& value);

    void addAttribInfo(std::vector<AttribInfo>& V, const std::string& name, const std::span<float>& value);
    void addAttribInfo(std::vector<AttribInfo>& V, const std::string& name, const std::span<math2801::vec2>& value);
    void addAttribInfo(std::vector<AttribInfo>& V, const std::string& name, const std::span<math2801::vec3>& value);
    void addAttribInfo(std::vector<AttribInfo>& V, const std::string& name, const std::span<math2801::vec4>& value);
    void addAttribInfo(std::vector<AttribInfo>& V, const std::string& name, const std::span<std::int32_t>& value);
    void addAttribInfo(std::vector<AttribInfo>& V, const std::string& name, const std::span<math2801::ivec2>& value);
    void addAttribInfo(std::vector<AttribInfo>& V, const std::string& name, const std::span<math2801::ivec3>& value);
    void addAttribInfo(std::vector<AttribInfo>& V, const std::string& name, const std::span<math2801::ivec4>& value);
    void addAttribInfo(std::vector<AttribInfo>& V, const std::string& name, const std::span<std::uint32_t>& value);
    void addAttribInfo(std::vector<AttribInfo>& V, const std::string& name, const std::span<math2801::uvec2>& value);
    void addAttribInfo(std::vector<AttribInfo>& V, const std::string& name, const std::span<math2801::uvec3>& value);
    void addAttribInfo(std::vector<AttribInfo>& V, const std::string& name, const std::span<math2801::uvec4>& value);

    //internal helper function
    void aai(std::vector<VertexManager::AttribInfo>& V, const std::string& name, const void* p, std::size_t ne, std::size_t es, VkFormat fmt);


    VertexManager(const VertexManager&) = delete;
    void operator=(const VertexManager&) = delete;

    std::vector<VertexManager::Input> inputs;
    std::vector<std::vector<char> > vertexData;
    std::map<std::string, int> nameToIndex;

    std::vector<VkVertexInputBindingDescription> bindings;
    std::vector<VkVertexInputAttributeDescription> attributes;
    std::vector<DeviceLocalBuffer*> buffers;

    unsigned numVertices=0;
    unsigned numIndices=0;
    bool pushedToGPU=false;
};
