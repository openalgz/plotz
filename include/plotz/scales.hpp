// Plotz Library
// For the license information refer to plotz.hpp

#pragma once

#include <algorithm>
#include <array>
#include <cmath>
#include <format>
#include <optional>
#include <string>
#include <vector>

#include "plotz/plot_traits.hpp"
#include "plotz/render_text.hpp"

namespace plotz
{
   enum class scale_type { linear, logarithmic };

   // Value mapping function type
   using value_mapper = std::function<double(double)>;

   struct scale_options
   {
      // Visual options
      std::array<uint8_t, 4> color{255, 255, 255, 255}; // Default white
      int line_width = 1; // Line width in pixels
      bool draw_grid_lines = false; // Whether to draw grid lines across the plot
      float grid_line_alpha = 0.3f; // Alpha value for grid lines (when enabled)

      // Scale options
      bool show_x_axis = true; // Whether to show the X axis
      bool show_y_axis = true; // Whether to show the Y axis
      scale_type x_scale_type = scale_type::linear; // Type of X scale
      scale_type y_scale_type = scale_type::linear; // Type of Y scale

      // Tick mark options
      int x_tick_count = 5; // Number of tick marks on X axis
      int y_tick_count = 5; // Number of tick marks on Y axis
      int tick_length = 5; // Length of tick marks in pixels

      // Label options
      bool show_labels = true; // Whether to show numerical labels
      float font_size_percentage = 2.0f; // Font size as percentage of image height
      std::string font_filename = ""; // Font file to use (must be set)
      std::array<uint8_t, 4> text_color{255, 255, 255, 255}; // Text color (default white)
      int label_precision = 2; // Decimal precision for labels
      bool scientific_notation = false; // Use scientific notation for labels

      // Axis value ranges (these can be set automatically from plot data)
      std::optional<double> x_min; // Minimum X value (optional)
      std::optional<double> x_max; // Maximum X value (optional)
      std::optional<double> y_min; // Minimum Y value (optional)
      std::optional<double> y_max; // Maximum Y value (optional)

      // Value mappers for custom unit conversion
      std::optional<value_mapper> x_mapper; // Function to map X values (e.g., bins to frequency)
      std::optional<value_mapper> y_mapper; // Function to map Y values (e.g., magnitude to dB)

      // Axis labels
      std::string x_label = "X"; // Label for X axis
      std::string y_label = "Y"; // Label for Y axis
      bool show_axis_labels = false; // Whether to show axis labels

      // Axis positions and margins
      int left_margin = 50; // Left margin in pixels
      int right_margin = 20; // Right margin in pixels
      int bottom_margin = 30; // Bottom margin in pixels
      int top_margin = 20; // Top margin in pixels
   };

   class scales
   {
     public:
      scales(uint32_t width, uint32_t height) noexcept : width_(width), height_(height) {}

      // Add scales to an existing image buffer
      void render(uint8_t* image_buffer, const scale_options& options)
      {
         // Validate options
         scale_options validated_options = validate_options(options);

         // Calculate the plot area within margins
         plot_area_.left = validated_options.left_margin;
         plot_area_.right = width_ - validated_options.right_margin;
         plot_area_.bottom = height_ - validated_options.bottom_margin;
         plot_area_.top = validated_options.top_margin;

         if (validated_options.font_filename.empty() && validated_options.show_labels) {
            throw std::runtime_error("Font filename must be set when labels are enabled");
         }

         // Render the axes and scales
         if (validated_options.draw_grid_lines) {
            draw_grid(image_buffer, validated_options);
         }

         if (validated_options.show_x_axis) {
            draw_x_axis(image_buffer, validated_options);
         }

         if (validated_options.show_y_axis) {
            draw_y_axis(image_buffer, validated_options);
         }

         // Draw axis labels if enabled
         if (validated_options.show_axis_labels) {
            draw_axis_labels(image_buffer, validated_options);
         }
      }

      // Convenience method to render with a plot
      void render_with_plot(uint8_t* image_buffer, const auto& plot, scale_options options)
      {
         // Extract data ranges from the plot and apply to options
         apply_plot_traits(plot, options);

         // Render with updated options
         render(image_buffer, options);
      }

      // Convenience method to render a plot with scales in one step
      std::vector<uint8_t> render_plot(auto& plot, scale_options options)
      {
         // Render the plot
         std::vector<uint8_t> image = plot.render();

         // Apply scales
         render_with_plot(image.data(), plot, options);

         return image;
      }

      // Get the content area (the part of the image where the plot data should go)
      struct rect
      {
         int left, top, right, bottom;

         int width() const { return right - left; }
         int height() const { return bottom - top; }
      };

      rect get_content_area() const { return plot_area_; }

     private:
      uint32_t width_;
      uint32_t height_;
      rect plot_area_{0, 0, 0, 0};

      // Apply plot traits to options
      template <typename PlotType>
      void apply_plot_traits(const PlotType& plot, scale_options& options)
      {
         // Set axis ranges if not explicitly provided
         if (!options.x_min.has_value()) {
            options.x_min = plot_traits<PlotType>::get_min_x(plot);
         }

         if (!options.x_max.has_value()) {
            options.x_max = plot_traits<PlotType>::get_max_x(plot);
         }

         if (!options.y_min.has_value()) {
            options.y_min = plot_traits<PlotType>::get_min_y(plot);
         }

         if (!options.y_max.has_value()) {
            options.y_max = plot_traits<PlotType>::get_max_y(plot);
         }

         // Set axis labels if not explicitly provided
         if (options.x_label == "X") {
            options.x_label = plot_traits<PlotType>::get_x_label(plot);
         }

         if (options.y_label == "Y") {
            options.y_label = plot_traits<PlotType>::get_y_label(plot);
         }
      }

      // Validate options and provide defaults for missing values
      scale_options validate_options(const scale_options& options)
      {
         scale_options validated = options;

         // Ensure we have valid min/max values
         if (!validated.x_min.has_value()) validated.x_min = 0.0;
         if (!validated.x_max.has_value()) validated.x_max = 1.0;
         if (!validated.y_min.has_value()) validated.y_min = 0.0;
         if (!validated.y_max.has_value()) validated.y_max = 1.0;

         // Ensure min < max
         if (validated.x_min.value() >= validated.x_max.value()) {
            validated.x_max = validated.x_min.value() + 1.0;
         }

         if (validated.y_min.value() >= validated.y_max.value()) {
            validated.y_max = validated.y_min.value() + 1.0;
         }

         // Fix logarithmic scales that have invalid ranges
         if (validated.x_scale_type == scale_type::logarithmic && validated.x_min.value() <= 0.0) {
            validated.x_min = 1.0;
         }

         if (validated.y_scale_type == scale_type::logarithmic && validated.y_min.value() <= 0.0) {
            validated.y_min = 1.0;
         }

         return validated;
      }

      // Helper to draw a horizontal line
      void draw_horizontal_line(uint8_t* image_buffer, int y, int x1, int x2, const std::array<uint8_t, 4>& color,
                                int line_width)
      {
         for (int ly = 0; ly < line_width; ++ly) {
            int py = y - ly / 2 + (ly % 2 == 0 ? 0 : 1);
            if (py < 0 || py >= static_cast<int>(height_)) continue;

            for (int x = x1; x <= x2; ++x) {
               if (x < 0 || x >= static_cast<int>(width_)) continue;

               size_t idx = (py * width_ + x) * 4;

               // Simple alpha blending
               float alpha = color[3] / 255.0f;
               float inv_alpha = 1.0f - alpha;

               image_buffer[idx] = static_cast<uint8_t>(image_buffer[idx] * inv_alpha + color[0] * alpha);
               image_buffer[idx + 1] = static_cast<uint8_t>(image_buffer[idx + 1] * inv_alpha + color[1] * alpha);
               image_buffer[idx + 2] = static_cast<uint8_t>(image_buffer[idx + 2] * inv_alpha + color[2] * alpha);
               image_buffer[idx + 3] = 255; // Ensure full opacity for the line
            }
         }
      }

      // Helper to draw a vertical line
      void draw_vertical_line(uint8_t* image_buffer, int x, int y1, int y2, const std::array<uint8_t, 4>& color,
                              int line_width)
      {
         for (int lx = 0; lx < line_width; ++lx) {
            int px = x - lx / 2 + (lx % 2 == 0 ? 0 : 1);
            if (px < 0 || px >= static_cast<int>(width_)) continue;

            for (int y = y1; y <= y2; ++y) {
               if (y < 0 || y >= static_cast<int>(height_)) continue;

               size_t idx = (y * width_ + px) * 4;

               // Simple alpha blending
               float alpha = color[3] / 255.0f;
               float inv_alpha = 1.0f - alpha;

               image_buffer[idx] = static_cast<uint8_t>(image_buffer[idx] * inv_alpha + color[0] * alpha);
               image_buffer[idx + 1] = static_cast<uint8_t>(image_buffer[idx + 1] * inv_alpha + color[1] * alpha);
               image_buffer[idx + 2] = static_cast<uint8_t>(image_buffer[idx + 2] * inv_alpha + color[2] * alpha);
               image_buffer[idx + 3] = 255; // Ensure full opacity for the line
            }
         }
      }

      // Draw the X axis with tick marks and labels
      void draw_x_axis(uint8_t* image_buffer, const scale_options& options)
      {
         // Determine the Y position for the X axis
         int y_pos = plot_area_.bottom;

         // Draw the main axis line
         draw_horizontal_line(image_buffer, y_pos, plot_area_.left, plot_area_.right, options.color,
                              options.line_width);

         // Draw tick marks and labels
         for (int i = 0; i <= options.x_tick_count; ++i) {
            double ratio = static_cast<double>(i) / options.x_tick_count;
            int x_pos = plot_area_.left + static_cast<int>(ratio * plot_area_.width());

            // Draw the tick mark
            draw_vertical_line(image_buffer, x_pos, y_pos, y_pos + options.tick_length, options.color,
                               options.line_width);

            if (options.show_labels) {
               // Calculate the value at this tick
               double value = options.x_min.value() + ratio * (options.x_max.value() - options.x_min.value());
               if (options.x_scale_type == scale_type::logarithmic) {
                  double log_min = std::log10(options.x_min.value());
                  double log_max = std::log10(options.x_max.value());
                  value = std::pow(10.0, log_min + ratio * (log_max - log_min));
               }

               // Apply mapper if present
               if (options.x_mapper) {
                  value = options.x_mapper.value()(value);
               }

               // Format the label
               std::string label;
               if (options.scientific_notation) {
                  label = std::format("{:.{}e}", value, options.label_precision);
               }
               else {
                  label = std::format("{:.{}f}", value, options.label_precision);
               }

               // Draw the label below the tick
               render_text_centered(image_buffer, label, x_pos, y_pos + options.tick_length + 10, options);
            }
         }
      }

      // Draw the Y axis with tick marks and labels
      void draw_y_axis(uint8_t* image_buffer, const scale_options& options)
      {
         // Determine the X position for the Y axis
         int x_pos = plot_area_.left;

         // Draw the main axis line
         draw_vertical_line(image_buffer, x_pos, plot_area_.top, plot_area_.bottom, options.color, options.line_width);

         // Draw tick marks and labels
         for (int i = 0; i <= options.y_tick_count; ++i) {
            double ratio = static_cast<double>(i) / options.y_tick_count;
            int y_pos = plot_area_.bottom - static_cast<int>(ratio * plot_area_.height());

            // Draw the tick mark
            draw_horizontal_line(image_buffer, y_pos, x_pos - options.tick_length, x_pos, options.color,
                                 options.line_width);

            if (options.show_labels) {
               // Calculate the value at this tick
               double value = options.y_min.value() + ratio * (options.y_max.value() - options.y_min.value());
               if (options.y_scale_type == scale_type::logarithmic) {
                  double log_min = std::log10(options.y_min.value());
                  double log_max = std::log10(options.y_max.value());
                  value = std::pow(10.0, log_min + ratio * (log_max - log_min));
               }

               // Apply mapper if present
               if (options.y_mapper) {
                  value = options.y_mapper.value()(value);
               }

               // Format the label
               std::string label;
               if (options.scientific_notation) {
                  label = std::format("{:.{}e}", value, options.label_precision);
               }
               else {
                  label = std::format("{:.{}f}", value, options.label_precision);
               }

               // Draw the label to the left of the tick
               render_text_right_aligned(image_buffer, label, x_pos - options.tick_length - 5, y_pos, options);
            }
         }
      }

      // Draw axis labels
      void draw_axis_labels(uint8_t* image_buffer, const scale_options& options)
      {
         if (!options.x_label.empty() && options.show_x_axis) {
            // Draw X axis label centered below the axis
            int x_pos = plot_area_.left + plot_area_.width() / 2;
            int y_pos = height_ - options.bottom_margin / 2;

            render_text_centered(image_buffer, options.x_label, x_pos, y_pos, options);
         }

         if (!options.y_label.empty() && options.show_y_axis) {
            // Draw Y axis label centered to the left of the axis
            // Note: For better vertical text, you might want to implement a specialized vertical text renderer
            int x_pos = options.left_margin / 3;
            int y_pos = plot_area_.top + plot_area_.height() / 2;

            // For now, just render it horizontally
            render_text_rotated(image_buffer, options.y_label, x_pos, y_pos, 90.0f, options);
         }
      }

      // Draw grid lines
      void draw_grid(uint8_t* image_buffer, const scale_options& options)
      {
         // Create a semi-transparent color for grid lines
         std::array<uint8_t, 4> grid_color = options.color;
         grid_color[3] = static_cast<uint8_t>(options.grid_line_alpha * 255);

         // Draw vertical grid lines
         for (int i = 0; i <= options.x_tick_count; ++i) {
            double ratio = static_cast<double>(i) / options.x_tick_count;
            int x_pos = plot_area_.left + static_cast<int>(ratio * plot_area_.width());

            // Skip the main axes to avoid drawing over them
            if (i > 0 && i < options.x_tick_count) {
               draw_vertical_line(image_buffer, x_pos, plot_area_.top, plot_area_.bottom, grid_color, 1);
            }
         }

         // Draw horizontal grid lines
         for (int i = 0; i <= options.y_tick_count; ++i) {
            double ratio = static_cast<double>(i) / options.y_tick_count;
            int y_pos = plot_area_.bottom - static_cast<int>(ratio * plot_area_.height());

            // Skip the main axes to avoid drawing over them
            if (i > 0 && i < options.y_tick_count) {
               draw_horizontal_line(image_buffer, y_pos, plot_area_.left, plot_area_.right, grid_color, 1);
            }
         }
      }

      // Helper to render text centered at a position
      void render_text_centered(uint8_t* image_buffer, const std::string& text, int x, int y,
                                const scale_options& options)
      {
         try {
            // Register the font if not already done
            ft_context.register_font(options.font_filename);

            // Get the font face for rendering
            FT_Face face = ft_context.get_font(options.font_filename);

            // Set font size
            int font_size = static_cast<int>(height_ * (options.font_size_percentage / 100.0f));
            FT_Set_Pixel_Sizes(face, 0, font_size);

            // Calculate text dimensions
            auto [text_width, text_height] = calculate_text_dimensions(face, text);

            // Calculate position for centered text
            int text_x = x - text_width / 2;
            int text_y = y + text_height / 2;

            // Ensure the text stays within the image boundaries
            text_x = std::max(0, std::min(static_cast<int>(width_) - text_width, text_x));
            text_y = std::max(text_height, std::min(static_cast<int>(height_), text_y));

            // Render the text
            render_text_to_image(image_buffer, width_, height_, text, options.font_filename,
                                 options.font_size_percentage, options.text_color);
         }
         catch (const std::exception& e) {
            std::cerr << "Error rendering text: " << e.what() << std::endl;
         }
      }

      // Helper to render text right-aligned
      void render_text_right_aligned(uint8_t* image_buffer, const std::string& text, int x, int y,
                                     const scale_options& options)
      {
         try {
            // Register the font if not already done
            ft_context.register_font(options.font_filename);

            // Get the font face for rendering
            FT_Face face = ft_context.get_font(options.font_filename);

            // Set font size
            int font_size = static_cast<int>(height_ * (options.font_size_percentage / 100.0f));
            FT_Set_Pixel_Sizes(face, 0, font_size);

            // Calculate text dimensions
            auto [text_width, text_height] = calculate_text_dimensions(face, text);

            // Calculate position for right-aligned text
            int text_x = x - text_width;
            int text_y = y + text_height / 2;

            // Ensure the text stays within the image boundaries
            text_x = std::max(0, std::min(static_cast<int>(width_) - text_width, text_x));
            text_y = std::max(text_height, std::min(static_cast<int>(height_), text_y));

            // Render the text
            render_text_to_image(image_buffer, width_, height_, text, options.font_filename,
                                 options.font_size_percentage, options.text_color);
         }
         catch (const std::exception& e) {
            std::cerr << "Error rendering text: " << e.what() << std::endl;
         }
      }

      // Helper to render rotated text (simplified version - for full rotation, you need more complex rendering)
      void render_text_rotated(uint8_t* image_buffer, const std::string& text, int x, int y, float angle_degrees,
                               const scale_options& options)
      {
         // Simple vertical text for now (no actual rotation)
         if (std::abs(angle_degrees - 90.0f) < 0.1f) {
            try {
               // Render each character individually, stacked vertically
               ft_context.register_font(options.font_filename);
               FT_Face face = ft_context.get_font(options.font_filename);

               int font_size = static_cast<int>(height_ * (options.font_size_percentage / 100.0f));
               FT_Set_Pixel_Sizes(face, 0, font_size);

               int y_offset = 0;
               for (char c : text) {
                  // Calculate dimensions of this character
                  FT_Load_Char(face, c, FT_LOAD_RENDER);
                  int char_height = face->glyph->bitmap.rows;

                  // Render at the current position
                  std::string char_str(1, c);
                  render_text_to_image(image_buffer, width_, height_, char_str, options.font_filename,
                                       options.font_size_percentage, options.text_color);

                  // Move down for the next character
                  y_offset += char_height + 2; // Add a small gap
               }
            }
            catch (const std::exception& e) {
               std::cerr << "Error rendering rotated text: " << e.what() << std::endl;
            }
         }
         else {
            // Fall back to horizontal text for non-90-degree rotations
            render_text_centered(image_buffer, text, x, y, options);
         }
      }
   };
}
