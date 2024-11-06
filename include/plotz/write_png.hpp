// Plotz Library
// For the license information refer to plotz.hpp

#pragma once

#include <png.h>

#include <cstdint>
#include <format>
#include <stdexcept>
#include <string>
#include <vector>

namespace plotz
{
   inline void write_png(const std::string& filename, const uint8_t* data, size_t w, size_t h)
   {
      png_structp png_ptr = png_create_write_struct(PNG_LIBPNG_VER_STRING, nullptr, nullptr, nullptr);
      if (!png_ptr) {
         throw std::runtime_error("Error initializing libpng write struct.");
      }

      png_infop info_ptr = png_create_info_struct(png_ptr);
      if (!info_ptr) {
         png_destroy_write_struct(&png_ptr, nullptr);
         throw std::runtime_error("Error initializing libpng info struct.");
      }

      if (setjmp(png_jmpbuf(png_ptr))) {
         png_destroy_write_struct(&png_ptr, &info_ptr);
         throw std::runtime_error("Error during PNG creation.");
      }

      std::unique_ptr<FILE, decltype(&fclose)> fp(fopen(filename.c_str(), "wb"), &fclose);
      if (!fp) {
         png_destroy_write_struct(&png_ptr, &info_ptr);
         throw std::runtime_error(std::format("Error writing {}: {}", filename, std::strerror(errno)));
      }
      png_init_io(png_ptr, fp.get());

      // Set PNG header information.
      static constexpr int bit_depth = 8;
      static constexpr int color_type = PNG_COLOR_TYPE_RGB_ALPHA;
      static constexpr int interlace_type = PNG_INTERLACE_NONE;
      png_set_IHDR(png_ptr, info_ptr, static_cast<png_uint_32>(w), static_cast<png_uint_32>(h), bit_depth, color_type,
                   interlace_type, PNG_COMPRESSION_TYPE_DEFAULT, PNG_FILTER_TYPE_DEFAULT);

      auto row_pointers = std::make_unique_for_overwrite<png_bytep[]>(h);
      for (size_t y = 0; y < h; ++y) {
         row_pointers[y] = const_cast<png_bytep>(data + y * w * 4);
      }
      png_set_rows(png_ptr, info_ptr, row_pointers.get());

      // Write the PNG image data.
      png_write_png(png_ptr, info_ptr, PNG_TRANSFORM_IDENTITY, nullptr);

      png_destroy_write_struct(&png_ptr, &info_ptr);
   }
}
