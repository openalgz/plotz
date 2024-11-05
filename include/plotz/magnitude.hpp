// Plotz Library
// For the license information refer to plotz.hpp

#pragma once

#include <algorithm>
#include <cassert>
#include <cmath>
#include <cstring>
#include <memory>

#include "plotz/color_scheme.hpp"

namespace plotz
{
   struct magnitude final
   {
      magnitude(uint32_t width_in, uint32_t height_in) noexcept : width(width_in), height(height_in) {}
      magnitude(const magnitude&) noexcept = default;
      magnitude(magnitude&&) noexcept = default;
      magnitude& operator=(const magnitude&) noexcept = default;
      magnitude& operator=(magnitude&&) noexcept = default;

      uint32_t width{}, height{}; // Dimensions of the plot
      float max_magnitude{}; // Maximum magnitude value for normalization
      std::vector<float> buffer = std::vector<float>(width * height); // Buffer to store magnitude values

      void add_point(uint32_t x, uint32_t y, float magnitude) noexcept
      {
         if (x >= width || y >= height) return; // Ignore points outside the plot

         size_t idx = static_cast<size_t>(y) * width + x;
         buffer[idx] = magnitude;

         if (magnitude > max_magnitude) {
            max_magnitude = magnitude;
         }
      }

      std::vector<uint8_t> render() const { return render(default_color_scheme_data); }

      std::vector<uint8_t> render(const std::vector<uint8_t>& colors) const
      {
         float saturation = max_magnitude > 0.0f ? max_magnitude : 1.0f;
         return render_saturated(colors, saturation);
      }

      // Method to render with a specific saturation level
      std::vector<uint8_t> render_saturated(const std::vector<uint8_t>& colors, float saturation) const
      {
         assert(saturation > 0.0f);

         size_t total_pixels = static_cast<size_t>(width) * height;
         std::vector<uint8_t> colorbuf(total_pixels * 4); // Assuming RGBA

         size_t ncolors = get_color_count(colors);
         if (ncolors == 0) {
            // Handle empty color scheme
            std::fill(colorbuf.begin(), colorbuf.end(), 0);
            return colorbuf;
         }

         for (size_t idx = 0; idx < total_pixels; ++idx) {
            float val = buffer[idx];
            // Normalize the value based on the maximum magnitude
            float normalized = std::clamp(val / saturation, 0.0f, 1.0f);

            // Determine the color index
            size_t color_idx = static_cast<size_t>((ncolors - 1) * normalized + 0.5f);
            color_idx = std::min(color_idx, ncolors - 1);

            // Assign the color to the buffer
            std::copy_n(colors.begin() + color_idx * 4, 4, colorbuf.begin() + idx * 4);
         }

         return colorbuf;
      }

      void reset() noexcept
      {
         std::fill(buffer.begin(), buffer.end(), 0.0f);
         max_magnitude = 0.0f;
      }
   };

   struct magnitude_mapped final
   {
      // Input dimensions
      uint32_t input_width{}, input_height{};

      // Output (image) dimensions
      uint32_t image_width{}, image_height{};

      // Maximum magnitude value for normalization
      float max_magnitude{};

      // Buffer to store magnitude values mapped to image dimensions
      std::vector<float> buffer = std::vector<float>(image_width * image_height, 0.0f);

      magnitude_mapped(uint32_t width_in, uint32_t height_in, uint32_t img_width, uint32_t img_height)
         : input_width(width_in), input_height(height_in), image_width(img_width), image_height(img_height)
      {}

      magnitude_mapped(const magnitude_mapped&) noexcept = default;
      magnitude_mapped(magnitude_mapped&&) noexcept = default;
      magnitude_mapped& operator=(const magnitude_mapped&) noexcept = default;
      magnitude_mapped& operator=(magnitude_mapped&&) noexcept = default;

      // Method to map input coordinates to image coordinates
      bool map_coordinates(uint32_t input_x, uint32_t input_y, uint32_t& image_x, uint32_t& image_y) const noexcept
      {
         if (input_width == 0 || input_height == 0) return false;

         // Calculate scaling factors
         float scale_x = float(image_width) / input_width;
         float scale_y = float(image_height) / input_height;

         // Map input coordinates to image coordinates
         image_x = uint32_t(input_x * scale_x);
         image_y = uint32_t(input_y * scale_y);

         // Clamp to image boundaries
         if (image_x >= image_width) image_x = image_width - 1;
         if (image_y >= image_height) image_y = image_height - 1;

         return true;
      }

      void add_point(uint32_t input_x, uint32_t input_y, float magnitude) noexcept
      {
         if (input_width == 0 || input_height == 0) return;

         // Calculate scaling factors
         float scale_x = float(image_width) / input_width;
         float scale_y = float(image_height) / input_height;

         // Determine the range of pixels to update
         uint32_t start_x = uint32_t(input_x * scale_x);
         uint32_t start_y = uint32_t(input_y * scale_y);

         // Calculate the end coordinates, ensuring they don't exceed image dimensions
         uint32_t end_x = uint32_t((input_x + 1) * scale_x);
         uint32_t end_y = uint32_t((input_y + 1) * scale_y);

         end_x = std::min(end_x, image_width);
         end_y = std::min(end_y, image_height);

         // Iterate over the block of pixels and accumulate magnitude
         for (uint32_t img_y = start_y; img_y < end_y; ++img_y) {
            for (uint32_t img_x = start_x; img_x < end_x; ++img_x) {
               size_t idx = static_cast<size_t>(img_y) * image_width + img_x;
               buffer[idx] += magnitude;

               // Update max_magnitude if necessary
               if (buffer[idx] > max_magnitude) {
                  max_magnitude = buffer[idx];
               }
            }
         }
      }

      // Render the buffer to a color buffer using a default color scheme
      std::vector<uint8_t> render() const { return render(default_color_scheme_data); }

      // Render the buffer to a color buffer using a specified color scheme
      std::vector<uint8_t> render(const std::vector<uint8_t>& colors) const
      {
         float saturation = max_magnitude > 0.0f ? max_magnitude : 1.0f;
         return render_saturated(colors, saturation);
      }

      // Method to render with a specific saturation level
      std::vector<uint8_t> render_saturated(const std::vector<uint8_t>& colors, float saturation) const
      {
         assert(saturation > 0.0f);

         size_t total_pixels = static_cast<size_t>(image_width) * image_height;
         std::vector<uint8_t> colorbuf(total_pixels * 4, 0); // Assuming RGBA

         size_t ncolors = get_color_count(colors);
         if (ncolors == 0) {
            // Handle empty color scheme
            std::fill(colorbuf.begin(), colorbuf.end(), 0);
            return colorbuf;
         }

         for (size_t idx = 0; idx < total_pixels; ++idx) {
            float val = buffer[idx];
            // Normalize the value based on the maximum magnitude
            float normalized = std::clamp(val / saturation, 0.0f, 1.0f);

            // Determine the color index
            size_t color_idx = static_cast<size_t>((ncolors - 1) * normalized + 0.5f);
            color_idx = std::min(color_idx, ncolors - 1);

            // Assign the color to the buffer
            std::copy_n(colors.begin() + color_idx * 4, 4, colorbuf.begin() + idx * 4);
         }

         return colorbuf;
      }

      // Reset the buffer and maximum magnitude
      void reset() noexcept
      {
         std::fill(buffer.begin(), buffer.end(), 0.0f);
         max_magnitude = 0.0f;
      }

     private:
      // Placeholder for getting the number of colors in the scheme
      static size_t get_color_count(const std::vector<uint8_t>& colors)
      {
         return colors.size() / 4; // Assuming RGBA
      }
   };
}
