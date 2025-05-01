// Plotz Library
// For the license information refer to plotz.hpp

#pragma once

#include <algorithm>
#include <array>
#include <cassert>
#include <cmath>
#include <cstring>
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
           background_color{0, 0, 0, 0} // Default transparent background (alpha = 0)
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

      bar_style style = bar_style::solid; // Default to solid style
      float peak_decay{}; // Rate at which peaks decay (0 = no decay)
      bool show_peaks = false; // Whether to show peak indicators
      float bar_width_factor = 0.8f; // Width of bars relative to bin spacing (0-1)

      std::array<uint8_t, 4> background_color; // RGBA background color

      // Set background color with separate R, G, B, A components
      void set_background_color(uint8_t r, uint8_t g, uint8_t b, uint8_t a = 255) noexcept
      {
         background_color = {r, g, b, a};
      }

      // Set background color with an rgba struct
      void set_background_color(const rgba& color) noexcept
      {
         background_color = {color[0], color[1], color[2], color[3]};
      }

      // Update the entire spectrum with new magnitude values
      void update(const std::vector<float>& magnitudes) noexcept;

      // Update a single bin's magnitude
      void update_bin(uint32_t bin, float magnitude_value) noexcept;

      // Shift buffer values to handle negative magnitudes
      void shift_buffer_to_non_negative();

      // Render the spectrum to a color buffer using default color scheme
      std::vector<uint8_t> render();

      // Render the spectrum to a color buffer using specified color scheme
      std::vector<uint8_t> render(const std::vector<uint8_t>& colors);

      // Method to render with a specific saturation level
      std::vector<uint8_t> render_saturated(const std::vector<uint8_t>& colors, float saturation) const;

      void reset() noexcept
      {
         std::fill(buffer.begin(), buffer.end(), 0.0f);
         std::fill(peak_values.begin(), peak_values.end(), 0.0f);
         max_magnitude = std::numeric_limits<float>::lowest();
         min_magnitude = (std::numeric_limits<float>::max)();
      }

     private:
      // Helper function to get color count from the color buffer
      static size_t get_color_count(const std::vector<uint8_t>& colors) noexcept
      {
         return colors.size() / 4; // 4 bytes per color (RGBA)
      }

      // Render when there are fewer bins than pixels (expand bins to pixels)
      void render_bins_to_pixels(const std::vector<uint8_t>& colors, float saturation, std::vector<uint8_t>& colorbuf,
                                 size_t ncolors) const;

      // Render when there are more bins than pixels (aggregate bins to pixels)
      void render_pixels_from_bins(const std::vector<uint8_t>& colors, float saturation, std::vector<uint8_t>& colorbuf,
                                   size_t ncolors) const;
   };
}
