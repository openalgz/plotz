// Plotz Library
// For the license information refer to plotz.hpp

#pragma once

#include <cstdint>
#include <string>

namespace plotz
{
   void write_png(const std::string& filename, const uint8_t* data, size_t w, size_t h);
}
