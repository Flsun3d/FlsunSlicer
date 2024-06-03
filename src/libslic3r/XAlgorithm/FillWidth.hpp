#ifndef _FillWidth_hpp_
#define _FillWidth_hpp_

#include "Polyline.hpp"

#include "ExtrusionEntity.hpp"
#include "ExtrusionEntityCollection.hpp"

#include <boost/geometry.hpp>
#include <boost/geometry/geometries/point_xy.hpp>
#include <boost/geometry/geometries/linestring.hpp>
#include <boost/geometry/geometries/multi_point.hpp>
#include <boost/geometry/geometries/multi_linestring.hpp>
#include <boost/geometry/geometries/point.hpp>
#include <boost/geometry/geometries/box.hpp>
#include <boost/geometry/index/rtree.hpp>

#include <igl/point_mesh_squared_distance.h>
#include <igl/AABB.h>

#include <Eigen/Core>
#include <chrono>   


namespace bg  = boost::geometry;
namespace bgi = boost::geometry::index;

namespace Slic3r {

typedef boost::geometry::model::d2::point_xy<double>    point_type;
typedef boost::geometry::model::polygon<point_type>     polygon_type;
typedef boost::geometry::model::linestring<point_type>  linestring_type;
typedef boost::geometry::model::multi_point<point_type> multi_point_type;

typedef bg::model::point<double, 2, bg::cs::cartesian>  BPoint2d;
typedef bg::model::box<BPoint2d>                        BBox2d;
typedef std::pair<bg::model::box<BPoint2d>, unsigned>   box_index_value;
typedef bgi::rtree<box_index_value, bgi::quadratic<16>> BRTree;

typedef std::pair<Eigen::MatrixXd, Eigen::MatrixXi>     polyline_mesh_type;
typedef std::pair<Eigen::Vector2d, Eigen::Vector2d>     bounding_box_type;

static constexpr double NEAR_THRESHOLD    = scale_(0.6);
static constexpr double DISCTETE_STEP_LEN = scale_(1.0);
static constexpr double INFILL_PERIMETER_OVERLAP_SCALE = 0.05;
static constexpr double INFILL_MIN_WIDTH = 0.1;
static constexpr double INFILL_PERIMETER_ANGLE_EPSILON = 0.2;

} // namespace Slic3r
#endif
