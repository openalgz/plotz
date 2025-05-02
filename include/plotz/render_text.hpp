// Plotz Library
// For the license information refer to plotz.hpp

#pragma once

#include <ft2build.h>
#include FT_FREETYPE_H
#include <array>
#include <format>
#include <memory>
#include <stdexcept>
#include <string>
#include <unordered_map>
#include <utility>

namespace plotz
{
   struct free_type_context
   {
      std::shared_ptr<FT_Library> ft = [] {
         FT_Library* ft_lib = new FT_Library;
         if (FT_Init_FreeType(ft_lib)) {
            delete ft_lib; // Clean up if initialization fails
            throw std::runtime_error("Could not initialize FreeType Library");
         }
         return std::shared_ptr<FT_Library>(ft_lib, [](FT_Library* ft) {
            if (ft) {
               FT_Done_FreeType(*ft);
               delete ft;
               ft = nullptr;
            }
         });
      }();

      std::unordered_map<std::string, std::shared_ptr<FT_Face>> faces; // font name to font faces

      // Load and register a font face
      void register_font(const std::string& font_filename);

      // Get a font face by filename
      FT_Face get_font(const std::string& font_filename);
   };

   // Inline global instance of FreeTypeContext
   inline free_type_context ft_context;

   std::pair<int, int> calculate_text_dimensions(FT_Face face, const std::string& text);

   // Function to render text using FreeType with dynamic font size and color
   void render_text_to_image(uint8_t* image, size_t img_width, size_t img_height, const std::string& text,
                             const std::string& font_filename,
                             float font_size_percentage, // Font size as a percentage of image height
                             const std::array<uint8_t, 4>& text_color = {} // (RGB) alpha is ignored
   );
}
