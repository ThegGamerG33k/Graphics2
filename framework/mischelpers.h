#pragma once
#include <string>
#include <vector>
#include <sstream>


/// Remove leading and trailing whitespace from a string
/// @param s The string to trim
/// @return Trimmed copy of the string
std::string trim(const std::string& s);

template<typename T>
unsigned len(const T& x)
{
    return (unsigned) x.size();
}

template<typename T>
T* pointer(std::vector<T>& v)
{
    return v.data();
}

/// Functions like Python's join() function: Combine entries in v
/// by converting to string and putting copies of J between them.
/// Example: join( "," , foo )
/// Returns string with foo[0] + "," + foo[1] + "," + foo[2] + ...
/// @param J The separator to use
/// @param v List of items
/// @return The joined contents
template<typename T>
std::string join( std::string J, const T& v)
{
    std::ostringstream oss;
    bool first=true;
    for(const auto& x : v ){
        if( !first ){
            oss<< J;
        }
        oss << x;
        first=false;
    }
    return oss.str();
}

template<typename T>
std::string str(const T& x)
{
    std::ostringstream oss;
    oss << x;
    return oss.str();
}

//std::vector<std::string> shlex_split(std::string s);

/// Split a string at whitespace characters
/// Ex: split( "foo     bar baz") returns {"foo","bar","baz"}
/// @param s The string to split. Multiple consecutive whitespace
///     characters are used to delimit each item
/// @return List of items found
std::vector<std::string> split(std::string s);

/// Split a string at the delimiter character. Multiple runs
/// of the delimiter produce empty entries in the result.
/// Ex: split( "foo,bar,,baz" , ',' ) returns {"foo","bar","","baz"}
/// @param s The string to split
/// @param delim The delimiter character
/// @return List of items
std::vector<std::string> split(std::string s, char delim);


/// Return the length of a vector
/// @param v The vector
/// @return Its length (v.size(), as an unsigned).
template<typename T>
unsigned len(const std::vector<T>& v){
    return (unsigned)v.size();
}

/// Return the length of a string
/// @param s The string
/// @return The string's length (s.length(), as an unsigned)
unsigned len(const std::string& s);

/// String concatenation
/// @param a A string
/// @param b A number
/// @return a + std::to_string(b)
std::string operator+(const std::string& a, int b);

/// String concatenation
/// @param a A string
/// @param b A number
/// @return a + std::to_string(b)
std::string operator+(int b, const std::string& a);

/// String concatenation
/// @param a A string
/// @param b A number
/// @return a + std::to_string(b)
std::string operator+(const std::string& a, long int b);

/// String concatenation
/// @param a A string
/// @param b A number
/// @return a + std::to_string(b)
std::string operator+(long int b,const std::string& a);

/// String concatenation
/// @param a A string
/// @param b A number
/// @return a + std::to_string(b)
std::string operator+(const std::string& a, long unsigned b);

/// String concatenation
/// @param a A string
/// @param b A number
/// @return a + std::to_string(b)
std::string operator+(long unsigned b, const std::string& a);

/// String concatenation
/// @param a A string
/// @param b A number
/// @return a + std::to_string(b)
std::string operator+(const std::string& a, unsigned b);

/// String concatenation
/// @param a A string
/// @param b A number
/// @return a + std::to_string(b)
std::string operator+(unsigned b, const std::string& a);

/// String concatenation
/// @param a A string
/// @param b A number
/// @return a + std::to_string(b)
std::string operator+(const std::string& a, float b);

/// String concatenation
/// @param a A string
/// @param b A number
/// @return a + std::to_string(b)
std::string operator+(float b, const std::string& a);

/// String concatenation
/// @param a A string
/// @param b A number
/// @return a + std::to_string(b)
std::string operator+(const std::string& a, double b);

/// String concatenation
/// @param a A string
/// @param b A number
/// @return a + std::to_string(b)
std::string operator+(double b, const std::string& a);
