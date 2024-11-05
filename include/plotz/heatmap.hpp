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
   inline const std::vector<float> default_stamp_data = {
      // (Data from stamp_default_4_data in the original code)
      0.0f,       0.0f,       0.1055728f, 0.1753789f, 0.2f, 0.1753789f, 0.1055728f, 0.0f,       0.0f,
      0.0f,       0.1514719f, 0.2788897f, 0.3675445f, 0.4f, 0.3675445f, 0.2788897f, 0.1514719f, 0.0f,
      0.1055728f, 0.2788897f, 0.4343146f, 0.5527864f, 0.6f, 0.5527864f, 0.4343146f, 0.2788897f, 0.1055728f,
      0.1753789f, 0.3675445f, 0.5527864f, 0.7171573f, 0.8f, 0.7171573f, 0.5527864f, 0.3675445f, 0.1753789f,
      0.2f,       0.4f,       0.6f,       0.8f,       1.0f, 0.8f,       0.6f,       0.4f,       0.2f,
      0.1753789f, 0.3675445f, 0.5527864f, 0.7171573f, 0.8f, 0.7171573f, 0.5527864f, 0.3675445f, 0.1753789f,
      0.1055728f, 0.2788897f, 0.4343146f, 0.5527864f, 0.6f, 0.5527864f, 0.4343146f, 0.2788897f, 0.1055728f,
      0.0f,       0.1514719f, 0.2788897f, 0.3675445f, 0.4f, 0.3675445f, 0.2788897f, 0.1514719f, 0.0f,
      0.0f,       0.0f,       0.1055728f, 0.1753789f, 0.2f, 0.1753789f, 0.1055728f, 0.0f,       0.0f,
   };

   struct heatmap_stamp final
   {
      heatmap_stamp(uint32_t width, uint32_t height, const std::vector<float>& data) noexcept
         : w(width), h(height), buffer(data)
      {
         assert(data.size() == w * h);
      }

      heatmap_stamp(const heatmap_stamp&) = default;
      heatmap_stamp(heatmap_stamp&&) = default;
      heatmap_stamp& operator=(const heatmap_stamp&) = default;
      heatmap_stamp& operator=(heatmap_stamp&&) = default;

      // Constructor to generate a default round stamp
      heatmap_stamp(uint32_t radius)
      {
         uint32_t d = 2 * radius + 1;
         w = d;
         h = d;
         buffer.resize(w * h);

         for (uint32_t y = 0; y < h; ++y) {
            for (uint32_t x = 0; x < w; ++x) {
               float dist = std::sqrt(static_cast<float>((x - radius) * (x - radius) + (y - radius) * (y - radius))) /
                            static_cast<float>(radius + 1);
               float clamped_ds = std::clamp(dist, 0.0f, 1.0f);
               buffer[y * w + x] = 1.0f - clamped_ds;
            }
         }
      }

      // Constructor with custom distance shape function
      heatmap_stamp(unsigned radius, std::function<float(float)> distshape)
      {
         uint32_t d = 2 * radius + 1;
         w = d;
         h = d;
         buffer.resize(w * h);

         for (uint32_t y = 0; y < h; ++y) {
            for (uint32_t x = 0; x < w; ++x) {
               float dist = std::sqrt(static_cast<float>((x - radius) * (x - radius) + (y - radius) * (y - radius))) /
                            static_cast<float>(radius + 1);
               float ds = distshape(dist);
               float clamped_ds = std::clamp(ds, 0.0f, 1.0f);
               buffer[y * w + x] = 1.0f - clamped_ds;
            }
         }
      }

      uint32_t get_width() const { return w; }
      uint32_t get_height() const { return h; }
      const std::vector<float>& get_buffer() const { return buffer; }

     private:
      uint32_t w, h; // Dimensions
      std::vector<float> buffer;
   };

   inline const heatmap_stamp default_heatmap_stamp{9, 9, default_stamp_data};

   inline size_t get_color_count(auto& colors) { return colors.size() / 4; }

   struct heatmap final
   {
      heatmap(uint32_t width_in, uint32_t height_in) noexcept : width(width_in), height(height_in) {}
      heatmap(const heatmap&) noexcept = default;
      heatmap(heatmap&&) noexcept = default;
      heatmap& operator=(const heatmap&) noexcept = default;
      heatmap& operator=(heatmap&&) noexcept = default;

      uint32_t width{}, height{}; // Dimensions
      float max_heat{}; // Maximum heat value
      std::vector<float> buffer = std::vector<float>(width * height);

      void add_point(uint32_t x, uint32_t y) { add_point_with_stamp(x, y, default_heatmap_stamp); }

      void add_point_with_stamp(uint32_t x, uint32_t y, const heatmap_stamp& stamp)
      {
         if (x >= width || y >= height) return;

         const auto stamp_w = stamp.get_width();
         const auto stamp_h = stamp.get_height();
         const auto& stamp_buf = stamp.get_buffer();

         uint32_t x0 = x < stamp_w / 2 ? (stamp_w / 2 - x) : 0;
         uint32_t y0 = y < stamp_h / 2 ? (stamp_h / 2 - y) : 0;
         uint32_t x1 = (x + stamp_w / 2) < width ? stamp_w : stamp_w / 2 + (width - x);
         uint32_t y1 = (y + stamp_h / 2) < height ? stamp_h : stamp_h / 2 + (height - y);

         for (uint32_t iy = y0; iy < y1; ++iy) {
            uint32_t buf_y = (y + iy) - stamp_h / 2;
            uint32_t buf_line_idx = buf_y * width + (x + x0) - stamp_w / 2;
            uint32_t stamp_line_idx = iy * stamp_w + x0;

            for (uint32_t ix = x0; ix < x1; ++ix, ++buf_line_idx, ++stamp_line_idx) {
               float stamp_value = stamp_buf[stamp_line_idx];
               buffer[buf_line_idx] += stamp_value;
               if (buffer[buf_line_idx] > max_heat) max_heat = buffer[buf_line_idx];
            }
         }
      }

      void add_weighted_point(uint32_t x, uint32_t y, float weight)
      {
         add_weighted_point_with_stamp(x, y, weight, default_heatmap_stamp);
      }

      void add_weighted_point_with_stamp(uint32_t x, uint32_t y, float weight, const heatmap_stamp& stamp)
      {
         if (x >= width || y >= height || weight < 0.0f) return;

         const auto stamp_w = stamp.get_width();
         const auto stamp_h = stamp.get_height();
         const auto& stamp_buffer = stamp.get_buffer();

         uint32_t x0 = x < stamp_w / 2 ? (stamp_w / 2 - x) : 0;
         uint32_t y0 = y < stamp_h / 2 ? (stamp_h / 2 - y) : 0;
         uint32_t x1 = (x + stamp_w / 2) < width ? stamp_w : stamp_w / 2 + (width - x);
         uint32_t y1 = (y + stamp_h / 2) < height ? stamp_h : stamp_h / 2 + (height - y);

         for (uint32_t iy = y0; iy < y1; ++iy) {
            uint32_t buf_y = (y + iy) - stamp_h / 2;
            uint32_t buf_line_idx = buf_y * width + (x + x0) - stamp_w / 2;
            uint32_t stamp_line_idx = iy * stamp_w + x0;

            for (uint32_t ix = x0; ix < x1; ++ix, ++buf_line_idx, ++stamp_line_idx) {
               float stamp_value = stamp_buffer[stamp_line_idx] * weight;
               buffer[buf_line_idx] += stamp_value;
               if (buffer[buf_line_idx] > max_heat) max_heat = buffer[buf_line_idx];
            }
         }
      }

      // Methods to render the heatmap
      std::vector<uint8_t> render() const { return render(default_color_scheme_data); }

      std::vector<uint8_t> render(const auto& colors) const
      {
         float saturation = max_heat > 0.0f ? max_heat : 1.0f;
         return render_saturated(colors, saturation);
      }

      std::vector<uint8_t> render_saturated(const auto& colors, float saturation) const
      {
         assert(saturation > 0.0f);

         size_t total_pixels = width * height;
         std::vector<uint8_t> colorbuf(total_pixels * 4);

         size_t ncolors = get_color_count(colors);

         for (size_t idx = 0; idx < total_pixels; ++idx) {
            float val = buffer[idx];
            val = std::clamp(val / saturation, 0.0f, 1.0f);

            size_t color_idx = static_cast<size_t>((ncolors - 1) * val + 0.5f);
            color_idx = (std::min)(color_idx, ncolors - 1);

            std::copy_n(colors.begin() + color_idx * 4, 4, colorbuf.begin() + idx * 4);
         }

         return colorbuf;
      }
   };
}
