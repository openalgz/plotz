// Plotz Library
// For the license information refer to plotz.hpp

#pragma once

#include <algorithm>
#include <cassert>
#include <cmath>
#include <vector>

#include "plotz/color_scheme.hpp"

namespace plotz
{
   struct spectrum final
   {
      enum class bar_style {
         solid, // Solid color bars (using the normalized value to select the color)
         gradient, // Gradient color bars (color changes from bottom to top)
         segmented // Segmented bars (like LED VU meters)
      };

      spectrum(uint32_t bins_in, uint32_t width_in, uint32_t height_in) noexcept
         : num_bins(bins_in),
           width(width_in),
           height(height_in),
           buffer(bins_in, 0.0f),
           peak_values(bins_in, 0.0f),
           prev_buffer(bins_in, 0.0f)
      {}

      spectrum(const spectrum&) noexcept = default;
      spectrum(spectrum&&) noexcept = default;
      spectrum& operator=(const spectrum&) noexcept = default;
      spectrum& operator=(spectrum&&) noexcept = default;

      uint32_t num_bins{}; // Number of frequency bins (FFT size)
      uint32_t width{}, height{}; // Visualization dimensions
      float max_magnitude = std::numeric_limits<float>::lowest(); // Maximum magnitude value
      float min_magnitude = (std::numeric_limits<float>::max)(); // Minimum magnitude value
      std::vector<float> buffer; // Buffer to store magnitude values (1D array of bins)
      std::vector<float> peak_values; // Buffer to store peak values
      std::vector<float> prev_buffer; // Buffer to store previous values for smoothing

      bar_style style = bar_style::gradient; // Default to gradient style
      float smoothing_factor = 0.3f; // 0 = no smoothing, 1 = maximum smoothing
      float peak_decay = 0.05f; // Rate at which peaks decay (0 = no decay)
      bool show_peaks = true; // Whether to show peak indicators
      float bar_width_factor = 0.8f; // Width of bars relative to bin spacing (0-1)

      // Update the entire spectrum with new magnitude values
      void update(const std::vector<float>& magnitudes) noexcept
      {
         const size_t size = (std::min)(num_bins, static_cast<uint32_t>(magnitudes.size()));

         // Store current buffer for smoothing
         prev_buffer = buffer;

         for (size_t i = 0; i < size; ++i) {
            float magnitude_value = magnitudes[i];

            // Apply smoothing if enabled
            if (smoothing_factor > 0.0f && smoothing_factor < 1.0f) {
               magnitude_value = prev_buffer[i] * smoothing_factor + magnitude_value * (1.0f - smoothing_factor);
            }

            buffer[i] = magnitude_value;

            // Update max_magnitude
            if (magnitude_value > max_magnitude) {
               max_magnitude = magnitude_value;
            }

            // Update min_magnitude
            if (magnitude_value < min_magnitude) {
               min_magnitude = magnitude_value;
            }

            // Update peak values
            if (magnitude_value > peak_values[i]) {
               peak_values[i] = magnitude_value;
            }
            else if (peak_decay > 0.0f) {
               // Apply decay to peaks
               peak_values[i] -= peak_decay;
               if (peak_values[i] < magnitude_value) {
                  peak_values[i] = magnitude_value;
               }
            }
         }
      }

      // Update a single bin's magnitude
      void update_bin(uint32_t bin, float magnitude_value) noexcept
      {
         if (bin >= num_bins) return; // Ignore if outside buffer

         // Apply smoothing if enabled
         if (smoothing_factor > 0.0f && smoothing_factor < 1.0f) {
            magnitude_value = buffer[bin] * smoothing_factor + magnitude_value * (1.0f - smoothing_factor);
         }

         buffer[bin] = magnitude_value;

         // Update max_magnitude
         if (magnitude_value > max_magnitude) {
            max_magnitude = magnitude_value;
         }

         // Update min_magnitude
         if (magnitude_value < min_magnitude) {
            min_magnitude = magnitude_value;
         }

         // Update peak value for this bin
         if (magnitude_value > peak_values[bin]) {
            peak_values[bin] = magnitude_value;
         }
         else if (peak_decay > 0.0f) {
            // Apply decay to peak
            peak_values[bin] -= peak_decay;
            if (peak_values[bin] < magnitude_value) {
               peak_values[bin] = magnitude_value;
            }
         }
      }

      // Shift buffer values to handle negative magnitudes
      void shift_buffer_to_non_negative()
      {
         if (buffer.empty()) return;

         // Determine if shifting is necessary
         if (min_magnitude < 0.0f) {
            float shift = -min_magnitude;
            for (auto& val : buffer) {
               val += shift;
            }
            // Also shift peak values
            for (auto& val : peak_values) {
               val += shift;
            }
            // Adjust max_magnitude after shifting
            max_magnitude += shift;
            // Reset min_magnitude since all values are now non-negative
            min_magnitude = 0.0f;
         }
      }

      // Render the spectrum to a color buffer using default color scheme
      std::vector<uint8_t> render() { return render(default_color_scheme_data); }

      // Render the spectrum to a color buffer using specified color scheme
      std::vector<uint8_t> render(const std::vector<uint8_t>& colors)
      {
         shift_buffer_to_non_negative();
         float saturation = max_magnitude > 0.0f ? max_magnitude : 1.0f;
         return render_saturated(colors, saturation);
      }

      // Method to render with a specific saturation level
      std::vector<uint8_t> render_saturated(const std::vector<uint8_t>& colors, float saturation) const
      {
         assert(saturation > 0.0f);

         size_t total_pixels = static_cast<size_t>(width) * height;
         std::vector<uint8_t> colorbuf(total_pixels * 4, 0); // Assuming RGBA, initialize to transparent

         size_t ncolors = get_color_count(colors);
         if (ncolors == 0) {
            return colorbuf; // Return transparent buffer for empty color scheme
         }

         // Calculate the bin-to-pixel mapping parameters
         float bin_to_pixel_ratio = static_cast<float>(width) / num_bins;

         for (uint32_t bin = 0; bin < num_bins; ++bin) {
            float val = buffer[bin];
            float peak_val = peak_values[bin];

            // Normalize the values based on the maximum magnitude
            float normalized = std::clamp(val / saturation, 0.0f, 1.0f);
            float peak_normalized = std::clamp(peak_val / saturation, 0.0f, 1.0f);

            // Calculate the pixel range for this bin
            uint32_t start_x = static_cast<uint32_t>(bin * bin_to_pixel_ratio);
            uint32_t end_x = static_cast<uint32_t>((bin + 1) * bin_to_pixel_ratio);

            // Adjust for bar width factor
            if (bar_width_factor < 1.0f) {
               uint32_t bar_width = static_cast<uint32_t>((end_x - start_x) * bar_width_factor);
               uint32_t padding = (end_x - start_x - bar_width) / 2;
               start_x += padding;
               end_x = start_x + bar_width;
            }

            // Ensure we don't exceed image boundaries
            end_x = (std::min)(end_x, width);

            // Calculate the height of this spectrum bar
            uint32_t bar_height = static_cast<uint32_t>(normalized * height);

            // Draw the vertical bar for each pixel in this bin's range
            for (uint32_t x = start_x; x < end_x; ++x) {
               // Draw based on selected style
               switch (style) {
               case bar_style::solid: {
                  // Determine the color index based on the normalized value
                  size_t color_idx = static_cast<size_t>((ncolors - 1) * normalized + 0.5f);
                  color_idx = (std::min)(color_idx, ncolors - 1);

                  // Draw a solid color bar
                  for (uint32_t y = 0; y < bar_height; ++y) {
                     // Calculate position: y starts from bottom
                     uint32_t pos_y = height - y - 1;
                     size_t idx = static_cast<size_t>(pos_y) * width + x;

                     // Set the pixel color
                     std::copy_n(colors.begin() + color_idx * 4, 4, colorbuf.begin() + idx * 4);
                  }
                  break;
               }

               case bar_style::gradient: {
                  // Draw a gradient bar
                  for (uint32_t y = 0; y < bar_height; ++y) {
                     // Calculate position: y starts from bottom
                     uint32_t pos_y = height - y - 1;
                     size_t idx = static_cast<size_t>(pos_y) * width + x;

                     // Map the y position to a color in the gradient
                     float color_pos = static_cast<float>(y) / static_cast<float>(height);
                     size_t color_idx = static_cast<size_t>((ncolors - 1) * color_pos + 0.5f);
                     color_idx = (std::min)(color_idx, ncolors - 1);

                     // Set the pixel color
                     std::copy_n(colors.begin() + color_idx * 4, 4, colorbuf.begin() + idx * 4);
                  }
                  break;
               }

               case bar_style::segmented: {
                  // Draw a segmented bar (like LED VU meters)
                  const uint32_t segment_count = 16; // Number of segments
                  const uint32_t segment_height = height / segment_count;
                  const float segment_value = 1.0f / segment_count;

                  for (uint32_t segment = 0; segment < segment_count; ++segment) {
                     // Calculate if this segment should be lit
                     float segment_threshold = segment * segment_value;
                     if (normalized >= segment_threshold) {
                        // Calculate segment position
                        uint32_t start_y = height - (segment + 1) * segment_height;
                        uint32_t end_y = height - segment * segment_height;

                        // Determine segment color based on position
                        float color_pos = static_cast<float>(segment) / static_cast<float>(segment_count - 1);
                        size_t color_idx = static_cast<size_t>((ncolors - 1) * color_pos + 0.5f);
                        color_idx = (std::min)(color_idx, ncolors - 1);

                        // Draw the segment (leaving a small gap)
                        for (uint32_t y = start_y + 1; y < end_y - 1; ++y) {
                           size_t idx = static_cast<size_t>(y) * width + x;
                           std::copy_n(colors.begin() + color_idx * 4, 4, colorbuf.begin() + idx * 4);
                        }
                     }
                  }
                  break;
               }
               }
            }

            // Draw peak indicator if enabled
            if (show_peaks && peak_normalized > 0.0f) {
               // Calculate peak position
               uint32_t peak_pos = height - static_cast<uint32_t>(peak_normalized * height) - 1;
               if (peak_pos < height) {
                  // Draw peak indicators for each pixel in this bin's range
                  for (uint32_t x = start_x; x < end_x; ++x) {
                     size_t idx = static_cast<size_t>(peak_pos) * width + x;

                     // Use the highest color for peak indicator
                     size_t color_idx = ncolors - 1;
                     std::copy_n(colors.begin() + color_idx * 4, 4, colorbuf.begin() + idx * 4);
                  }
               }
            }
         }

         return colorbuf;
      }

      void reset() noexcept
      {
         std::fill(buffer.begin(), buffer.end(), 0.0f);
         std::fill(peak_values.begin(), peak_values.end(), 0.0f);
         std::fill(prev_buffer.begin(), prev_buffer.end(), 0.0f);
         max_magnitude = std::numeric_limits<float>::lowest();
         min_magnitude = (std::numeric_limits<float>::max)();
      }

     private:
      // Helper function to get color count from the color buffer
      static size_t get_color_count(const std::vector<uint8_t>& colors) noexcept
      {
         return colors.size() / 4; // 4 bytes per color (RGBA)
      }
   };
}
