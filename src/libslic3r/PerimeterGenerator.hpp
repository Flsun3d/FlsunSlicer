///|/ Copyright (c) Prusa Research 2016 - 2023 Vojtěch Bubník @bubnikv, Lukáš Hejl @hejllukas
///|/ Copyright (c) Slic3r 2015 - 2016 Alessandro Ranellucci @alranel
///|/ Copyright (c) 2015 Maksim Derbasov @ntfshard
///|/
///|/ PrusaSlicer is released under the terms of the AGPLv3 or higher
///|/
#ifndef slic3r_PerimeterGenerator_hpp_
#define slic3r_PerimeterGenerator_hpp_

#include "libslic3r.h"
#include <vector>
#include "ExtrusionEntityCollection.hpp"
#include "Flow.hpp"
#include "Polygon.hpp"
#include "PrintConfig.hpp"
#include "SurfaceCollection.hpp"
#include <libslic3r/Layer.hpp>

namespace Slic3r {

class PerimeterGenerator{

public:

     // Input parameters
     const SurfaceCollection      *slices;
     const ExPolygons             *upper_slices;
     const ExPolygons             *lower_slices;
     double                       layer_height;
     int                          layer_id;
     Flow                         perimeter_flow;
     Flow                         ext_perimeter_flow;
     Flow                         overhang_flow;
     Flow                         solid_infill_flow;
     const PrintRegionConfig      &config;
     const PrintObjectConfig      &object_config;
     const PrintConfig            &print_config;
    
     // Output:
     // Loops with the external thin walls
     ExtrusionEntityCollection &out_loops;
     // Gaps without the thin walls
     ExtrusionEntityCollection &out_gap_fill;
     // Infills without the gap fills
     ExPolygons &out_fill_expolygons; 
     
    PerimeterGenerator(

          // Input:
          const SurfaceCollection * slices,
          double                      layer_height,
          int                         layer_id,
          Flow                        perimeter_flow,
          Flow                        ext_perimeter_flow,
          Flow                        overhang_flow,
          Flow                        solid_infill_flow,
          const PrintRegionConfig    &config,
          const PrintObjectConfig    &object_config,
          const PrintConfig          &print_config,
          const bool                  spiral_vase,

         // Output:
         // Loops with the external thin walls
         ExtrusionEntityCollection &loops,
         // Gaps without the thin walls
         ExtrusionEntityCollection &gap_fill,
         // Infills without the gap fills
         ExPolygons &fill_surfaces)
         : slices(slices)
         , upper_slices(nullptr)
         , lower_slices(nullptr)
         , layer_height(layer_height)
         , layer_id(-1)

         ,perimeter_flow(perimeter_flow) 

         ,ext_perimeter_flow(ext_perimeter_flow)
         ,overhang_flow(overhang_flow)
         ,solid_infill_flow(solid_infill_flow)
         , config(config)
         , object_config(object_config)
         , print_config(print_config)
         , spiral_vase(spiral_vase)
         , scaled_resolution(scaled<double>(print_config.gcode_resolution.value))
         , mm3_per_mm(perimeter_flow.mm3_per_mm())
         , ext_mm3_per_mm(ext_perimeter_flow.mm3_per_mm())
         , mm3_per_mm_overhang(overhang_flow.mm3_per_mm())
         , out_loops(loops)
         , out_gap_fill(gap_fill)
         , out_fill_expolygons(fill_surfaces)

     {}

     void            process_classic(std::vector<std::pair<ExtrusionRange, ExtrusionRange>> &perimeter_and_gapfill_ranges,

                                      std::vector<ExPolygonRange> &fill_expolygons_ranges);

    void             process_arachne(std::vector<std::pair<ExtrusionRange, ExtrusionRange>> &perimeter_and_gapfill_ranges,

                          std::vector<ExPolygonRange> &fill_expolygons_ranges);


     void                          split_top_surfaces(const ExPolygons &orig_polygons,
                                                      ExPolygons &      top_fills,
                                                      ExPolygons &      non_top_polygons,
                                                      ExPolygons &      fill_clip);
//private:
 public:
    // Derived parameters
    bool                         spiral_vase;
    double                       scaled_resolution;
    double                       ext_mm3_per_mm;
    double                       mm3_per_mm;
    double                       mm3_per_mm_overhang;
    Polygons                     lower_slices_polygons_cache;
    //  struct Parameters {    
//    Parameters(
//        double                      layer_height,
//        int                         layer_id,
//        Flow                        perimeter_flow,
//        Flow                        ext_perimeter_flow,
//        Flow                        overhang_flow,
//        Flow                        solid_infill_flow,
//        const PrintRegionConfig    &config,
//        const PrintObjectConfig    &object_config,
//        const PrintConfig          &print_config,
//        const bool                  spiral_vase) :   
//           
//            upper_slices(nullptr), 
//            lower_slices(nullptr),
//            layer_height(layer_height),
//            layer_id(layer_id),
//            perimeter_flow(perimeter_flow), 
//            ext_perimeter_flow(ext_perimeter_flow),
//            overhang_flow(overhang_flow), 
//            solid_infill_flow(solid_infill_flow),
//            config(config), 
//            object_config(object_config), 
//            print_config(print_config),
//            spiral_vase(spiral_vase),
//            scaled_resolution(scaled<double>(print_config.gcode_resolution.value)),
//            mm3_per_mm(perimeter_flow.mm3_per_mm()),
//            ext_mm3_per_mm(ext_perimeter_flow.mm3_per_mm()), 
//            mm3_per_mm_overhang(overhang_flow.mm3_per_mm())
//        {
//        }
//
//    // Input parameters
//    const ExPolygons *           upper_slices;
//    const ExPolygons*             lower_slices;
//    double                       layer_height;
//    int                          layer_id;
//    Flow                         perimeter_flow;
//    Flow                         ext_perimeter_flow;
//    Flow                         overhang_flow;
//    Flow                         solid_infill_flow;
//    const PrintRegionConfig     &config;
//    const PrintObjectConfig     &object_config;
//    const PrintConfig           &print_config;
//
//    // Derived parameters
//    bool                         spiral_vase;
//    double                       scaled_resolution;
//    double                       ext_mm3_per_mm;
//    double                       mm3_per_mm;
//    double                       mm3_per_mm_overhang;
//
// private:
//    Parameters() = delete;
//};

 //void process_classic(
 //   // Inputs:
 //   const Parameters           &params,
 //   const Surface              &surface,
 //   const ExPolygons           *lower_slices,
 //   const ExPolygons           *upper_slices,
 //   // Cache:
 //   Polygons                   &lower_slices_polygons_cache,
 //   // Output:
 //   // Loops with the external thin walls
 //   ExtrusionEntityCollection  &out_loops,
 //   // Gaps without the thin walls
 //   ExtrusionEntityCollection  &out_gap_fill,
 //   // Infills without the gap fills
 //   ExPolygons                 &out_fill_expolygons);

 //void process_arachne(
 //   // Inputs:
 //   const Parameters           &params,
 //   const Surface              &surface,
 //   const ExPolygons           *lower_slices,
 //    const ExPolygons          *upper_slices,
 //   // Cache:
 //   Polygons                   &lower_slices_polygons_cache,
 //   // Output:
 //   // Loops with the external thin walls
 //   ExtrusionEntityCollection  &out_loops,
 //   // Gaps without the thin walls
 //   ExtrusionEntityCollection  &out_gap_fill,
 //   // Infills without the gap fills
 //   ExPolygons                 &out_fill_expolygons);

 /* void split_top_surfaces(const ExPolygons &orig_polygons, ExPolygons &top_fills, ExPolygons &non_top_polygons, ExPolygons &fill_clip,const Parameters   &params) ;

  ExtrusionMultiPath thick_polyline_to_multi_path(const ThickPolyline &thick_polyline, ExtrusionRole role, const Flow &flow, float tolerance, float merge_tolerance);*/

 }; // namespace PerimeterGenerator

 ExtrusionMultiPath thick_polyline_to_multi_path(const ThickPolyline &thick_polyline, ExtrusionRole role, const Flow &flow, float tolerance, float merge_tolerance);

} // namespace Slic3r

#endif
