// Plotz Library
// For the license information refer to plotz.hpp

#include <cerrno>
#include <complex>
#include <cstring>
#include <format>
#include <iostream>
#include <random>
#include <vector>

#include "plotz/plotz.hpp"

// Function to render text using FreeType
inline void render_text_to_image(uint8_t* image, size_t img_width, size_t img_height, const std::string& text,
                                 const std::string& font_filename, int font_size, int x_pos, int y_pos)
{
   FT_Library ft;
   if (FT_Init_FreeType(&ft)) {
      std::cerr << "Could not init FreeType Library\n";
      return;
   }

   FT_Face face;
   if (FT_New_Face(ft, font_filename.c_str(), 0, &face)) {
      std::cerr << "Failed to load font: " << font_filename << "\n";
      FT_Done_FreeType(ft);
      return;
   }

   FT_Set_Pixel_Sizes(face, 0, font_size);

   // Starting position
   int pen_x = x_pos;
   int pen_y = y_pos;

   for (char c : text) {
      // Load character glyph
      if (FT_Load_Char(face, c, FT_LOAD_RENDER)) {
         std::cerr << "Failed to load Glyph\n";
         continue;
      }

      FT_GlyphSlot g = face->glyph;

      // For each pixel in the glyph bitmap
      for (unsigned int row = 0; row < g->bitmap.rows; ++row) {
         for (unsigned int col = 0; col < g->bitmap.width; ++col) {
            int x = pen_x + g->bitmap_left + col;
            int y = pen_y - g->bitmap_top + row;

            if (x < 0 || x >= img_width || y < 0 || y >= img_height) continue;

            // Calculate the pixel index in the image buffer
            size_t pixel_index = (y * img_width + x) * 4;

            // Get the value from the glyph bitmap
            unsigned char glyph_value = g->bitmap.buffer[row * g->bitmap.width + col];

            // Simple blending: assume white text, blend based on glyph_value
            // Modify as needed for different colors or blending modes
            unsigned char alpha = glyph_value;
            unsigned char inv_alpha = 255 - alpha;

            // Existing image pixel (RGBA)
            unsigned char* pixel = &image[pixel_index];

            // Set the pixel to white with the glyph's alpha
            pixel[0] = (pixel[0] * inv_alpha + 255 * alpha) / 255; // R
            pixel[1] = (pixel[1] * inv_alpha + 255 * alpha) / 255; // G
            pixel[2] = (pixel[2] * inv_alpha + 255 * alpha) / 255; // B
            pixel[3] = std::min(255, pixel[3] + alpha); // A
         }
      }

      // Advance to the next character position
      pen_x += g->advance.x >> 6;
      pen_y += g->advance.y >> 6;
   }

   FT_Done_Face(face);
   FT_Done_FreeType(ft);
}

// Function to draw the legend
inline void draw_legend(uint8_t* image, size_t img_width, size_t img_height, size_t heatmap_width,
                        const std::vector<uint8_t>& colors, const std::string& font_filename, int font_size)
{
   // Legend dimensions and margins
   size_t legend_width = img_width - heatmap_width;
   size_t legend_margin_top = 20;
   size_t legend_margin_bottom = 20;
   size_t legend_margin_left = 10;
   size_t legend_margin_right = 10;

   // Fill legend area with white background
   for (size_t y = 0; y < img_height; ++y) {
      for (size_t x = heatmap_width; x < img_width; ++x) {
         unsigned char* pixel = &image[(y * img_width + x) * 4];
         pixel[0] = 255; // R
         pixel[1] = 255; // G
         pixel[2] = 255; // B
         pixel[3] = 255; // A
      }
   }

   // Calculate the area where the legend color bar will be drawn
   size_t legend_bar_left = heatmap_width + legend_margin_left;
   size_t legend_bar_right = img_width - legend_margin_right;
   size_t legend_bar_width = legend_bar_right - legend_bar_left;
   size_t legend_bar_top = legend_margin_top;
   size_t legend_bar_bottom = img_height - legend_margin_bottom;
   size_t legend_bar_height = legend_bar_bottom - legend_bar_top;

   size_t ncolors = plotz::get_color_count(colors);

   // Draw the color bar
   for (size_t y = 0; y < legend_bar_height; ++y) {
      float value = static_cast<float>(legend_bar_height - y - 1) / static_cast<float>(legend_bar_height - 1);

      size_t color_idx = static_cast<size_t>(value * (ncolors - 1) + 0.5f);
      color_idx = std::min(color_idx, ncolors - 1);
      const unsigned char* color = &colors[color_idx * 4];

      for (size_t x = legend_bar_left; x < legend_bar_right; ++x) {
         size_t pixel_x = x;
         size_t pixel_y = legend_bar_top + y;
         unsigned char* pixel = &image[(pixel_y * img_width + pixel_x) * 4];
         pixel[0] = color[0]; // R
         pixel[1] = color[1]; // G
         pixel[2] = color[2]; // B
         pixel[3] = color[3]; // A
      }
   }

   // Add labels to the legend
   std::string max_label = "Max";
   std::string min_label = "Min";

   // Positions for labels
   int x_label = static_cast<int>(heatmap_width + (legend_width / 2) - (font_size * 1.5));
   int y_max_label = static_cast<int>(legend_bar_top - 5);
   int y_min_label = static_cast<int>(legend_bar_bottom + font_size + 5);

   render_text_to_image(image, img_width, img_height, max_label, font_filename, font_size, x_label, y_max_label);
   render_text_to_image(image, img_width, img_height, min_label, font_filename, font_size, x_label, y_min_label);
}

/*
 size_t legend_width = 124; // Width of the legend area
 size_t new_w = w + legend_width; // New image width including legend

 std::vector<uint8_t> new_image(new_w * h * 4, 0);

 // Copy data to the new image buffer
 for (size_t y = 0; y < h; ++y) {
    const auto* src_row = &image[y * w * 4];
    auto* dest_row = &new_image[y * new_w * 4];
    std::memcpy(dest_row, src_row, w * 4);
 }

 int font_size = 14;
 draw_legend(new_image.data(), new_w, h, w, plot::default_color_scheme_data, font_filename, font_size);
 */

std::vector<std::complex<float>> generateSpiral(int width, int height, int numPoints, float turns)
{
   std::vector<std::complex<float>> data;
   data.reserve(numPoints);

   float centerX = width / 2.0f;
   float centerY = height / 2.0f;
   float maxRadius = std::min(width, height) / 2.0f;

   for (int i = 0; i < numPoints; ++i) {
      float t = static_cast<float>(i) / numPoints;
      float angle = turns * 2.0f * M_PI * t;
      float radius = maxRadius * t;

      float x = centerX + radius * std::cos(angle);
      float y = centerY + radius * std::sin(angle);

      data.emplace_back(x, y);
   }

   return data;
}

std::vector<std::complex<float>> generateMandelbrot(int width, int height, int maxIterations)
{
   std::vector<std::complex<float>> data;

   float xMin = 0.0f;
   float xMax = static_cast<float>(width);
   float yMin = 0.0f;
   float yMax = static_cast<float>(height);

   for (int px = 0; px < width; ++px) {
      for (int py = 0; py < height; ++py) {
         // Map pixel to complex plane
         float x0 = static_cast<float>(px) / width * 3.5f - 2.5f; // Typically Mandelbrot is plotted from -2.5 to 1
         float y0 = static_cast<float>(py) / height * 2.0f - 1.0f; // Typically Mandelbrot is plotted from -1 to 1

         std::complex<float> c(x0, y0);
         std::complex<float> z(0, 0);
         int iterations = 0;

         while (std::abs(z) <= 2.0f && iterations < maxIterations) {
            z = z * z + c;
            ++iterations;
         }

         if (iterations == maxIterations) {
            // Point is in the Mandelbrot set
            // Scale back to image coordinates
            float real = static_cast<float>(px);
            float imag = static_cast<float>(py);
            data.emplace_back(real, imag);
         }
      }
   }

   return data;
}

// Function to generate Lissajous curve
std::vector<std::complex<float>> generateLissajous(int width, int height, int numPoints, float a, float b, float delta)
{
   std::vector<std::complex<float>> data;
   data.reserve(numPoints);

   float centerX = width / 2.0f;
   float centerY = height / 2.0f;
   float amplitudeX = width / 2.0f * 0.9f; // 90% to add some padding
   float amplitudeY = height / 2.0f * 0.9f;

   for (int i = 0; i < numPoints; ++i) {
      float t = static_cast<float>(i) / numPoints * 2.0f * M_PI;
      float x = centerX + amplitudeX * std::sin(a * t + delta);
      float y = centerY + amplitudeY * std::sin(b * t);
      data.emplace_back(x, y);
   }

   return data;
}

void heatmap_test()
{
   static constexpr size_t w = 1024, h = 1024, npoints = 1000;

   // Create the heatmap object with the given dimensions (in pixels).
   plotz::heatmap hm(w, h);

   // Create two normal distributions for generating random points.
   /*std::random_device rd;
   std::mt19937 prng(rd());
   std::normal_distribution<float> x_distr(0.5f * w, (0.5f / 3.0f) * w);
   std::normal_distribution<float> y_distr(0.5f * h, 0.25f * h);

  // Add random points to the heatmap.
  for (unsigned i = 0; i < npoints; ++i) {
      unsigned x = static_cast<unsigned>(x_distr(prng));
      unsigned y = static_cast<unsigned>(y_distr(prng));
      if (x < w && y < h) {
          hm.add_point(x, y);
      }
  }*/

   auto data = generateSpiral(w, h, npoints, 10);
   // auto data = generateMandelbrot(w, h, 50);

   float a = 3.0f; // Frequency for x
   float b = 2.0f; // Frequency for y
   float delta = M_PI / 2; // Phase shift

   // auto data = generateLissajous(w, h, npoints, a, b, delta);

   for (size_t i = 0; i < data.size(); ++i) {
      hm.add_point(data[i].real(), data[i].imag());
   }

   // Render the heatmap to an image buffer.
   // The image vector contains the image data in 32-bit RGBA format.
   std::vector<uint8_t> image = hm.render();

   // Save the image using libpng.
   return plotz::write_png("heatmap.png", image.data(), w, h);
}

void magnitude_test()
{
   static constexpr size_t w = 4096, h = 4096;

   plotz::magnitude plot(w, h);

   for (uint32_t y = 0; y < h; ++y) {
      for (uint32_t x = 0; x < w; ++x) {
         float magnitude = static_cast<float>(x + y) / (w + h);
         plot.add_point(x, y, magnitude);
      }
   }

   std::vector<uint8_t> image = plot.render();

   std::string text = "Sample Magnitude Plot";
   std::string font_filename = FONTS_DIR "/RobotoMono-SemiBold.ttf";
   float font_percent = 3.f;

   plotz::render_text_to_image(image.data(), w, h, text, font_filename, font_percent);

   return plotz::write_png("magnitude.png", image.data(), w, h);
}

void magnitude_test2()
{
   // Define plot dimensions
   uint32_t width = 1024;
   uint32_t height = 1024;

   // Create a magnitude plot instance
   plotz::magnitude plot(width, height);

   // Define center coordinates
   float center_x = static_cast<float>(width) / 2.0f;
   float center_y = static_cast<float>(height) / 2.0f;

   // Define frequency for the sine wave to create ring patterns
   // Adjust the frequency to change the number of rings
   float frequency = 20.0f; // Higher values = more rings

   // Populate the magnitude buffer with a ripple pattern
   for (uint32_t y = 0; y < height; ++y) {
      for (uint32_t x = 0; x < width; ++x) {
         float dx = static_cast<float>(x) - center_x;
         float dy = static_cast<float>(y) - center_y;
         float distance = std::sqrt(dx * dx + dy * dy);

         // Normalize distance based on the maximum possible distance in the image
         float max_distance = std::sqrt(center_x * center_x + center_y * center_y);
         float normalized_distance = distance / max_distance;

         // Compute magnitude using a sine function to create concentric rings
         // The sine function oscillates between -1 and 1; we scale it to 0 to 1
         float magnitude = (std::sin(normalized_distance * frequency) + 1.0f) / 2.0f;

         plot.add_point(x, y, magnitude);
      }
   }

   std::vector<uint8_t> image = plot.render();

   return plotz::write_png("magnitude2.png", image.data(), width, height);
}

void magnitude_mapped_test()
{
   static constexpr size_t w_data = 100, h_data = 100;
   static constexpr size_t w = 2048, h = 4096;

   plotz::magnitude_mapped plot(w_data, h_data, w, h);

   for (uint32_t y = 0; y < h_data; ++y) {
      for (uint32_t x = 0; x < w_data; ++x) {
         float magnitude = float(x + y) / (w_data + h_data);
         plot.add_point(x, y, magnitude);
      }
   }

   std::vector<uint8_t> image = plot.render();

   std::string text = "Sample Mapped Magnitude Plot";
   std::string font_filename = FONTS_DIR "/RobotoMono-SemiBold.ttf";
   float font_percent = 2.f;

   plotz::render_text_to_image(image.data(), w, h, text, font_filename, font_percent);

   return plotz::write_png("magnitude_mapped.png", image.data(), w, h);
}

void magnitude_mapped_shrink_test()
{
   static constexpr size_t w_data = 1024, h_data = 1024;
   static constexpr size_t w = 512, h = 512;

   plotz::magnitude_mapped plot(w_data, h_data, w, h);

   for (uint32_t y = 0; y < h_data; ++y) {
      for (uint32_t x = 0; x < w_data; ++x) {
         float magnitude = float(x + y) / (w_data + h_data);
         plot.add_point(x, y, magnitude);
      }
   }

   std::vector<uint8_t> image = plot.render();

   std::string text = "Sample Mapped Magnitude Plot";
   std::string font_filename = FONTS_DIR "/RobotoMono-SemiBold.ttf";
   float font_percent = 2.f;

   plotz::render_text_to_image(image.data(), w, h, text, font_filename, font_percent);

   return plotz::write_png("magnitude_mapped_shrink.png", image.data(), w, h);
}

int main()
{
   heatmap_test();
   magnitude_test();
   magnitude_test2();
   magnitude_mapped_test();
   magnitude_mapped_shrink_test();
}
