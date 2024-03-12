#pragma once
#include "vkhelpers.h"
#include "parseMembers.h"
#include "math2801.h"

//FIXME: Do uvec{2,3,4}

/// Class to manage push constants
class PushConstants{
  public:

    /// Push constant data. Key=name, value=information about it
    std::map<std::string,parseMembers::Item> items;

    /// Total size of all push constants
    int byteSize;

    /// Parse file and initialize items & byteSize.
    /// @param filename The file to parse
    PushConstants(std::string filename);


    ///Set one or more push constants
    ///Pass in a list of pairs. Example:
    /// set( cmd, { "worldMatrix", mat4(...) }, {"colorFlag", 1.0f } } );
    //~ template<typename ...T>
    //~ void set(VkCommandBuffer cmd, T... args)
    //~ {
        //~ this->setHelper(cmd, args... );
    //~ }

    ///Pass in a single pair. Example:
    /// set( cmd, "worldMatrix", mat4(...));
    template<typename T>
    void set(VkCommandBuffer cmd, const std::string& name, const T& value)
    {
        this->setHelper(cmd, std::make_pair(name,value) );
    }


  private:

    template<typename ...T, typename U>
    void setHelper(VkCommandBuffer cmd, const std::pair<std::string, U>& value, T... args){
        this->setV(cmd, value.first, value.second);
        setHelper(cmd, args...);
    }

    template<typename U>
    void setHelper(VkCommandBuffer cmd, const std::pair<std::string, U>& value){
        this->setV(cmd, value.first, value.second);
    }


    /// Set a push constant; if the name does not exist,
    /// an exception is raised.
    /// @param cmd The command buffer
    /// @param name Variable name
    /// @param value Value to set
    void setV(VkCommandBuffer cmd, const std::string& name, std::int32_t value);

    /// Set a push constant; if the name does not exist,
    /// an exception is raised.
    /// @param cmd The command buffer
    /// @param name Variable name
    /// @param value Value to set
    void setV(VkCommandBuffer cmd, const std::string& name, std::uint32_t value);

    /// Set a push constant; if the name does not exist,
    /// an exception is raised.
    /// @param cmd The command buffer
    /// @param name Variable name
    /// @param value Value to set
    void setV(VkCommandBuffer cmd, const std::string& name, float value);

    /// Set a push constant; if the name does not exist,
    /// an exception is raised.
    /// @param cmd The command buffer
    /// @param name Variable name
    /// @param value Value to set
    void setV(VkCommandBuffer cmd, const std::string& name, double value);

    /// Set a push constant; if the name does not exist,
    /// an exception is raised.
    /// @param cmd The command buffer
    /// @param name Variable name
    /// @param value Value to set
    void setV(VkCommandBuffer cmd, const std::string& name, math2801::ivec2 value);

    /// Set a push constant; if the name does not exist,
    /// an exception is raised.
    /// @param cmd The command buffer
    /// @param name Variable name
    /// @param value Value to set
    void setV(VkCommandBuffer cmd, const std::string& name, math2801::vec2 value);

    /// Set a push constant; if the name does not exist,
    /// an exception is raised.
    /// @param cmd The command buffer
    /// @param name Variable name
    /// @param value Value to set
    void setV(VkCommandBuffer cmd, const std::string& name, math2801::vec3 value);

    /// Set a push constant; if the name does not exist,
    /// an exception is raised.
    /// @param cmd The command buffer
    /// @param name Variable name
    /// @param value Value to set
    void setV(VkCommandBuffer cmd, const std::string& name, math2801::vec4 value);

    /// Set a push constant; if the name does not exist,
    /// an exception is raised.
    /// @param cmd The command buffer
    /// @param name Variable name
    /// @param value Value to set
    void setV(VkCommandBuffer cmd, const std::string& name, math2801::mat4 value);

    /// Set a push constant; if the name does not exist,
    /// an exception is raised.
    /// @param cmd The command buffer
    /// @param name Variable name
    /// @param value Value to set
    void setV(VkCommandBuffer cmd, const std::string& name, const std::vector<math2801::vec4>& value);

    /// Set a push constant; if the name does not exist,
    /// an exception is raised.
    /// @param cmd The command buffer
    /// @param name Variable name
    /// @param value Value to set
    void setV(VkCommandBuffer cmd, const std::string& name, const std::vector<math2801::uvec4>& value);

    /// Set a push constant; if the name does not exist,
    /// an exception is raised.
    /// @param cmd The command buffer
    /// @param name Variable name
    /// @param value Value to set
    void setV(VkCommandBuffer cmd, const std::string& name, const std::vector<math2801::ivec4>& value);
};
