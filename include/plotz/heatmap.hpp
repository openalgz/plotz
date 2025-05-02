// Plotz Library
// For the license information refer to plotz.hpp

#pragma once

// Originally from: https://github.com/lucasb-eyer/libheatmap

#include <algorithm>
#include <cassert>
#include <cmath>
#include <cstring>
#include <functional>
#include <memory>

#include "plotz/color_scheme.hpp"

namespace plotz
{
   struct heatmap_stamp final
   {
      heatmap_stamp(uint32_t width, uint32_t height, const std::vector<float>& data) noexcept;
      heatmap_stamp(const heatmap_stamp&) = default;
      heatmap_stamp(heatmap_stamp&&) = default;
      heatmap_stamp& operator=(const heatmap_stamp&) = default;
      heatmap_stamp& operator=(heatmap_stamp&&) = default;

      // Constructor to generate a default round stamp
      heatmap_stamp(uint32_t radius);

      // Constructor with custom distance shape function
      heatmap_stamp(unsigned radius, std::function<float(float)> distshape);

      uint32_t get_width() const { return w; }
      uint32_t get_height() const { return h; }
      const std::vector<float>& get_buffer() const { return buffer; }

     private:
      uint32_t w, h; // Dimensions
      std::vector<float> buffer;
   };

   struct heatmap final
   {
      heatmap(uint32_t width_in, uint32_t height_in) noexcept;
      heatmap(const heatmap&) noexcept = default;
      heatmap(heatmap&&) noexcept = default;
      heatmap& operator=(const heatmap&) noexcept = default;
      heatmap& operator=(heatmap&&) noexcept = default;

      uint32_t width{}, height{}; // Dimensions
      float max_heat{}; // Maximum heat value
      std::vector<float> buffer = std::vector<float>(width * height);

      void add_point(uint32_t x, uint32_t y);
      void add_point_with_stamp(uint32_t x, uint32_t y, const heatmap_stamp& stamp);
      void add_weighted_point(uint32_t x, uint32_t y, float weight);
      void add_weighted_point_with_stamp(uint32_t x, uint32_t y, float weight, const heatmap_stamp& stamp);

      // Methods to render the heatmap
      std::vector<uint8_t> render() const;
      std::vector<uint8_t> render(const auto& colors) const;
      std::vector<uint8_t> render_saturated(const auto& colors, float saturation) const;
   };
}
