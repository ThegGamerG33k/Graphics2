#pragma once

#include <vector>
#include <string>

namespace imageencode{

/// Encode image data to a PNG
/// @param w width
/// @param h height
/// @param fmt Image format (must be RGBA8 or RGB8)
/// @param data The pixel data
/// @return The PNG data
std::vector<char> encodePNG(int w, int h, std::string fmt, const std::vector<char>& data);

/// Encode image data to a PNG
/// @param w width
/// @param h height
/// @param fmt Image format (must be RGBA8 or RGB8)
/// @param data The pixel data
/// @param quality Image quality, 0...100 (85 is usually a good choice)
/// @return The JPEG data
std::vector<char> encodeJPEG(int w, int h, std::string fmt, const std::vector<char>& data, int quality);

void writePNG(std::string filename, int w, int h, std::string fmt, const std::vector<char>& data);
void writeJPEG(std::string filename, int w, int h, std::string fmt, const std::vector<char>& data);

};
