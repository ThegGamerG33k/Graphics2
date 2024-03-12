#pragma once

#include "vkhelpers.h"
#include <set>
#include <string>
#include <vector>
#include <map>
#include <cstdint>
#include "math2801.h"
#include "parseMembers.h"
#include <array>

//FIXME: Do uvec{2,3,4} and array of ivec4/uvec4

class DeviceLocalBuffer;
class DescriptorSet;

/// Class to manage collection of uniforms
class Uniforms{
    public:

    /// Create uniform manager
    /// @param ctx Associated context
    /// @param slot Default slot in descriptor set for uniforms
    /// @param uniformFilename Name of the file that holds the uniforms
    /// @param name For debugging
    Uniforms(VulkanContext* ctx, int slot, std::string uniformFilename, std::string name);

    template<typename ...T, typename U>
    void set( const std::pair<std::string, U>& v1, T... args)
    {
        this->set(v1.first,v1.second);
        this->set(args...);
    }

    template<typename U>
    void set( const std::pair<std::string, U>& v1)
    {
        this->set(v1.first,v1.second);
    }

    /// Set uniform value; if the name does not specify a valid
    /// uniform or the value is not the correct type,
    /// an exception is thrown.
    /// @param name The uniform name
    /// @param value The value to set
    void set(const std::string& name, std::int32_t value);

    /// Set uniform value; if the name does not specify a valid
    /// uniform or the value is not the correct type,
    /// an exception is thrown.
    /// @param name The uniform name
    /// @param value The value to set
    void set(const std::string& name, std::uint32_t value);

    /// Set uniform value; if the name does not specify a valid
    /// uniform or the value is not the correct type,
    /// an exception is thrown.
    /// @param name The uniform name
    /// @param value The value to set
    void set(const std::string& name, float value);

    /// Set uniform value; if the name does not specify a valid
    /// uniform or the value is not the correct type,
    /// an exception is thrown.
    /// @param name The uniform name
    /// @param value The value to set
    void set(const std::string& name, double value);

    /// Set uniform value; if the name does not specify a valid
    /// uniform or the value is not the correct type,
    /// an exception is thrown.
    /// @param name The uniform name
    /// @param value The value to set
    void set(const std::string& name, math2801::ivec2 value);

    /// Set uniform value; if the name does not specify a valid
    /// uniform or the value is not the correct type,
    /// an exception is thrown.
    /// @param name The uniform name
    /// @param value The value to set
    void set(const std::string& name, math2801::vec2 value);

    /// Set uniform value; if the name does not specify a valid
    /// uniform or the value is not the correct type,
    /// an exception is thrown.
    /// @param name The uniform name
    /// @param value The value to set
    void set(const std::string& name, math2801::vec3 value);

    /// Set uniform value; if the name does not specify a valid
    /// uniform or the value is not the correct type,
    /// an exception is thrown.
    /// @param name The uniform name
    /// @param value The value to set
    void set(const std::string& name, math2801::vec4 value);

    /// Set uniform value; if the name does not specify a valid
    /// uniform or the value is not the correct type,
    /// an exception is thrown.
    /// @param name The uniform name
    /// @param value The value to set
    void set(const std::string& name, math2801::mat4 value);

    /// Set uniform value; if the name does not specify a valid
    /// uniform or the value is not the correct type,
    /// an exception is thrown.
    /// @param name The uniform name
    /// @param value The value to set
    void set(const std::string& name, const std::vector<math2801::vec4>& value);

    /// Set uniform value; if the name does not specify a valid
    /// uniform or the value is not the correct type,
    /// an exception is thrown.
    /// @param name The uniform name
    /// @param value The value to set
    void set(const std::string& name, const std::vector<math2801::uvec4>& value);

    /// Set uniform value; if the name does not specify a valid
    /// uniform or the value is not the correct type,
    /// an exception is thrown.
    /// @param name The uniform name
    /// @param value The value to set
    void set(const std::string& name, const std::vector<math2801::ivec4>& value);

    /// Set uniform value; if the name does not specify a valid
    /// uniform or the value is not the correct type,
    /// an exception is thrown.
    /// @param name The uniform name
    /// @param value The value to set
    void set(const std::string& name, const std::vector<math2801::mat4>& value);


    /// Update the uniform data visible to the the GPU and bind the associated descriptor set.
    /// If this function is not called, any changes to the uniforms
    /// will not be visible to the GPU.
    /// @param cmd The command buffer
    /// @param descriptorSet The descriptor set to use
    /// @param slot The slot number in the descriptor set. If this is not given,
    ///             the default slot number from the constructor is used.
    void bind(VkCommandBuffer cmd, DescriptorSet* descriptorSet, int slot=-1);

    /// Take snapshot of current uniform data, copy it into a private
    /// buffer, and return that buffer.
    /// The Uniforms object still owns the buffer, so the caller
    /// should not dispose of it. This is normally called only by
    /// other framework functions (Uniforms::bind() and Descriptors::setSlot())
    DeviceLocalBuffer* getBufferForSetSlot();

    /// Get the value of the symbol #define'd in the uniform specification.
    /// If the symbol does not exist, an exception is thrown.
    /// Only integer values are permitted.
    /// @param name The name of the symbol.
    /// @return The value of the symbol.
    int getDefine(const std::string& name);

    /// Return true if a symbol with the given name was
    /// #define'd in the uniform file.
    /// @param name The symbol name.
    /// @return True if the symbol exists; false if not.
    bool hasDefine(const std::string& name);

  private:

    void _update();
    DeviceLocalBuffer* _getBuffer(int frameID);

    std::string name;
    int slot;
    std::set<std::string> uninitialized;        //keeps track of uniforms that haven't been set yet
    std::vector<DeviceLocalBuffer*> availableBuffers;
    std::map<unsigned, std::vector<DeviceLocalBuffer*> > activeBuffers;  //key=frame number
    std::vector< std::pair<DeviceLocalBuffer*, std::vector<char> > > needToBePushed;
    std::vector<VkDeviceMemory> memories;
    std::vector<VkDeviceSize> availableMemory;
    std::vector<VkDeviceSize> offsets;
    std::map<std::string,int> defines;
    std::map<std::string,parseMembers::Item> items;
    std::vector<char> shadowBuffer;


    //size of all uniforms
    VkDeviceSize byteSize;

    std::uint32_t memoryTypeBits;

    void _frameCompleteCallback(int frameNumber);
    void _frameEndCallback();
    void _cleanupFunc();
    void _noSuchUniform(std::string name);

    VulkanContext* ctx;
    Uniforms(const Uniforms&) = delete;
    void operator=(const Uniforms&) = delete;
};
