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
            pixel[3] = (std::min)(255, pixel[3] + alpha); // A
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
      color_idx = (std::min)(color_idx, ncolors - 1);
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
   float maxRadius = (std::min)(width, height) / 2.0f;

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

// Function to plot spiral points onto a magnitude_mapped
void plotSpiral(plotz::magnitude_mapped& plot, const std::vector<std::complex<float>>& spiralPoints, float intensity = 1.0f) {
   // Initialize plot with zeros
   plot.reset();
   
   // For each point in the spiral, add it to the plot
   for (const auto& point : spiralPoints) {
      int x = static_cast<int>(std::real(point));
      int y = static_cast<int>(std::imag(point));
      
      // Make sure the point is within bounds
      if (x >= 0 && x < static_cast<int>(plot.input_width) &&
          y >= 0 && y < static_cast<int>(plot.input_height)) {
         
         plot.add_point(x, y, intensity);
      }
   }
}

int magnitude_grid_plot() {
   const uint32_t input_size = 200;     // Size of input data
   const uint32_t plot_size = 150;      // Size of each plot in the output image
   
   // Create an 8x8 grid
   plotz::magnitude_mapped_grid grid(8, input_size, input_size, plot_size, plot_size);
   
   // Generate different spiral patterns for each plot in the grid
   for (size_t row = 0; row < 8; ++row) {
      for (size_t col = 0; col < 8; ++col) {
         auto& plot = grid.get_plot(row, col);
         
         // Vary parameters based on position in grid
         size_t numPoints = 200 + (row * 100) + (col * 50);  // Points increase with row/col
         float turns = 2.0f + (row * 0.5f) + (col * 0.25f); // Turns increase with row/col
         
         // Generate the spiral
         auto spiralPoints = generateSpiral(input_size, input_size, int(numPoints), turns);
         
         // Plot the spiral
         float intensity = 1.0f;
         plotSpiral(plot, spiralPoints, intensity);
      }
   }
   
   // Create a colorful gradient scheme
   std::vector<uint8_t> spiral_color_scheme = {
      // Define a vibrant color gradient
      20, 0, 100, 255,    // Deep blue
      50, 0, 200, 255,    // Royal blue
      0, 100, 255, 255,   // Azure
      0, 200, 200, 255,   // Cyan
      0, 255, 100, 255,   // Teal
      100, 255, 0, 255,   // Green
      200, 255, 0, 255,   // Chartreuse
      255, 200, 0, 255,   // Yellow
      255, 100, 0, 255,   // Orange
      255, 0, 100, 255,   // Red
      200, 0, 200, 255,   // Magenta
   };
   
   // Render and save the grid as a PNG
   grid.write_png("spiral_grid.png", spiral_color_scheme);
}

void spectrum_test_sine()
{
   static constexpr uint32_t bins = 256; // Number of frequency bins
   static constexpr uint32_t w = 800; // Width of the visualization
   static constexpr uint32_t h = 300; // Height of the visualization

   // Create a spectrum plot instance with separate bin count and display width
   plotz::spectrum plot(bins, w, h);

   // Set the style
   plot.style = plotz::spectrum::bar_style::gradient;
   plot.show_peaks = true;
   plot.bar_width_factor = 0.8f; // Make bars slightly narrower than bin width

   // Create a simple spectrum with a single frequency spike
   std::vector<float> magnitudes(bins, 0.0f);

   // Add a peak at bin 64 (simulating a sine wave)
   uint32_t peak_bin = 64;
   magnitudes[peak_bin] = 1.0f;

   // Add some smaller values around the peak to simulate spectral leakage
   for (int i = 1; i <= 10; ++i) {
      if (peak_bin - i >= 0) magnitudes[peak_bin - i] = 1.0f / (i * i);

      if (peak_bin + i < bins) magnitudes[peak_bin + i] = 1.0f / (i * i);
   }

   // Update the spectrum with the magnitudes
   plot.update(magnitudes);

   // Render the spectrum to an image buffer
   std::vector<uint8_t> image = plot.render();

   // Add text label to the image
   std::string text = "Single Frequency Spectrum";
   std::string font_filename = FONTS_DIR "/RobotoMono-SemiBold.ttf";
   float font_percent = 3.0f;

   plotz::render_text_to_image(image.data(), w, h, text, font_filename, font_percent);

   // Save the image using libpng
   return plotz::write_png("spectrum_sine.png", image.data(), w, h);
}

void spectrum_test_complex()
{
   static constexpr uint32_t bins = 256; // Number of frequency bins
   static constexpr uint32_t w = 1024; // Width of the visualization
   static constexpr uint32_t h = 256; // Height of the visualization

   // Create a spectrum plot instance with separate bin count and display width
   plotz::spectrum plot(bins, w, h);

   // Set the style
   plot.style = plotz::spectrum::bar_style::solid;
   plot.show_peaks = true;
   plot.bar_width_factor = 0.9f; // Slightly narrower bars

   // Create a more complex spectrum with multiple frequencies
   std::vector<float> magnitudes(bins, 0.0f);

   // Add several frequency peaks
   uint32_t peaks[] = {32, 64, 96, 128, 192};
   float amplitudes[] = {0.5f, 1.0f, 0.7f, 0.3f, 0.8f};

   for (int p = 0; p < 5; ++p) {
      uint32_t peak_bin = peaks[p];
      float amplitude = amplitudes[p];

      magnitudes[peak_bin] = amplitude;

      // Add some smaller values around the peak
      for (int i = 1; i <= 5; ++i) {
         if (peak_bin - i >= 0) magnitudes[peak_bin - i] = amplitude / (i * i);

         if (peak_bin + i < bins) magnitudes[peak_bin + i] = amplitude / (i * i);
      }
   }

   // Update the spectrum with the magnitudes
   plot.update(magnitudes);

   // Render the spectrum to an image buffer using a different color scheme
   std::vector<uint8_t> image = plot.render(plotz::make_color_scheme(plotz::inferno_key_colors));

   // Add text label to the image
   std::string text = "Multi-Frequency Spectrum";
   std::string font_filename = FONTS_DIR "/RobotoMono-SemiBold.ttf";
   float font_percent = 3.0f;

   plotz::render_text_to_image(image.data(), w, h, text, font_filename, font_percent);

   // Save the image using libpng
   return plotz::write_png("spectrum_complex.png", image.data(), w, h);
}

void spectrum_test_audio()
{
   static constexpr uint32_t bins = 128; // Number of frequency bins (typical for audio)
   static constexpr uint32_t w = 1280; // Width of the visualization (10 pixels per bin)
   static constexpr uint32_t h = 320; // Height of the visualization

   // Create a spectrum plot instance with separate bin count and display width
   plotz::spectrum plot(bins, w, h);

   // Set the style to the LED-style segmented display
   plot.style = plotz::spectrum::bar_style::segmented;
   plot.show_peaks = true;
   plot.bar_width_factor = 0.7f; // Narrower bars with more space between them

   // Create a realistic audio spectrum shape
   std::vector<float> magnitudes(bins);

   // Generate an audio-like spectrum (pink noise-like distribution)
   for (uint32_t i = 0; i < bins; ++i) {
      // Create a curve that falls off at higher frequencies (pink noise)
      float x = static_cast<float>(i) / bins;

      // Base curve (1/f distribution)
      float pink_noise = 1.0f / (1.0f + 10.0f * x);

      // Add some "music-like" resonances
      float resonances = 0.0f;
      resonances += 0.8f * std::exp(-20.0f * std::pow(x - 0.1f, 2.0f)); // Bass peak
      resonances += 0.6f * std::exp(-20.0f * std::pow(x - 0.3f, 2.0f)); // Mid peak
      resonances += 0.4f * std::exp(-30.0f * std::pow(x - 0.7f, 2.0f)); // High peak

      // Combine the pink noise and resonances
      magnitudes[i] = 0.5f * pink_noise + 0.5f * resonances;
   }

   // Update the spectrum
   plot.update(magnitudes);

   // Render the spectrum using an alternate color scheme
   std::vector<uint8_t> image = plot.render(plotz::make_color_scheme(plotz::temperature_key_colors));

   // Add text label to the image
   std::string text = "Audio Spectrum Analyzer";
   std::string font_filename = FONTS_DIR "/RobotoMono-SemiBold.ttf";
   float font_percent = 3.0f;

   plotz::render_text_to_image(image.data(), w, h, text, font_filename, font_percent);

   // Save the image
   return plotz::write_png("spectrum_audio.png", image.data(), w, h);
}

void spectrum_test_high_resolution()
{
   // Test with equal bins and width
   static constexpr uint32_t bins = 1024; // 1K bins
   static constexpr uint32_t w = 1024; // Output image width (equal to bins)
   static constexpr uint32_t h = 512; // Output image height

   // Create a spectrum plot instance
   plotz::spectrum plot(bins, w, h);

   // Set the style
   plot.style = plotz::spectrum::bar_style::gradient;
   plot.show_peaks = true;

   // Create spectrum data with multiple frequency peaks
   std::vector<float> magnitudes(bins, 0.0f);

   // Generate a complex spectrum pattern
   for (uint32_t i = 0; i < bins; ++i) {
      float normalized_freq = static_cast<float>(i) / bins;

      // Create a pattern with multiple frequency components
      float val = 0.0f;

      // Main peaks
      val += 1.0f * std::exp(-200.0f * std::pow(normalized_freq - 0.1f, 2.0f));
      val += 0.8f * std::exp(-200.0f * std::pow(normalized_freq - 0.3f, 2.0f));
      val += 0.6f * std::exp(-200.0f * std::pow(normalized_freq - 0.5f, 2.0f));
      val += 0.4f * std::exp(-200.0f * std::pow(normalized_freq - 0.7f, 2.0f));
      val += 0.2f * std::exp(-200.0f * std::pow(normalized_freq - 0.9f, 2.0f));

      // Add some fine details
      val += 0.05f * std::sin(normalized_freq * 100.0f);

      magnitudes[i] = val;
   }

   // Update the spectrum
   plot.update(magnitudes);

   // Render the spectrum
   std::vector<uint8_t> image = plot.render(plotz::make_color_scheme(plotz::jet_key_colors));

   // Add text label to the image
   std::string text = std::format("High Resolution Spectrum: {} bins, {}x{} pixels", bins, w, h);
   std::string font_filename = FONTS_DIR "/RobotoMono-SemiBold.ttf";
   float font_percent = 2.5f;

   plotz::render_text_to_image(image.data(), w, h, text, font_filename, font_percent);

   // Save the image
   return plotz::write_png("spectrum_high_resolution.png", image.data(), w, h);
}

void spectrum_test_more_bins_than_pixels()
{
   // Test with more bins than pixels (common for FFT visualization)
   static constexpr uint32_t bins = 8192; // 8K bins (much higher than pixel width)
   static constexpr uint32_t w = 1024; // Output image width
   static constexpr uint32_t h = 512; // Output image height

   // Create a spectrum plot instance
   plotz::spectrum plot(bins, w, h);

   // Set the style
   plot.style = plotz::spectrum::bar_style::gradient;
   plot.show_peaks = true;

   // Create spectrum data
   std::vector<float> magnitudes(bins, 0.0f);

   // Generate a complex spectrum pattern with fine detail
   for (uint32_t i = 0; i < bins; ++i) {
      float normalized_freq = static_cast<float>(i) / bins;

      // Create a pattern with multiple frequency components
      float val = 0.0f;

      // Main envelope (overall shape of the spectrum)
      val += 0.5f * (1.0f - normalized_freq) * (1.0f - normalized_freq); // Higher at low frequencies

      // Add peaks at various frequencies
      for (int p = 1; p <= 20; ++p) {
         float peak_freq = p / 21.0f;
         float peak_width = 0.002f;
         val += (1.0f / p) * std::exp(-std::pow((normalized_freq - peak_freq) / peak_width, 2.0f));
      }

      // Add some fine harmonic structure (will be aggregated in the final visualization)
      for (int h = 1; h <= 50; ++h) {
         float harmonic_freq = h / 50.0f;
         if (std::fabs(normalized_freq - harmonic_freq) < 0.001f) {
            val += 0.2f / h;
         }
      }

      magnitudes[i] = val;
   }

   // Update the spectrum
   plot.update(magnitudes);

   // Render the spectrum
   std::vector<uint8_t> image = plot.render(plotz::make_color_scheme(plotz::viridis_key_colors));

   // Add text label to the image
   std::string text = std::format("Detailed Spectrum: {} bins rendered to {} pixels", bins, w);
   std::string font_filename = FONTS_DIR "/RobotoMono-SemiBold.ttf";
   float font_percent = 2.5f;

   plotz::render_text_to_image(image.data(), w, h, text, font_filename, font_percent);

   // Save the image
   return plotz::write_png("spectrum_more_bins_than_pixels.png", image.data(), w, h);
}

void spectrum_test_backgrounds()
{
   static constexpr uint32_t bins = 256; // Number of frequency bins
   static constexpr uint32_t w = 1024; // Width of the visualization
   static constexpr uint32_t h = 256; // Height of the visualization

   // Create a complex spectrum with multiple frequencies
   std::vector<float> magnitudes(bins, 0.0f);

   // Generate a pattern with multiple harmonics
   for (uint32_t i = 0; i < bins; ++i) {
      float normalized_freq = static_cast<float>(i) / bins;

      // Create a pattern with multiple frequency components
      float val = 0.0f;

      // Main peaks at different harmonics
      for (int harmonic = 1; harmonic <= 8; ++harmonic) {
         float peak_freq = 0.1f * harmonic;
         if (peak_freq >= 1.0f) break; // Stay within Nyquist frequency

         // Each harmonic is smaller than the previous one
         float amplitude = 1.0f / harmonic;
         val += amplitude * std::exp(-200.0f * std::pow(normalized_freq - peak_freq, 2.0f));
      }

      magnitudes[i] = val;
   }

   // Create and render with three different background colors

   // 1. Default transparent background
   {
      plotz::spectrum plot(bins, w, h);
      plot.style = plotz::spectrum::bar_style::gradient;
      plot.show_peaks = true;
      // Default is transparent - no need to set

      plot.update(magnitudes);
      std::vector<uint8_t> image = plot.render(plotz::make_color_scheme(plotz::viridis_key_colors));

      std::string text = "Spectrum with Transparent Background";
      std::string font_filename = FONTS_DIR "/RobotoMono-SemiBold.ttf";
      float font_percent = 2.5f;

      plotz::render_text_to_image(image.data(), w, h, text, font_filename, font_percent);
      plotz::write_png("spectrum_transparent_bg.png", image.data(), w, h);
   }

   // 2. Black background
   {
      plotz::spectrum plot(bins, w, h);
      plot.style = plotz::spectrum::bar_style::gradient;
      plot.show_peaks = true;
      plot.set_background_color(plotz::black);

      plot.update(magnitudes);
      std::vector<uint8_t> image = plot.render(plotz::make_color_scheme(plotz::viridis_key_colors));

      std::string text = "Spectrum with Black Background";
      std::string font_filename = FONTS_DIR "/RobotoMono-SemiBold.ttf";
      float font_percent = 2.5f;

      plotz::render_text_to_image(image.data(), w, h, text, font_filename, font_percent);
      plotz::write_png("spectrum_black_bg.png", image.data(), w, h);
   }

   // 3. Custom dark blue background
   {
      plotz::spectrum plot(bins, w, h);
      plot.style = plotz::spectrum::bar_style::gradient;
      plot.show_peaks = true;
      plot.set_background_color(10, 20, 40, 255); // Dark blue background

      plot.update(magnitudes);
      std::vector<uint8_t> image = plot.render(plotz::make_color_scheme(plotz::temperature_key_colors));

      std::string text = "Spectrum with Dark Blue Background";
      std::string font_filename = FONTS_DIR "/RobotoMono-SemiBold.ttf";
      float font_percent = 2.5f;

      plotz::render_text_to_image(image.data(), w, h, text, font_filename, font_percent);
      plotz::write_png("spectrum_dark_blue_bg.png", image.data(), w, h);
   }
}

// Update the main function to call the new test functions
int main()
{
   heatmap_test();
   magnitude_test();
   magnitude_test2();
   magnitude_mapped_test();
   magnitude_mapped_shrink_test();
   magnitude_grid_plot();

   // Add our spectrum tests
   spectrum_test_sine();
   spectrum_test_complex();
   spectrum_test_audio();
   spectrum_test_high_resolution();
   spectrum_test_more_bins_than_pixels();
   spectrum_test_backgrounds();
}
