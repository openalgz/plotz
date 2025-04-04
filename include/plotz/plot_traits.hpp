// Plotz Library
// For the license information refer to plotz.hpp

#pragma once

#include <array>
#include <string>
#include <type_traits>

#include "plotz/spectrum.hpp"

namespace plotz
{
   // Forward declarations of plot types
   struct heatmap;
   struct magnitude;
   struct magnitude_mapped;
   struct spectrum;

   // Default trait implementation (will cause a compile error if used without specialization)
   template <class PlotType>
   struct plot_traits
   {
      // This will cause a compile error if someone tries to use a plot type without a specialization
      static_assert(std::is_void<PlotType>::value, "No plot_traits specialization exists for this plot type");

      static double get_min_x(const PlotType& plot);
      static double get_max_x(const PlotType& plot);
      static double get_min_y(const PlotType& plot);
      static double get_max_y(const PlotType& plot);
      static std::string get_x_label(const PlotType& plot);
      static std::string get_y_label(const PlotType& plot);
      static std::array<double, 4> get_data_rect(const PlotType& plot); // [min_x, min_y, max_x, max_y]
   };

   // Specialization for spectrum plot
   template <>
   struct plot_traits<spectrum>
   {
      static double get_min_x(const spectrum& plot) { return 0.0; }
      static double get_max_x(const spectrum& plot) { return static_cast<double>(plot.num_bins); }
      static double get_min_y(const spectrum& plot) { return static_cast<double>(plot.min_magnitude); }
      static double get_max_y(const spectrum& plot) { return static_cast<double>(plot.max_magnitude); }

      static std::string get_x_label(const spectrum& plot) { return "Frequency Bin"; }
      static std::string get_y_label(const spectrum& plot) { return "Magnitude"; }

      static std::array<double, 4> get_data_rect(const spectrum& plot)
      {
         return {get_min_x(plot), get_min_y(plot), get_max_x(plot), get_max_y(plot)};
      }
   };

   // Specialization for heatmap plot
   template <>
   struct plot_traits<heatmap>
   {
      static double get_min_x(const heatmap& plot) { return 0.0; }
      static double get_max_x(const heatmap& plot) { return static_cast<double>(plot.width); }
      static double get_min_y(const heatmap& plot) { return 0.0; }
      static double get_max_y(const heatmap& plot) { return static_cast<double>(plot.height); }

      static std::string get_x_label(const heatmap& plot) { return "X"; }
      static std::string get_y_label(const heatmap& plot) { return "Y"; }

      static std::array<double, 4> get_data_rect(const heatmap& plot)
      {
         return {get_min_x(plot), get_min_y(plot), get_max_x(plot), get_max_y(plot)};
      }
   };

   // Specialization for magnitude plot
   template <>
   struct plot_traits<magnitude>
   {
      static double get_min_x(const magnitude& plot) { return 0.0; }
      static double get_max_x(const magnitude& plot) { return static_cast<double>(plot.width); }
      static double get_min_y(const magnitude& plot) { return 0.0; }
      static double get_max_y(const magnitude& plot) { return static_cast<double>(plot.height); }

      static std::string get_x_label(const magnitude& plot) { return "X"; }
      static std::string get_y_label(const magnitude& plot) { return "Y"; }

      static std::array<double, 4> get_data_rect(const magnitude& plot)
      {
         return {get_min_x(plot), get_min_y(plot), get_max_x(plot), get_max_y(plot)};
      }
   };

   // Specialization for magnitude_mapped plot
   template <>
   struct plot_traits<magnitude_mapped>
   {
      static double get_min_x(const magnitude_mapped& plot) { return 0.0; }
      static double get_max_x(const magnitude_mapped& plot) { return static_cast<double>(plot.input_width); }
      static double get_min_y(const magnitude_mapped& plot) { return 0.0; }
      static double get_max_y(const magnitude_mapped& plot) { return static_cast<double>(plot.input_height); }

      static std::string get_x_label(const magnitude_mapped& plot) { return "X"; }
      static std::string get_y_label(const magnitude_mapped& plot) { return "Y"; }

      static std::array<double, 4> get_data_rect(const magnitude_mapped& plot)
      {
         return {get_min_x(plot), get_min_y(plot), get_max_x(plot), get_max_y(plot)};
      }
   };
}
