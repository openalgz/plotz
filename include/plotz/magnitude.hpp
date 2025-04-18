// Plotz Library
// For the license information refer to plotz.hpp

#pragma once

#include <algorithm>
#include <cassert>
#include <cmath>
#include <cstring>
#include <memory>

#include "plotz/color_scheme.hpp"
#include "plotz/write_png.hpp"

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
   
   /**
    * @brief Class to manage a grid of magnitude_mapped plots with shared scaling
    */
   class magnitude_mapped_grid
   {
   private:
      size_t grid_size_;       // Size of the grid (grid_size_ x grid_size_)
      size_t plot_count_;      // Total number of plots
      
      uint32_t input_width_;   // Width of each input plot
      uint32_t input_height_;  // Height of each input plot
      uint32_t plot_width_;    // Width of each rendered plot
      uint32_t plot_height_;   // Height of each rendered plot
      
      // Global magnitude extrema
      float global_max_magnitude_ = std::numeric_limits<float>::lowest();
      float global_min_magnitude_ = std::numeric_limits<float>::max();
      
      // Vector of magnitude_mapped instances
      std::vector<magnitude_mapped> plots_;
      
   public:
      /**
       * @brief Constructor for magnitude_mapped_grid
       * @param grid_size Size of the grid (grid_size x grid_size)
       * @param input_width Width of each input plot
       * @param input_height Height of each input plot
       * @param plot_width Width of each rendered plot
       * @param plot_height Height of each rendered plot
       */
      magnitude_mapped_grid(size_t grid_size,
                            uint32_t input_width, uint32_t input_height,
                            uint32_t plot_width, uint32_t plot_height)
      : grid_size_(grid_size), plot_count_(grid_size * grid_size),
      input_width_(input_width), input_height_(input_height),
      plot_width_(plot_width), plot_height_(plot_height),
      plots_(plot_count_, magnitude_mapped(input_width, input_height, plot_width, plot_height))
      {
      }
      
      /**
       * @brief Get a reference to a specific plot in the grid
       * @param row Row index (0 to grid_size_-1)
       * @param col Column index (0 to grid_size_-1)
       * @return Reference to the requested magnitude_mapped instance
       */
      magnitude_mapped& get_plot(size_t row, size_t col) {
         assert(row < grid_size_ && col < grid_size_);
         return plots_[row * grid_size_ + col];
      }
      
      /**
       * @brief Move a populated magnitude_mapped into the grid
       * @param row Row index (0 to grid_size_-1)
       * @param col Column index (0 to grid_size_-1)
       * @param plot The magnitude_mapped to move
       */
      void set_plot(size_t row, size_t col, magnitude_mapped&& plot) {
         assert(row < grid_size_ && col < grid_size_);
         
         // Move the plot into the grid
         plots_[row * grid_size_ + col] = std::move(plot);
         
         // Update global extrema after adding the new plot
         update_global_extrema();
      }
      
      /**
       * @brief Add a point to a specific plot
       * @param row Row index of the plot (0 to grid_size_-1)
       * @param col Column index of the plot (0 to grid_size_-1)
       * @param input_x X-coordinate in input space
       * @param input_y Y-coordinate in input space
       * @param magnitude_value Magnitude value to add
       */
      void add_point(size_t row, size_t col, uint32_t input_x, uint32_t input_y, float magnitude_value) {
         assert(row < grid_size_ && col < grid_size_);
         
         // Add the point to the appropriate plot
         auto& plot = plots_[row * grid_size_ + col];
         plot.add_point(input_x, input_y, magnitude_value);
         
         // Update global extrema
         global_max_magnitude_ = std::max(global_max_magnitude_, plot.max_magnitude);
         global_min_magnitude_ = std::min(global_min_magnitude_, plot.min_magnitude);
      }
      
      /**
       * @brief Update global min/max magnitudes from all plots
       */
      void update_global_extrema() {
         global_max_magnitude_ = std::numeric_limits<float>::lowest();
         global_min_magnitude_ = std::numeric_limits<float>::max();
         
         for (const auto& plot : plots_) {
            global_max_magnitude_ = std::max(global_max_magnitude_, plot.max_magnitude);
            global_min_magnitude_ = std::min(global_min_magnitude_, plot.min_magnitude);
         }
      }
      
      /**
       * @brief Shift all buffers to handle negative magnitudes
       */
      void shift_all_buffers_to_non_negative() {
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
      
      /**
       * @brief Render all plots to a single image using global scaling
       * @param colors Vector of color values to use for rendering
       * @return Vector of RGBA color values for the combined image
       */
      std::vector<uint8_t> render(const std::vector<uint8_t>& colors) {
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
      
      /**
       * @brief Render and save the grid to a PNG file
       * @param filename Name of the output PNG file
       * @param colors Vector of color values to use for rendering
       */
      void write_png(const std::string& filename, const std::vector<uint8_t>& colors) {
         auto buffer = render(colors);
         size_t total_width = plot_width_ * grid_size_;
         size_t total_height = plot_height_ * grid_size_;
         
         plotz::write_png(filename, buffer.data(), total_width, total_height);
      }
      
      /**
       * @brief Reset all plots
       */
      void reset() {
         for (auto& plot : plots_) {
            plot.reset();
         }
         
         global_max_magnitude_ = std::numeric_limits<float>::lowest();
         global_min_magnitude_ = std::numeric_limits<float>::max();
      }
   };
}
