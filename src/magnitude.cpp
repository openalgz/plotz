// Plotz Library
// For the license information refer to plotz.hpp

#include "plotz/magnitude.hpp"

namespace plotz
{
   // magnitude implementation

   magnitude::magnitude(uint32_t width_in, uint32_t height_in) noexcept
      : width(width_in), height(height_in), buffer(width * height)
   {}

   void magnitude::add_point(uint32_t x, uint32_t y, float magnitude_value) noexcept
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

   void magnitude::shift_buffer_to_non_negative()
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

   std::vector<uint8_t> magnitude::render() { return render(default_color_scheme_data); }

   std::vector<uint8_t> magnitude::render(const std::vector<uint8_t>& colors)
   {
      shift_buffer_to_non_negative();
      float saturation = max_magnitude > 0.0f ? max_magnitude : 1.0f;
      return render_saturated(colors, saturation);
   }

   std::vector<uint8_t> magnitude::render_saturated(const std::vector<uint8_t>& colors, float saturation) const
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

   void magnitude::reset() noexcept
   {
      std::fill(buffer.begin(), buffer.end(), 0.0f);
      max_magnitude = std::numeric_limits<float>::lowest();
      min_magnitude = (std::numeric_limits<float>::max)();
   }

   // magnitude_mapped implementation

   magnitude_mapped::magnitude_mapped(uint32_t width_in, uint32_t height_in, uint32_t img_width, uint32_t img_height)
      : input_width(width_in),
        input_height(height_in),
        image_width(img_width),
        image_height(img_height),
        buffer(image_width * image_height, 0.0f)
   {}

   bool magnitude_mapped::map_coordinates(uint32_t input_x, uint32_t input_y, uint32_t& image_x,
                                          uint32_t& image_y) const noexcept
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

   void magnitude_mapped::add_point(uint32_t input_x, uint32_t input_y, float magnitude_value) noexcept
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

   void magnitude_mapped::shift_buffer_to_non_negative()
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

   std::vector<uint8_t> magnitude_mapped::render() { return render(default_color_scheme_data); }

   std::vector<uint8_t> magnitude_mapped::render(const std::vector<uint8_t>& colors)
   {
      shift_buffer_to_non_negative();
      float saturation = max_magnitude > 0.0f ? max_magnitude : 1.0f;
      return render_saturated(colors, saturation);
   }

   std::vector<uint8_t> magnitude_mapped::render_saturated(const std::vector<uint8_t>& colors, float saturation) const
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

   void magnitude_mapped::reset() noexcept
   {
      std::fill(buffer.begin(), buffer.end(), 0.0f);
      max_magnitude = std::numeric_limits<float>::lowest();
      min_magnitude = (std::numeric_limits<float>::max)();
   }

   // magnitude_mapped_grid implementation

   magnitude_mapped_grid::magnitude_mapped_grid(size_t grid_size, uint32_t input_width, uint32_t input_height,
                                                uint32_t plot_width, uint32_t plot_height)
      : grid_size_(grid_size),
        plot_count_(grid_size * grid_size),
        input_width_(input_width),
        input_height_(input_height),
        plot_width_(plot_width),
        plot_height_(plot_height),
        plots_(plot_count_, magnitude_mapped(input_width, input_height, plot_width, plot_height))
   {}

   magnitude_mapped& magnitude_mapped_grid::get_plot(size_t row, size_t col)
   {
      assert(row < grid_size_ && col < grid_size_);
      return plots_[row * grid_size_ + col];
   }

   void magnitude_mapped_grid::set_plot(size_t row, size_t col, magnitude_mapped&& plot)
   {
      assert(row < grid_size_ && col < grid_size_);

      // Move the plot into the grid
      plots_[row * grid_size_ + col] = std::move(plot);

      // Update global extrema after adding the new plot
      update_global_extrema();
   }

   void magnitude_mapped_grid::add_point(size_t row, size_t col, uint32_t input_x, uint32_t input_y,
                                         float magnitude_value)
   {
      assert(row < grid_size_ && col < grid_size_);

      // Add the point to the appropriate plot
      auto& plot = plots_[row * grid_size_ + col];
      plot.add_point(input_x, input_y, magnitude_value);

      // Update global extrema
      global_max_magnitude_ = std::max(global_max_magnitude_, plot.max_magnitude);
      global_min_magnitude_ = std::min(global_min_magnitude_, plot.min_magnitude);
   }

   void magnitude_mapped_grid::update_global_extrema()
   {
      global_max_magnitude_ = std::numeric_limits<float>::lowest();
      global_min_magnitude_ = std::numeric_limits<float>::max();

      for (const auto& plot : plots_) {
         global_max_magnitude_ = std::max(global_max_magnitude_, plot.max_magnitude);
         global_min_magnitude_ = std::min(global_min_magnitude_, plot.min_magnitude);
      }
   }

   void magnitude_mapped_grid::shift_all_buffers_to_non_negative()
   {
      if (global_min_magnitude_ < 0.0f) {
         float shift = -global_min_magnitude_;

         for (auto& plot : plots_) {
            // Apply shift to each buffer
            for (auto& val : plot.buffer) {
               val += shift;
            }

            // Adjust each plot's max_magnitude
            plot.max_magnitude += shift;
            plot.min_magnitude = 0.0f;
         }

         // Update global values
         global_max_magnitude_ += shift;
         global_min_magnitude_ = 0.0f;
      }
   }

   std::vector<uint8_t> magnitude_mapped_grid::render(const std::vector<uint8_t>& colors)
   {
      update_global_extrema();
      shift_all_buffers_to_non_negative();

      // Determine the combined image dimensions
      size_t total_width = plot_width_ * grid_size_;
      size_t total_height = plot_height_ * grid_size_;

      // Prepare the combined color buffer
      std::vector<uint8_t> combined_buffer(total_width * total_height * 4, 0);

      // Use the global max for saturation
      float saturation = global_max_magnitude_ > 0.0f ? global_max_magnitude_ : 1.0f;

      // Render each plot and copy to the combined buffer
      for (size_t row = 0; row < grid_size_; ++row) {
         for (size_t col = 0; col < grid_size_; ++col) {
            // Get the plot
            auto& plot = plots_[row * grid_size_ + col];

            // Render the plot with global saturation
            auto plot_buffer = plot.render_saturated(colors, saturation);

            // Calculate the starting position in the combined buffer
            size_t start_x = col * plot_width_;
            size_t start_y = row * plot_height_;

            // Copy the plot's buffer to the combined buffer
            for (uint32_t y = 0; y < plot_height_; ++y) {
               for (uint32_t x = 0; x < plot_width_; ++x) {
                  size_t src_idx = (y * plot_width_ + x) * 4;
                  size_t dst_idx = ((start_y + y) * total_width + (start_x + x)) * 4;

                  // Copy the RGBA values
                  std::copy_n(plot_buffer.begin() + src_idx, 4, combined_buffer.begin() + dst_idx);
               }
            }
         }
      }

      return combined_buffer;
   }

   void magnitude_mapped_grid::write_png(const std::string& filename, const std::vector<uint8_t>& colors)
   {
      auto buffer = render(colors);
      size_t total_width = plot_width_ * grid_size_;
      size_t total_height = plot_height_ * grid_size_;

      plotz::write_png(filename, buffer.data(), total_width, total_height);
   }

   void magnitude_mapped_grid::reset()
   {
      for (auto& plot : plots_) {
         plot.reset();
      }

      global_max_magnitude_ = std::numeric_limits<float>::lowest();
      global_min_magnitude_ = std::numeric_limits<float>::max();
   }
}
