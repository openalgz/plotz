// Plotz Library
// For the license information refer to plotz.hpp

#pragma once

#include <algorithm>
#include <cassert>
#include <cmath>
#include <cstring>
#include <limits>
#include <memory>
#include <vector>

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
      magnitude(uint32_t width_in, uint32_t height_in) noexcept;

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
      std::vector<float> buffer;

      /**
       * @brief Adds a point with a specific magnitude value to the buffer
       * @param x X-coordinate of the point
       * @param y Y-coordinate of the point
       * @param magnitude_value Magnitude value to store at the specified point
       * @note Points outside the plot dimensions are ignored
       */
      void add_point(uint32_t x, uint32_t y, float magnitude_value) noexcept;

      /**
       * @brief Shifts buffer values to handle negative magnitudes
       * @details If min_magnitude is negative, all values are shifted to ensure non-negative values
       */
      void shift_buffer_to_non_negative();

      /**
       * @brief Renders the buffer to a color buffer using the default color scheme
       * @return Vector of RGBA color values for each pixel
       */
      std::vector<uint8_t> render();

      /**
       * @brief Renders the buffer to a color buffer using a specified color scheme
       * @param colors Vector of color values to use for rendering
       * @return Vector of RGBA color values for each pixel
       */
      std::vector<uint8_t> render(const std::vector<uint8_t>& colors);

      /**
       * @brief Renders the buffer with a specific saturation level
       * @param colors Vector of color values to use for rendering
       * @param saturation Saturation level for normalization
       * @return Vector of RGBA color values for each pixel
       * @pre saturation must be greater than 0.0f
       */
      std::vector<uint8_t> render_saturated(const std::vector<uint8_t>& colors, float saturation) const;

      /**
       * @brief Resets the buffer and magnitude extrema
       * @details Clears all magnitude values and resets min/max tracking
       */
      void reset() noexcept;
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
      std::vector<float> buffer;

      /**
       * @brief Constructor for magnitude_mapped structure
       * @param width_in Width of the input data
       * @param height_in Height of the input data
       * @param img_width Width of the output image
       * @param img_height Height of the output image
       */
      magnitude_mapped(uint32_t width_in, uint32_t height_in, uint32_t img_width, uint32_t img_height);

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
      bool map_coordinates(uint32_t input_x, uint32_t input_y, uint32_t& image_x, uint32_t& image_y) const noexcept;

      /**
       * @brief Adds a point with a specific magnitude value to the buffer
       * @param input_x X-coordinate in input space
       * @param input_y Y-coordinate in input space
       * @param magnitude_value Magnitude value to add at the specified point
       * @details Maps input coordinates to potentially multiple pixels in the output image
       */
      void add_point(uint32_t input_x, uint32_t input_y, float magnitude_value) noexcept;

      /**
       * @brief Shifts buffer values to handle negative magnitudes
       * @details If min_magnitude is negative, all values are shifted to ensure non-negative values
       */
      void shift_buffer_to_non_negative();

      /**
       * @brief Renders the buffer to a color buffer using the default color scheme
       * @return Vector of RGBA color values for each pixel
       */
      std::vector<uint8_t> render();

      /**
       * @brief Renders the buffer to a color buffer using a specified color scheme
       * @param colors Vector of color values to use for rendering
       * @return Vector of RGBA color values for each pixel
       */
      std::vector<uint8_t> render(const std::vector<uint8_t>& colors);

      /**
       * @brief Renders the buffer with a specific saturation level
       * @param colors Vector of color values to use for rendering
       * @param saturation Saturation level for normalization
       * @return Vector of RGBA color values for each pixel
       * @pre saturation must be greater than 0.0f
       */
      std::vector<uint8_t> render_saturated(const std::vector<uint8_t>& colors, float saturation) const;

      /**
       * @brief Resets the buffer and magnitude extrema
       * @details Clears all magnitude values and resets min/max tracking
       */
      void reset() noexcept;
   };

   /**
    * @brief Class to manage a grid of magnitude_mapped plots with shared scaling
    */
   class magnitude_mapped_grid
   {
     private:
      size_t grid_size_; // Size of the grid (grid_size_ x grid_size_)
      size_t plot_count_; // Total number of plots

      uint32_t input_width_; // Width of each input plot
      uint32_t input_height_; // Height of each input plot
      uint32_t plot_width_; // Width of each rendered plot
      uint32_t plot_height_; // Height of each rendered plot

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
      magnitude_mapped_grid(size_t grid_size, uint32_t input_width, uint32_t input_height, uint32_t plot_width,
                            uint32_t plot_height);

      /**
       * @brief Get a reference to a specific plot in the grid
       * @param row Row index (0 to grid_size_-1)
       * @param col Column index (0 to grid_size_-1)
       * @return Reference to the requested magnitude_mapped instance
       */
      magnitude_mapped& get_plot(size_t row, size_t col);

      /**
       * @brief Move a populated magnitude_mapped into the grid
       * @param row Row index (0 to grid_size_-1)
       * @param col Column index (0 to grid_size_-1)
       * @param plot The magnitude_mapped to move
       */
      void set_plot(size_t row, size_t col, magnitude_mapped&& plot);

      /**
       * @brief Add a point to a specific plot
       * @param row Row index of the plot (0 to grid_size_-1)
       * @param col Column index of the plot (0 to grid_size_-1)
       * @param input_x X-coordinate in input space
       * @param input_y Y-coordinate in input space
       * @param magnitude_value Magnitude value to add
       */
      void add_point(size_t row, size_t col, uint32_t input_x, uint32_t input_y, float magnitude_value);

      /**
       * @brief Update global min/max magnitudes from all plots
       */
      void update_global_extrema();

      /**
       * @brief Shift all buffers to handle negative magnitudes
       */
      void shift_all_buffers_to_non_negative();

      /**
       * @brief Render all plots to a single image using global scaling
       * @param colors Vector of color values to use for rendering
       * @return Vector of RGBA color values for the combined image
       */
      std::vector<uint8_t> render(const std::vector<uint8_t>& colors);

      /**
       * @brief Render and save the grid to a PNG file
       * @param filename Name of the output PNG file
       * @param colors Vector of color values to use for rendering
       */
      void write_png(const std::string& filename, const std::vector<uint8_t>& colors);

      /**
       * @brief Reset all plots
       */
      void reset();
   };
}
