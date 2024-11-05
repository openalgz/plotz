// Plotz Library
// For the license information refer to plotz.hpp

#pragma once

#include <cstdint>
#include <vector>

namespace plotz
{
   using rgba = std::array<uint8_t, 4>;

   inline constexpr rgba white{255, 255, 255, 255};
   inline constexpr rgba black{0, 0, 0, 255};

   inline std::vector<uint8_t> interpolate_color(uint8_t r1, uint8_t g1, uint8_t b1, uint8_t a1, uint8_t r2, uint8_t g2,
                                                 uint8_t b2, uint8_t a2, int steps)
   {
      std::vector<uint8_t> colors;
      for (int i = 0; i < steps; ++i) {
         float ratio = static_cast<float>(i) / (steps - 1);
         uint8_t r = static_cast<uint8_t>(r1 + ratio * (r2 - r1));
         uint8_t g = static_cast<uint8_t>(g1 + ratio * (g2 - g1));
         uint8_t b = static_cast<uint8_t>(b1 + ratio * (b2 - b1));
         uint8_t a = static_cast<uint8_t>(a1 + ratio * (a2 - a1));
         colors.push_back(r);
         colors.push_back(g);
         colors.push_back(b);
         colors.push_back(a);
      }
      return colors;
   }

   inline std::vector<uint8_t> make_color_scheme(const std::vector<rgba>& key_colors, int steps_between_keys = 100)
   {
      std::vector<uint8_t> data;
      data.reserve((key_colors.size() - 1) * steps_between_keys * 4);

      for (size_t i = 0; i < key_colors.size() - 1; ++i) {
         const auto& start = key_colors[i];
         const auto& end = key_colors[i + 1];
         std::vector<uint8_t> segment = interpolate_color(start[0], start[1], start[2], start[3], end[0], end[1],
                                                          end[2], end[3], steps_between_keys);
         data.insert(data.end(), segment.begin(), segment.end());
      }
      return data;
   }

   // Define key colors for the rainbow
   inline const std::vector<rgba> raindbow_key_colors{
      {148, 0, 211, 255}, // Violet
      {75, 0, 130, 255}, // Indigo
      {0, 0, 255, 255}, // Blue
      {0, 255, 0, 255}, // Green
      {255, 255, 0, 255}, // Yellow
      {255, 127, 0, 255}, // Orange
      {255, 0, 0, 255} // Red
   };

   inline const std::vector<rgba> viridis_key_colors{
      {68, 1, 84, 255}, // Dark Purple
      {72, 35, 116, 255}, // Purple
      {64, 67, 135, 255}, // Blue
      {52, 94, 141, 255}, // Blue-Green
      {33, 145, 140, 255}, // Green
      {94, 201, 98, 255}, // Yellow-Green
      {253, 231, 37, 255} // Yellow
   };

   inline const std::vector<rgba> jet_key_colors{
      {0, 0, 131, 255}, // Dark Blue
      {0, 60, 170, 255}, // Blue
      {5, 255, 255, 255}, // Cyan
      {255, 255, 0, 255}, // Yellow
      {250, 0, 0, 255}, // Red
      {128, 0, 0, 255} // Dark Red
   };

   inline const std::vector<rgba> soft_key_colors{
      {30, 30, 150, 255}, // Dark Blue
      {50, 50, 200, 255}, // Blue
      {50, 120, 220, 255}, // Blue-Grey
      {180, 180, 180, 255}, // Light Grey
      {220, 140, 80, 255}, // Brownish Orange
      {200, 80, 80, 255}, // Dark Red
      {150, 50, 50, 255} // Very Dark Red
   };

   inline const std::vector<rgba> inferno_key_colors{
      {0, 0, 4, 255}, // Very Dark Purple
      {68, 1, 84, 255}, // Dark Purple
      {148, 64, 161, 255}, // Purple
      {236, 112, 199, 255}, // Pink
      {253, 181, 98, 255}, // Orange
      {253, 231, 37, 255}, // Yellow
      {252, 255, 164, 255} // Light Yellow
   };

   inline const std::vector<rgba> turbo_key_colors{
      {48, 18, 59, 255}, // Dark Purple
      {49, 54, 149, 255}, // Blue
      {33, 113, 181, 255}, // Blue-Green
      {94, 201, 98, 255}, // Green
      {253, 231, 37, 255}, // Yellow
      {224, 163, 0, 255}, // Orange
      {136, 0, 0, 255} // Dark Red
   };

   inline const std::vector<rgba> pastel_key_colors{
      {151, 136, 157, 255}, // Pastel Purple
      {152, 154, 202, 255}, // Pastel Blue
      {144, 184, 218, 255}, // Pastel Blue-Green
      {174, 228, 176, 255}, // Pastel Green
      {254, 243, 146, 255}, // Pastel Yellow
      {239, 209, 128, 255}, // Pastel Orange
      {195, 127, 127, 255} // Pastel Red
   };

   inline const std::vector<rgba> temperature_key_colors{
      {48, 18, 59, 255}, // Dark Purple
      {49, 54, 149, 255}, // Blue
      {253, 231, 37, 255}, // Yellow
      {224, 163, 0, 255}, // Orange
      {136, 0, 0, 255} // Dark Red
   };

   inline const std::vector<uint8_t> default_color_scheme_data = make_color_scheme(temperature_key_colors);
}
