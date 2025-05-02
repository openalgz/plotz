// Plotz Library
// For the license information refer to plotz.hpp

#include "plotz/render_text.hpp"

#include <iostream>

namespace plotz
{
   void free_type_context::register_font(const std::string& font_filename)
   {
      if (faces.find(font_filename) == faces.end()) {
         FT_Face* face = new FT_Face;
         if (FT_New_Face(*ft, font_filename.c_str(), 0, face)) {
            delete face; // Clean up if face creation fails
            throw std::runtime_error(std::format("Failed to load font: {}", font_filename));
         }
         faces[font_filename] = std::shared_ptr<FT_Face>(face, [](FT_Face* face) {
            if (face) {
               FT_Done_Face(*face);
               delete face;
               face = nullptr;
            }
         });
      }
   }

   FT_Face free_type_context::get_font(const std::string& font_filename)
   {
      auto it = faces.find(font_filename);
      if (it == faces.end()) {
         throw std::runtime_error(std::format("Font not registered: {}", font_filename));
      }
      return *(it->second); // Dereference shared_ptr to return FT_Face
   }

   std::pair<int, int> calculate_text_dimensions(FT_Face face, const std::string& text)
   {
      int width = 0;
      int max_ascent = 0;
      int max_descent = 0;

      for (char c : text) {
         if (FT_Load_Char(face, c, FT_LOAD_RENDER)) {
            std::cerr << "Failed to load Glyph for character: " << c << "\n";
            continue;
         }

         FT_GlyphSlot g = face->glyph;

         // Accumulate the advance width
         width += g->advance.x >> 6; // Convert from 1/64th pixels to pixels

         // Track maximum ascent and descent for vertical sizing
         if (g->bitmap_top > max_ascent) max_ascent = g->bitmap_top;

         int descent = g->bitmap.rows - g->bitmap_top;
         if (descent > max_descent) max_descent = descent;
      }

      int height = max_ascent + max_descent;
      return {width, height};
   }

   void render_text_to_image(uint8_t* image, size_t img_width, size_t img_height, const std::string& text,
                             const std::string& font_filename,
                             float font_size_percentage, // Font size as a percentage of image height
                             const std::array<uint8_t, 4>& text_color // (RGB) alpha is ignored
   )
   {
      // Register the font if not already done
      ft_context.register_font(font_filename);

      // Get the font face for rendering
      FT_Face face = ft_context.get_font(font_filename);

      // Step 1: Determine the font size based on font_size_percentage
      // Ensure the percentage is reasonable (e.g., between 1% and 100%)
      font_size_percentage = std::clamp(font_size_percentage, 1.0f, 100.0f);
      int font_size = static_cast<int>(img_height * (font_size_percentage / 100.0f));
      FT_Set_Pixel_Sizes(face, 0, font_size);

      // Step 2: Calculate text dimensions
      auto [text_width, text_height] = calculate_text_dimensions(face, text);

      // Step 3: Calculate starting positions to center the text
      int x_pos = int(img_width - text_width) / 2;
      int y_pos = int(img_height) - int(int(img_height + text_height) / 10);

      // Ensure starting positions are within the image boundaries
      x_pos = (std::max)(0, x_pos);
      y_pos = (std::min)(static_cast<int>(img_height), (std::max)(0, y_pos));

      // Starting position
      int pen_x = x_pos;
      int pen_y = y_pos;

      // Step 4: Render each character
      for (char c : text) {
         // Load character glyph
         if (FT_Load_Char(face, c, FT_LOAD_RENDER)) {
            throw std::runtime_error(std::format("Failed to load Glyph for character: {}", c));
         }

         FT_GlyphSlot g = face->glyph;

         // Render the glyph bitmap onto the image
         for (unsigned int row = 0; row < g->bitmap.rows; ++row) {
            for (unsigned int col = 0; col < g->bitmap.width; ++col) {
               int x = pen_x + g->bitmap_left + col;
               int y = pen_y - g->bitmap_top + row;

               // Check boundaries
               if (x < 0 || x >= static_cast<int>(img_width) || y < 0 || y >= static_cast<int>(img_height)) continue;

               // Calculate the pixel index in the image buffer (assuming RGBA)
               size_t pixel_index = (y * img_width + x) * 4;

               // Get the glyph's alpha value
               unsigned char glyph_alpha = g->bitmap.buffer[row * g->bitmap.width + col];

               // Simple blending: use specified text color with glyph alpha
               unsigned char alpha = glyph_alpha;
               unsigned char inv_alpha = 255 - alpha;

               // Existing image pixel (RGBA)
               unsigned char* pixel = &image[pixel_index];

               // Blend the text color with the existing pixel based on alpha
               pixel[0] = (pixel[0] * inv_alpha + text_color[0] * alpha) / 255; // Red
               pixel[1] = (pixel[1] * inv_alpha + text_color[1] * alpha) / 255; // Green
               pixel[2] = (pixel[2] * inv_alpha + text_color[2] * alpha) / 255; // Blue
               pixel[3] = (std::min)(255, pixel[3] + alpha); // Alpha
            }
         }

         // Advance the pen position for the next character
         pen_x += g->advance.x >> 6; // Convert from 1/64th pixels to pixels
         pen_y += g->advance.y >> 6;
      }
   }
}
