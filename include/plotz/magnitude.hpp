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
   /**
    * @brief Structure for managing magnitude values in a fixed-size buffer
    * @details Handles the storage, normalization, and rendering of magnitude values to color buffers
    */
   struct magnitude final
   {
      /**
       * @brief Constructor for magnitude structure
       * @param width_in Width of the plot in pixels
       * @param height_in Height of the plot in pixels
       */
      magnitude(uint32_t width_in, uint32_t height_in) noexcept : width(width_in), height(height_in) {}

      magnitude(const magnitude&) noexcept = default;
      magnitude(magnitude&&) noexcept = default;
      magnitude& operator=(const magnitude&) noexcept = default;
      magnitude& operator=(magnitude&&) noexcept = default;

      uint32_t width{}; ///< Width of the plot in pixels
      uint32_t height{}; ///< Height of the plot in pixels

      /// @brief Maximum magnitude value for normalization
      float max_magnitude = std::numeric_limits<float>::lowest();

      /// @brief Minimum magnitude value in the buffer
      float min_magnitude = (std::numeric_limits<float>::max)();

      /// @brief Buffer to store magnitude values
      std::vector<float> buffer = std::vector<float>(width * height);

      /**
       * @brief Adds a point with a specific magnitude value to the buffer
       * @param x X-coordinate of the point
       * @param y Y-coordinate of the point
       * @param magnitude_value Magnitude value to store at the specified point
       * @note Points outside the plot dimensions are ignored
       */
      void add_point(uint32_t x, uint32_t y, float magnitude_value) noexcept
      {
         if (x >= width || y >= height) return; // Ignore points outside the plot

         size_t idx = static_cast<size_t>(y) * width + x;
         buffer[idx] = magnitude_value;

         // Update max_magnitude if necessary
         if (magnitude_value > max_magnitude) {
            max_magnitude = magnitude_value;
         }

         // Update min_magnitude if necessary
         if (magnitude_value < min_magnitude) {
            min_magnitude = magnitude_value;
         }
      }

      /**
       * @brief Shifts buffer values to handle negative magnitudes
       * @details If min_magnitude is negative, all values are shifted to ensure non-negative values
       */
      void shift_buffer_to_non_negative()
      {
         if (buffer.empty()) return;

         // Determine if shifting is necessary
         if (min_magnitude < 0.0f) {
            float shift = -min_magnitude;
            for (auto& val : buffer) {
               val += shift;
            }
            // Adjust max_magnitude after shifting
            max_magnitude += shift;
            // Reset min_magnitude since all values are now non-negative
            min_magnitude = 0.0f;
         }
      }

      /**
       * @brief Renders the buffer to a color buffer using the default color scheme
       * @return Vector of RGBA color values for each pixel
       */
      std::vector<uint8_t> render() { return render(default_color_scheme_data); }

      /**
       * @brief Renders the buffer to a color buffer using a specified color scheme
       * @param colors Vector of color values to use for rendering
       * @return Vector of RGBA color values for each pixel
       */
      std::vector<uint8_t> render(const std::vector<uint8_t>& colors)
      {
         shift_buffer_to_non_negative();
         float saturation = max_magnitude > 0.0f ? max_magnitude : 1.0f;
         return render_saturated(colors, saturation);
      }

      /**
       * @brief Renders the buffer with a specific saturation level
       * @param colors Vector of color values to use for rendering
       * @param saturation Saturation level for normalization
       * @return Vector of RGBA color values for each pixel
       * @pre saturation must be greater than 0.0f
       */
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
            color_idx = (std::min)(color_idx, ncolors - 1);

            // Assign the color to the buffer
            std::copy_n(colors.begin() + color_idx * 4, 4, colorbuf.begin() + idx * 4);
         }

         return colorbuf;
      }

      /**
       * @brief Resets the buffer and magnitude extrema
       * @details Clears all magnitude values and resets min/max tracking
       */
      void reset() noexcept
      {
         std::fill(buffer.begin(), buffer.end(), 0.0f);
         max_magnitude = std::numeric_limits<float>::lowest();
         min_magnitude = (std::numeric_limits<float>::max)();
      }
   };

   /**
    * @brief Structure for managing magnitude values with coordinate mapping
    * @details Maps input coordinates to image coordinates and handles rendering
    */
   struct magnitude_mapped final
   {
      uint32_t input_width{}; ///< Width of the input data
      uint32_t input_height{}; ///< Height of the input data

      uint32_t image_width{}; ///< Width of the output image
      uint32_t image_height{}; ///< Height of the output image

      /// @brief Maximum magnitude value for normalization
      float max_magnitude = std::numeric_limits<float>::lowest();

      /// @brief Minimum magnitude value in the buffer
      float min_magnitude = (std::numeric_limits<float>::max)();

      /// @brief Buffer to store magnitude values mapped to image dimensions
      std::vector<float> buffer = std::vector<float>(image_width * image_height, 0.0f);

      /**
       * @brief Constructor for magnitude_mapped structure
       * @param width_in Width of the input data
       * @param height_in Height of the input data
       * @param img_width Width of the output image
       * @param img_height Height of the output image
       */
      magnitude_mapped(uint32_t width_in, uint32_t height_in, uint32_t img_width, uint32_t img_height)
         : input_width(width_in), input_height(height_in), image_width(img_width), image_height(img_height)
      {}

      magnitude_mapped(const magnitude_mapped&) noexcept = default;
      magnitude_mapped(magnitude_mapped&&) noexcept = default;
      magnitude_mapped& operator=(const magnitude_mapped&) noexcept = default;
      magnitude_mapped& operator=(magnitude_mapped&&) noexcept = default;

      /**
       * @brief Maps input coordinates to image coordinates
       * @param input_x X-coordinate in input space
       * @param input_y Y-coordinate in input space
       * @param[out] image_x Resulting X-coordinate in image space
       * @param[out] image_y Resulting Y-coordinate in image space
       * @return True if mapping was successful, false otherwise
       */
      bool map_coordinates(uint32_t input_x, uint32_t input_y, uint32_t& image_x, uint32_t& image_y) const noexcept
      {
         if (input_width == 0 || input_height == 0) return false;

         // Calculate scaling factors
         float scale_x = static_cast<float>(image_width) / input_width;
         float scale_y = static_cast<float>(image_height) / input_height;

         // Map input coordinates to image coordinates
         image_x = static_cast<uint32_t>(input_x * scale_x);
         image_y = static_cast<uint32_t>(input_y * scale_y);

         // Clamp to image boundaries
         if (image_x >= image_width) image_x = image_width - 1;
         if (image_y >= image_height) image_y = image_height - 1;

         return true;
      }

      /**
       * @brief Adds a point with a specific magnitude value to the buffer
       * @param input_x X-coordinate in input space
       * @param input_y Y-coordinate in input space
       * @param magnitude_value Magnitude value to add at the specified point
       * @details Maps input coordinates to potentially multiple pixels in the output image
       */
      void add_point(uint32_t input_x, uint32_t input_y, float magnitude_value) noexcept
      {
         if (input_width == 0 || input_height == 0) return;

         // Calculate scaling factors
         float scale_x = static_cast<float>(image_width) / input_width;
         float scale_y = static_cast<float>(image_height) / input_height;

         // Determine the range of pixels to update
         uint32_t start_x = static_cast<uint32_t>(input_x * scale_x);
         uint32_t start_y = static_cast<uint32_t>(input_y * scale_y);

         // Calculate the end coordinates, ensuring they don't exceed image dimensions
         uint32_t end_x = static_cast<uint32_t>((input_x + 1) * scale_x);
         uint32_t end_y = static_cast<uint32_t>((input_y + 1) * scale_y);

         end_x = (std::min)(end_x, image_width);
         end_y = (std::min)(end_y, image_height);

         // Iterate over the block of pixels and accumulate magnitude
         for (uint32_t img_y = start_y; img_y < end_y; ++img_y) {
            for (uint32_t img_x = start_x; img_x < end_x; ++img_x) {
               size_t idx = static_cast<size_t>(img_y) * image_width + img_x;
               buffer[idx] += magnitude_value;

               // Update max_magnitude if necessary
               if (buffer[idx] > max_magnitude) {
                  max_magnitude = buffer[idx];
               }

               // Update min_magnitude if necessary
               if (buffer[idx] < min_magnitude) {
                  min_magnitude = buffer[idx];
               }
            }
         }
      }

      /**
       * @brief Shifts buffer values to handle negative magnitudes
       * @details If min_magnitude is negative, all values are shifted to ensure non-negative values
       */
      void shift_buffer_to_non_negative()
      {
         if (buffer.empty()) return;

         // Determine if shifting is necessary
         if (min_magnitude < 0.0f) {
            float shift = -min_magnitude;
            for (auto& val : buffer) {
               val += shift;
            }
            // Adjust max_magnitude after shifting
            max_magnitude += shift;
            // Reset min_magnitude since all values are now non-negative
            min_magnitude = 0.0f;
         }
      }

      /**
       * @brief Renders the buffer to a color buffer using the default color scheme
       * @return Vector of RGBA color values for each pixel
       */
      std::vector<uint8_t> render() { return render(default_color_scheme_data); }

      /**
       * @brief Renders the buffer to a color buffer using a specified color scheme
       * @param colors Vector of color values to use for rendering
       * @return Vector of RGBA color values for each pixel
       */
      std::vector<uint8_t> render(const std::vector<uint8_t>& colors)
      {
         shift_buffer_to_non_negative();
         float saturation = max_magnitude > 0.0f ? max_magnitude : 1.0f;
         return render_saturated(colors, saturation);
      }

      /**
       * @brief Renders the buffer with a specific saturation level
       * @param colors Vector of color values to use for rendering
       * @param saturation Saturation level for normalization
       * @return Vector of RGBA color values for each pixel
       * @pre saturation must be greater than 0.0f
       */
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
            color_idx = (std::min)(color_idx, ncolors - 1);

            // Assign the color to the buffer
            std::copy_n(colors.begin() + color_idx * 4, 4, colorbuf.begin() + idx * 4);
         }

         return colorbuf;
      }

      /**
       * @brief Resets the buffer and magnitude extrema
       * @details Clears all magnitude values and resets min/max tracking
       */
      void reset() noexcept
      {
         std::fill(buffer.begin(), buffer.end(), 0.0f);
         max_magnitude = std::numeric_limits<float>::lowest();
         min_magnitude = (std::numeric_limits<float>::max)();
      }
   };
}
