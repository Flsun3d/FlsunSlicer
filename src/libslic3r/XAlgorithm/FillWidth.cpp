#include "FillWidth.hpp"


double _discretize_infill_time = 0;
int _call_discretize_times = 0;


namespace Slic3r {


void get_time_duration() {
    _call_discretize_times += 1;
    auto               start = std::chrono::high_resolution_clock::now();

    auto end        = std::chrono::high_resolution_clock::now();
    auto duration   = std::chrono::duration_cast<std::chrono::microseconds>(end - start);
    auto _cost_time = double(duration.count());
    _discretize_infill_time += _cost_time;
    if (_call_discretize_times % 10000 == 0) {
        std::cout << "discretize infill time:" << _cost_time << "microseconds" << std::endl;
        std::cout << "discretize infill sum time:" << _discretize_infill_time << "microseconds" << std::endl;
    }

    return;
}


// get discrete points for line.
std::vector<Point> get_line_insert_points(const Point &_start_pt, const Point &_end_pt)
{
    std::vector<Point> _insert_pts;

    Vec2f _diff_vt(_end_pt.x() - _start_pt.x(), _end_pt.y() - _start_pt.y());
    _diff_vt *= SCALING_FACTOR;

    double _line_len = _diff_vt.norm() / SCALING_FACTOR;
    Vec2f  _unit_vt   = _diff_vt.normalized();

    if (_line_len < DISCTETE_STEP_LEN) {
        return _insert_pts;
    } 
    else {
        int    _pt_count = int(_line_len / DISCTETE_STEP_LEN);
        double _step_len = _line_len / (_pt_count + 1);
        Vec2f _start_pt_float(_start_pt.x(), _start_pt.y());

        for (size_t _i = 1; _i <= _pt_count; _i++) {
            Vec2f _pt = _start_pt_float + (_i * _step_len) * _unit_vt;
            _insert_pts.push_back(Point(_pt.x(), _pt.y()));
        }

    }

    return _insert_pts;
}


// get discrete points for polyline.
std::vector<Point> get_polyline_discrete_points(const Polyline &_polyline, std::map<int, int>& _index_mapping, const double _step_len)
{
    // init _discrete_pts.
    std::vector<Point> _discrete_pts;

    auto _polyline_pts = _polyline.points;
    auto _polyline_pt_count = _polyline_pts.size();

    for (auto _i = 0; _i < _polyline_pts.size(); _i++) {
        // get point by index.
        auto _pt = _polyline_pts[_i];

        if (_i == _polyline_pts.size() - 1) {
            // reach the end of polyline.
            _index_mapping[_discrete_pts.size()] = _i; 
            _discrete_pts.push_back(_pt);
            break;
        }
        
        // add _pt to _discrete_pts.
        _index_mapping[_discrete_pts.size()] = _i;
        _discrete_pts.push_back(_pt);

        // get next point.
        auto _next_pt = _polyline_pts[(_i + 1) % _polyline_pt_count];

        // get discrete points in the inner side of line.
        auto _line_insert_pts = get_line_insert_points(_pt, _next_pt);

        // add line discrete points.
        for (auto _line_pt : _line_insert_pts) {
            _index_mapping[_discrete_pts.size()] = _i;
            _discrete_pts.push_back(_line_pt);
        }

    }

    return _discrete_pts;
}


// get bounding_box for polyline.
bounding_box_type get_polyline_bounding_box(const Polyline& _polyline) {
    // init _min_pt and _max_pt.
    auto _min_pt = Eigen::Vector2d(1e10, 1e10);
    auto _max_pt = Eigen::Vector2d(-1e10, -1e10);

    for (auto _pt: _polyline.points) {
        auto _pt_x = _pt.x();
        auto _pt_y = _pt.y();

        if (_pt_x < _min_pt(0)) {
            _min_pt(0) = _pt_x;
        }

        if (_pt_y < _min_pt(1)) {
            _min_pt(1) = _pt_y;
        }

        if (_pt_x > _max_pt(0)) {
            _max_pt(0) = _pt_x;
        }
        
        if (_pt_y > _max_pt(1)) {
            _max_pt(1) = _pt_y;
        }
    }

    // construct _bounding_box.
    bounding_box_type _bounding_box = std::make_pair(_min_pt, _max_pt);

    return _bounding_box;
}


// inflate bounding box with given buffer.
void inflate_bounding_box(bounding_box_type &_bounding_box, const double _buffer)
{
    _bounding_box.first(0) -= _buffer;
    _bounding_box.first(1) -= _buffer;
    _bounding_box.second(0) += _buffer;
    _bounding_box.second(1) += _buffer;

    return;
}


// get bounding boxes from 
std::vector<bounding_box_type> get_bounding_boxes_from_extrusion_paths(const ExtrusionPaths &_paths) {
    std::vector<bounding_box_type> _bounding_boxes;

    for (auto _path: _paths) {
        // get bounding box.
        auto _bounding_box = get_polyline_bounding_box(_path.polyline);

        // inflate bounding box.
        inflate_bounding_box(_bounding_box, NEAR_THRESHOLD);

        _bounding_boxes.push_back(_bounding_box);
    }

    return _bounding_boxes;
}


// get rtree from bounding boxes.
BRTree get_rtree_from_bounding_boxes(const std::vector<bounding_box_type> &_bounding_boxes)
{
    BRTree _rtree;
    for (int _i = 0; _i < _bounding_boxes.size(); _i++) {
        auto _bounding_box = _bounding_boxes[_i];
        BBox2d _bbox2d(BPoint2d(_bounding_box.first.x(), _bounding_box.first.y()),
                       BPoint2d(_bounding_box.second.x(), _bounding_box.second.y()));
        _rtree.insert(std::make_pair(_bbox2d, _i));
    }

    return _rtree;
}

    // check if given value in range.
bool is_in_range(const double& _value, const double& _start, const double& _end) {
    if (_value > _start - EPSILON && _value < _end + EPSILON) {
        return true;
    }

    return false;
}


// check if given value in range.
bool is_in_range(const double &_value, const Eigen::Vector2d & range)
{
    if (_value > range(0) - EPSILON && _value < range(1) + EPSILON) {
        return true;
    }

    return false;
}


// check if two bounding box intersect.
bool is_bounding_box_intersection(const bounding_box_type& _bounding_box1, const bounding_box_type& _bounding_box2) {
    bool _is_intersection = false;

    auto _min_corner1 = _bounding_box1.first;
    auto _max_corner1 = _bounding_box1.second;

    auto _min_corner2 = _bounding_box2.first;
    auto _max_corner2 = _bounding_box2.second;

    bool _x_range_intersect = is_in_range(_min_corner1(0), _min_corner2(0), _max_corner2(0)) ||
                              is_in_range(_max_corner1(0), _min_corner2(0), _max_corner2(0)) ||
                              is_in_range(_min_corner2(0), _min_corner1(0), _max_corner1(0)) ||
                              is_in_range(_max_corner2(0), _min_corner1(0), _max_corner1(0));

    bool _y_range_intersect = is_in_range(_min_corner1(1), _min_corner2(1), _max_corner2(1)) ||
                              is_in_range(_max_corner1(1), _min_corner2(1), _max_corner2(1)) ||
                              is_in_range(_min_corner2(1), _min_corner1(1), _max_corner1(1)) ||
                              is_in_range(_max_corner2(1), _min_corner1(1), _max_corner1(1));

    if (_x_range_intersect && _y_range_intersect) {
        _is_intersection = true;
    }

    return _is_intersection;
}




// get discrete points for ExtrusionPath.
std::vector<Point> discretize_extrusion_path(const ExtrusionPath &_path, const double _step_len)
{
    std::map<int, int> _index_mapping;

    return get_polyline_discrete_points(_path.polyline, _index_mapping, _step_len);
}


polyline_mesh_type get_mesh_from_polyline(const Polyline &_polyline)
{
    Eigen::MatrixXd _verts(_polyline.size(), 2);
    for (int _i = 0; _i < _polyline.size(); _i++) {
        _verts(_i, 0) = _polyline[_i].x();
        _verts(_i, 1) = _polyline[_i].y();
    }

    Eigen::MatrixXi _edges(_polyline.size() - 1, 2);
    for (int _i = 0; _i < _polyline.size() - 1; _i++) {
        _edges(_i, 0) = _i;
        _edges(_i, 1) = _i + 1;
    }

    return std::make_pair(_verts, _edges);
}


// get meshes from ExtrusionPaths.
std::vector<polyline_mesh_type> get_meshes_from_extrusion_paths(const ExtrusionPaths &_paths)
{
    std::vector<polyline_mesh_type> _polyline_meshes;
    for (auto _path: _paths) {
        auto _polyline_mesh = get_mesh_from_polyline(_path.polyline);
        _polyline_meshes.push_back(_polyline_mesh);
    }

    return _polyline_meshes;
}


// discretize ExtrusionPaths.
std::vector<std::vector<Point>> discretize_extrusion_paths(const ExtrusionPaths& _paths) {
    std::vector<std::vector<Point>> _discrete_paths;
    for (size_t _i = 0; _i < _paths.size(); _i++) {
        auto _discrete_path = discretize_extrusion_path(_paths[_i], DISCTETE_STEP_LEN);
        _discrete_paths.push_back(_discrete_path);
    }

    return _discrete_paths;
}


// convert points to eigen matrix.
Eigen::MatrixXd convert_pts2matrix(const std::vector<Point> _pts) {
    Eigen::MatrixXd _new_pts(_pts.size(), 2);
    for (int _i = 0; _i < _pts.size(); _i++) {
        _new_pts(_i, 0) = _pts[_i].x();
        _new_pts(_i, 1) = _pts[_i].y();
    }

    return _new_pts;
}


//convert polyline to boost linestring format.
linestring_type convert_polyline2linestring(Polyline _polyline) {
    linestring_type _linestring;

    for (auto _pt_it = _polyline.points.begin(); _pt_it < _polyline.points.end(); _pt_it++) {
        auto _boost_pt = point_type((*_pt_it).x(), (*_pt_it).y());
		_linestring.push_back(_boost_pt);
    }

    return _linestring;
}


// calculate distance between point and polyline.
double calc_point_polyline_distance(const Point& _pt, const Polyline& _polyline) { 
    auto   _b_pt = point_type(_pt.x(), _pt.y());
    
    // get linestring from polyline.
    auto   _linestring = convert_polyline2linestring(_polyline);
    
    // calculate distance.
    double _dist = boost::geometry::distance(_b_pt, _linestring);

    return _dist;
}


// calculate distance from points to polyline.
double calc_points_polyline_distance(const Eigen::MatrixXd & _pts, const Polyline & _polyline) {
    // init _polyline_verts.
    Eigen::MatrixXd _polyline_verts(_polyline.points.size(), 2);
    for (auto _polyline_pt_idx = 0; _polyline_pt_idx < _polyline.points.size(); _polyline_pt_idx++) {
        _polyline_verts(_polyline_pt_idx, 0) = _polyline.points[_polyline_pt_idx].x() * SCALING_FACTOR;
        _polyline_verts(_polyline_pt_idx, 1) = _polyline.points[_polyline_pt_idx].y() * SCALING_FACTOR;
    }

    // init _polyline_edges.
    Eigen::MatrixXi _polyline_edges(_polyline.points.size() - 1, 2);
    for (auto _i = 0; _i < _polyline.points.size() - 1; _i++) {
        _polyline_edges(_i, 0)  = _i;
        _polyline_edges(_i, 1) = _i + 1;
    }

    Eigen::VectorXd _squared_distances;
    Eigen::MatrixXi _indices;
    Eigen::MatrixXd _closest_pts;

    // calc distance from points to polyline.
    igl::point_mesh_squared_distance(_pts, _polyline_verts, _polyline_edges, _squared_distances, _indices, _closest_pts);

    double _min_dist = 1e10;
    int    _pt_count = _pts.rows();
    for (int _row_idx = 0; _row_idx < _pt_count; _row_idx++) {
        if (_squared_distances(_row_idx) < _min_dist) {
            _min_dist = _squared_distances(_row_idx);
        }
    }

    return _min_dist;
}


// calculate distance from points to polyline.
double calc_points2polyline_mesh_distance(const Eigen::MatrixXd &_pts, const polyline_mesh_type &_polyline_mesh)
{
    // init _polyline_verts.
    Eigen::MatrixXd _polyline_verts = _polyline_mesh.first;

    // init _polyline_edges.
    Eigen::MatrixXi _polyline_edges = _polyline_mesh.second;

    Eigen::VectorXd _squared_distances;
    Eigen::MatrixXi _indices;
    Eigen::MatrixXd _closest_pts;

    // calc distance from points to polyline.
    igl::point_mesh_squared_distance(_pts, _polyline_verts, _polyline_edges, _squared_distances, _indices, _closest_pts);

    double _min_dist = 1e10;
    int    _pt_count = _pts.rows();
    for (int _row_idx = 0; _row_idx < _pt_count; _row_idx++) {
        if (_squared_distances(_row_idx) < _min_dist) {
            _min_dist = _squared_distances(_row_idx);
        }
    }

    return _min_dist;
}


// calculate distance between two polylines.
double calc_polyline_distance(const Polyline &_polyline1, const Polyline &_polyline2)
{
    // get linestring from polyline.
    auto _linestring1 = convert_polyline2linestring(_polyline1);
    auto _linestring2 = convert_polyline2linestring(_polyline2);

    // calculate distance between two linestrings.
    double _dist = boost::geometry::distance(_linestring1, _linestring2);

    return _dist;
}


// check if infill polyline near perimeters.
bool is_infill_near_perimeters(ExtrusionPath *                 _infill_path,
                               ExtrusionPaths &                _perimeters,
                               std::vector<bounding_box_type> &_perimeter_bounding_boxes,
                               std::vector<int> &              _near_perimeter_indices)
{
    bool _is_near = false;
    double _min_dist = 1e10;
    
    auto   _infill_bounding_box = get_polyline_bounding_box(_infill_path->polyline);

    for (auto i = 0; i < _perimeters.size(); i++) {
        // get perimeter.
        auto _perimeter = _perimeters[i];
        auto _perimeter_bounding_box = _perimeter_bounding_boxes[i];

        if (!is_bounding_box_intersection(_infill_bounding_box, _perimeter_bounding_box)) {
            continue;
        }

        // get near dist threshold.
        auto _near_dist_threshold = scale_(_infill_path->width() / 2.0 + _perimeter.width() / 2.0);

        // calculate distance between infill and perimeter.
        double _dist = calc_polyline_distance(_infill_path->polyline, _perimeter.polyline);
        if (_dist < _near_dist_threshold) {
            // add perimeter index.
            _near_perimeter_indices.push_back(i);
        }
    }

    auto _near_perimeter_count = _near_perimeter_indices.size();
    if (_near_perimeter_count > 0) {
        // update _is_near.
        _is_near = true;
    }

    return _is_near;
}


// check if infill polyline near perimeters.
bool is_infill_near_perimeters(ExtrusionPath *                 _infill_path,
                               ExtrusionPaths &                _perimeters,
                               BRTree &                        _perimeter_rtree,
                               std::vector<int> &              _near_perimeter_indices)
{
    bool   _is_near  = false;
    double _min_dist = 1e10;

    auto _infill_bounding_box = get_polyline_bounding_box(_infill_path->polyline);
    BBox2d _infill_bbox2d(BPoint2d(_infill_bounding_box.first.x(), _infill_bounding_box.first.y()),
                          BPoint2d(_infill_bounding_box.second.x(), _infill_bounding_box.second.y()));

    std::vector<box_index_value> _result_box_indices;
    _perimeter_rtree.query(bgi::intersects(_infill_bbox2d), std::back_inserter(_result_box_indices));

    for (auto _box_index_info: _result_box_indices) {
        // get perimeter.
        auto _perimeter_idx = _box_index_info.second;
        auto _perimeter = _perimeters[_perimeter_idx];

        // get near dist threshold.
        auto _near_dist_threshold = scale_(_infill_path->width() / 2.0 + _perimeter.width() / 2.0);

        // calculate distance between infill and perimeter.
        double _dist = calc_polyline_distance(_infill_path->polyline, _perimeter.polyline);
        if (_dist < _near_dist_threshold) {
            // add perimeter index.
            _near_perimeter_indices.push_back(_perimeter_idx);
        }
    }
    
    auto _near_perimeter_count = _near_perimeter_indices.size();
    if (_near_perimeter_count > 0) {
        // update _is_near.
        _is_near = true;
    }

    return _is_near;
}



// check if infill polyline near perimeters.
bool is_near_perimeters(ExtrusionPath *   _path,
                        ExtrusionPaths &  _perimeters,
                        BRTree &          _perimeter_rtree,
                        const double      _near_dist_threshold,
                        std::vector<int> &_near_perimeter_indices)
{
    bool   _is_near  = false;
    double _min_dist = 1e10;

    auto   _bounding_box = get_polyline_bounding_box(_path->polyline);
    BBox2d _bbox2d(BPoint2d(_bounding_box.first.x(), _bounding_box.first.y()), BPoint2d(_bounding_box.second.x(), _bounding_box.second.y()));

    std::vector<box_index_value> _result_box_indices;
    _perimeter_rtree.query(bgi::intersects(_bbox2d), std::back_inserter(_result_box_indices));

    for (auto _box_index_info : _result_box_indices) {
        // get perimeter.
        auto _perimeter_idx = _box_index_info.second;
        auto _perimeter     = _perimeters[_perimeter_idx];

        // calculate distance between infill and perimeter.
        double _dist = calc_polyline_distance(_path->polyline, _perimeter.polyline);
        if (_dist < _near_dist_threshold) {
            // add perimeter index.
            _near_perimeter_indices.push_back(_perimeter_idx);
        }
    }

    auto _near_perimeter_count = _near_perimeter_indices.size();
    if (_near_perimeter_count > 0) {
        // update _is_near.
        _is_near = true;
    }

    return _is_near;
}


// check if infill polyline near perimeters.
bool is_infill_near_perimeters(ExtrusionPath *                  _infill_path,
                               ExtrusionPaths &                 _perimeters,
                               std::vector<polyline_mesh_type> &_perimeter_meshes,
                               std::vector<bounding_box_type>  &_perimeter_bounding_boxes,
                               std::vector<int> &               _near_perimeter_indices){
    bool   _is_near  = false;
    double _min_dist = 1e10;

    auto _infill_discrete_pts = discretize_extrusion_path(*_infill_path, DISCTETE_STEP_LEN);
    auto _infill_matrix_pts   = convert_pts2matrix(_infill_discrete_pts);
    auto _infill_bounding_box = get_polyline_bounding_box(_infill_path->polyline);

    for (auto i = 0; i < _perimeters.size(); i++) {
        // get perimeter.
        auto _perimeter          = _perimeters[i];
        auto _perimeter_mesh = _perimeter_meshes[i];
        auto _perimeter_bounding_box = _perimeter_bounding_boxes[i];

        if (!is_bounding_box_intersection(_infill_bounding_box, _perimeter_bounding_box)) {
            continue;
        }
        
        // get near dist threshold.
        auto _near_dist_threshold = scale_(_infill_path->width() / 2.0 + _perimeter.width() / 2.0);

        // calculate distance between infill and perimeter.
        double _dist = calc_points2polyline_mesh_distance(_infill_matrix_pts, _perimeter_mesh);
        if (_dist < _near_dist_threshold) {
            // add perimeter index.
            _near_perimeter_indices.push_back(i);
        }
    }

    auto _near_perimeter_count = _near_perimeter_indices.size();
    if (_near_perimeter_count > 0) {
        // update _is_near.
        _is_near = true;
    }


    return _is_near;
}


// check if infill polyline near perimeters.
bool is_infill_near_perimeters(ExtrusionPath *                  _infill_path,
                               ExtrusionPaths &                 _perimeters,
                               std::vector<std::vector<Point>> &_discrete_perimeters,
                               std::vector<int> &               _near_perimeter_indices)
{
    bool   _is_near  = false;
    double _min_dist = 1e10;

    for (auto i = 0; i < _perimeters.size(); i++) {
        // get perimeter.
        auto _perimeter          = _perimeters[i];
        auto _discrete_perimeter = _discrete_perimeters[i];

        // get near dist threshold.
        auto _near_dist_threshold = scale_(_infill_path->width() / 2.0 + _perimeter.width() / 2.0);

        // calculate distance between infill and perimeter.
        Eigen::MatrixXd _perimeter_pts = convert_pts2matrix(_discrete_perimeter);
        double _dist = calc_points_polyline_distance(_perimeter_pts, _infill_path->polyline);
        if (_dist < _near_dist_threshold) {
            // add perimeter index.
            _near_perimeter_indices.push_back(i);
        }
    }

    auto _near_perimeter_count = _near_perimeter_indices.size();
    if (_near_perimeter_count > 0) {
        // update _is_near.
        _is_near = true;
    }

    return _is_near;
}


// get mapping from index of point to perimeter.
std::map<int, std::pair<int, double>> get_point_perimeter_mapping_deprecated(const std::vector<Point> & _infill_discrete_pts, const ExtrusionPaths &_perimeters)
{
    // get mapping from index of point to perimeter.
    std::map<int, std::pair<int, double>> _pt_perimeter_mapping;

    for (int _pt_idx = 0; _pt_idx < _infill_discrete_pts.size(); _pt_idx++) {
        auto   _pt                    = _infill_discrete_pts[_pt_idx];
        double _pt_perimeter_min_dist = 1e10;
        int    _nearest_perimeter_idx = -1;

        for (auto _perimeter_idx = 0; _perimeter_idx < _perimeters.size(); _perimeter_idx++) {
            auto _perimeter = _perimeters[_perimeter_idx];

            // calculate distance from point to polyline.
            double _pt_perimeter_dist = calc_point_polyline_distance(_pt, _perimeter.polyline);
            printf("dist: %f\n", _pt_perimeter_dist);

            if (_pt_perimeter_dist < _pt_perimeter_min_dist) {
                // update _min_dist and _nearest_perimeter_idx.
                _pt_perimeter_min_dist = _pt_perimeter_dist;
                _nearest_perimeter_idx = _perimeter_idx;
            }
        }

        // update _pt_perimeter_mapping.
        _pt_perimeter_mapping[_pt_idx] = std::make_pair(_nearest_perimeter_idx, _pt_perimeter_min_dist);
    }

    return _pt_perimeter_mapping;
}


// get mapping from index of point to perimeter.
std::map<int, std::pair<int, double>> get_point_perimeter_mapping(const std::vector<Point> &_infill_discrete_pts,
                                                                  const ExtrusionPaths &    _perimeters,
                                                                  std::map<int, Vec2d> &    _closest_pt_mapping)
{   
    // get mapping from index of point to perimeter.
    std::map<int, std::pair<int, double>> _pt_perimeter_mapping;  

    // init _pt_perimeter_distances.
    Eigen::MatrixXd _pt_perimeter_distances(_infill_discrete_pts.size(), _perimeters.size());
    Eigen::MatrixXd _pt_perimeter_closest_pts(_infill_discrete_pts.size(), _perimeters.size() * 2);

    // init _infill_pts.
    Eigen::MatrixXd _infill_pts(_infill_discrete_pts.size(), 2);
    for (auto _i = 0; _i < _infill_discrete_pts.size(); _i++) {
        _infill_pts(_i, 0) = _infill_discrete_pts[_i].x() * SCALING_FACTOR;
        _infill_pts(_i, 1) = _infill_discrete_pts[_i].y() * SCALING_FACTOR;
    }

    // calculate _pt_perimeter_distances.
    for (auto _perimeter_idx = 0; _perimeter_idx < _perimeters.size(); _perimeter_idx++) {
        // get _perimeter.
        auto _perimeter = _perimeters[_perimeter_idx];
        // get _perimeter_pts.
        auto _perimeter_pts = _perimeter.polyline.points;

        // init _perimeter_verts.
        Eigen::MatrixXd _perimeter_verts(_perimeter_pts.size(), 2);
        for (auto _perimeter_pt_idx = 0; _perimeter_pt_idx < _perimeter_pts.size(); _perimeter_pt_idx++) {
            _perimeter_verts(_perimeter_pt_idx, 0) = _perimeter_pts[_perimeter_pt_idx].x() * SCALING_FACTOR;
            _perimeter_verts(_perimeter_pt_idx, 1) = _perimeter_pts[_perimeter_pt_idx].y() * SCALING_FACTOR;
        }

        // init _perimeter_edges.
        Eigen::MatrixXi _perimeter_edges(_perimeter_pts.size() - 1, 2);
        for (auto _i = 0; _i < _perimeter_pts.size() - 1; _i++) {
            _perimeter_edges(_i, 0) = _i;
            _perimeter_edges(_i, 1) = _i + 1;
        }

        Eigen::VectorXd _squared_distances;
        Eigen::MatrixXi _indices;
        Eigen::MatrixXd _closest_pts;

        // calculate distances from _infill_pts to _perimeter.
        igl::point_mesh_squared_distance(_infill_pts, _perimeter_verts, _perimeter_edges, _squared_distances, _indices, _closest_pts);
        
        for (auto _infill_pt_idx = 0; _infill_pt_idx < _infill_discrete_pts.size(); _infill_pt_idx++) {
            _pt_perimeter_distances(_infill_pt_idx, _perimeter_idx) = _squared_distances(_infill_pt_idx);
            _pt_perimeter_closest_pts(_infill_pt_idx, 2 * _perimeter_idx) = _closest_pts(_infill_pt_idx, 0);
            _pt_perimeter_closest_pts(_infill_pt_idx, 2 * _perimeter_idx + 1) = _closest_pts(_infill_pt_idx, 1);
        }
    }

    for (auto _infill_pt_idx = 0; _infill_pt_idx < _infill_discrete_pts.size(); _infill_pt_idx++) {
        double _pt_perimeter_min_dist = 1e10;
        int    _nearest_perimeter_idx = -1;
        for (auto _perimeter_idx = 0; _perimeter_idx < _perimeters.size(); _perimeter_idx++) {
            if (_pt_perimeter_distances(_infill_pt_idx, _perimeter_idx) < _pt_perimeter_min_dist) {
                _pt_perimeter_min_dist = _pt_perimeter_distances(_infill_pt_idx, _perimeter_idx);
                _nearest_perimeter_idx = _perimeter_idx;
            }
        }

        // update _pt_perimeter_mapping.
        _pt_perimeter_min_dist = scale_(1.0) * std::sqrt(_pt_perimeter_min_dist);
        _pt_perimeter_mapping[_infill_pt_idx] = std::make_pair(_nearest_perimeter_idx, _pt_perimeter_min_dist);
        _closest_pt_mapping[_infill_pt_idx]   = Vec2d(_pt_perimeter_closest_pts(_infill_pt_idx, 2 * _nearest_perimeter_idx), _pt_perimeter_closest_pts(_infill_pt_idx, 2 * _nearest_perimeter_idx + 1));
    }

    return _pt_perimeter_mapping;
}


// get index ranges for infill discrete points.
std::vector<std::pair<int, int>> get_index_ranges_deprecated(const double _infill_width, 
                                                  const std::vector<Point> & _infill_discrete_pts,
                                                  const ExtrusionPaths & _perimeters,
                                                  std::map<int, std::pair<int, double>> &_pt_perimeter_mapping,
                                                  std::map<int, Vec2d> &                 _closest_pt_mapping)
{
    std::vector<std::pair<int, int>> _index_ranges;
    std::pair<int, int>              _index_range = std::make_pair(-1, -1);

    // get index ranges for infill discrete points.
    for (int _pt_idx = 0; _pt_idx < _infill_discrete_pts.size(); _pt_idx++) {
        auto _nearest_perimeter     = _perimeters[_pt_perimeter_mapping[_pt_idx].first];

        // get near dist threshold.
        auto _near_dist_threshold   = scale_(_infill_width / 2.0 + _nearest_perimeter.width() / 2.0);
        bool _is_pt_near_perimeters = (_pt_perimeter_mapping[_pt_idx].second < _near_dist_threshold);
        
        if (fabs(_index_range.first + 1) < EPSILON && fabs(_index_range.second + 1) < EPSILON) {
            // _index_range is empty.
            if (_is_pt_near_perimeters) {
                // current point is near perimeters.
                _index_range.first  = _pt_idx;
                _index_range.second = _pt_idx;
            }
        } else {
            // _index_range is not empty.
            if (_pt_idx == _index_range.second + 1) {
                if (_is_pt_near_perimeters) {
                    // current point is near perimeters.
                    _index_range.second = _pt_idx;
                } else {
                    // current point is away from perimeters.
                    if (fabs(_index_range.second - _index_range.first + 1) > 2 - EPSILON) {
                        // the length of index range >= 2.
                        // add range to _index_ranges.
                        _index_ranges.push_back(std::make_pair(_index_range.first, _index_range.second));
                    }

                    // refresh _index_range.
                    _index_range.first  = -1;
                    _index_range.second = -1;
                }
            }
        }
    }

    if (fabs(_index_range.second - _index_range.first + 1) > 2 - EPSILON) {
        // the length of index range >= 2.
        // add range to _index_ranges.
        _index_ranges.push_back(_index_range);
    }

    return _index_ranges;
}


// check if 2 points are near segment.
bool is_near_pair_valid(const Vec2d &_pt0, const Vec2d &_pt1, const Vec2d &_close_pt0, const Vec2d &_close_pt1)
{
    Vec2d _line_vt = _pt1 - _pt0;
    _line_vt.normalize(); 

    Vec2d _normal_vt0 = _pt0 - _close_pt0;
    _normal_vt0.normalize();

    Vec2d _normal_vt1 = _pt1 - _close_pt1;
    _normal_vt1.normalize();

    // evaluate segment validity.
    bool _is_valid = (fabs(_line_vt.dot(_normal_vt0)) < INFILL_PERIMETER_ANGLE_EPSILON) ||
                     (fabs(_line_vt.dot(_normal_vt1)) < INFILL_PERIMETER_ANGLE_EPSILON); 
    return _is_valid;
}


// get index ranges for infill discrete points.
std::vector<std::pair<int, int>> get_index_ranges(const double                           _infill_width,
                                                  const std::vector<Point> &             _infill_discrete_pts,
                                                  const ExtrusionPaths &                 _perimeters,
                                                  std::map<int, std::pair<int, double>> &_pt_perimeter_mapping,
                                                  std::map<int, Vec2d> &                 _closest_pt_mapping)
{
    std::vector<std::pair<int, int>> _index_ranges;
    std::pair<int, int>              _index_range = std::make_pair(-1, -1);

    // get index ranges for infill discrete points.
    for (int _pt_idx = 0; _pt_idx < _infill_discrete_pts.size(); _pt_idx++) {
        auto _nearest_perimeter = _perimeters[_pt_perimeter_mapping[_pt_idx].first];

        // get near dist threshold.
        auto _near_dist_threshold   = scale_(_infill_width / 2.0 + _nearest_perimeter.width() / 2.0);
        bool _is_pt_near_perimeters = (_pt_perimeter_mapping[_pt_idx].second < _near_dist_threshold);

        if (fabs(_index_range.first + 1) < EPSILON && fabs(_index_range.second + 1) < EPSILON) {
            // _index_range is empty.
            if (_is_pt_near_perimeters) {
                // current point is near perimeters.
                _index_range.first  = _pt_idx;
                _index_range.second = _pt_idx;
            }
        } else {
            // _index_range is not empty.
            if (_pt_idx == _index_range.second + 1) {
                auto _last_pt    = _infill_discrete_pts[_index_range.second];
                auto _last_vec2d = Vec2d(_last_pt.x() * SCALING_FACTOR, _last_pt.y() * SCALING_FACTOR);

                auto _pt    = _infill_discrete_pts[_pt_idx];
                auto _vec2d = Vec2d(_pt.x() * SCALING_FACTOR, _pt.y() * SCALING_FACTOR);
                
                bool _is_near_pair = is_near_pair_valid(_last_vec2d, _vec2d, _closest_pt_mapping[_index_range.second], _closest_pt_mapping[_pt_idx]);

                if (_is_pt_near_perimeters) {
                    // current point is near perimeters.
                    if (_is_near_pair) {
                        // current point and last point form near pair.
                        _index_range.second = _pt_idx;
                    } else {
                        // current point and last point are not near pair.
                        if (fabs(_index_range.second - _index_range.first + 1) > 2 - EPSILON) {
                            // the length of index range >= 2.
                            // add range to _index_ranges.
                            _index_ranges.push_back(std::make_pair(_index_range.first, _index_range.second));
                        }

                        // refresh _index_range.
                        _index_range.first  = _pt_idx;
                        _index_range.second = _pt_idx;
                    }
                } else {
                    // current point is away from perimeters.
                    if (fabs(_index_range.second - _index_range.first + 1) > 2 - EPSILON) {
                        // the length of index range >= 2.
                        // add range to _index_ranges.
                        _index_ranges.push_back(std::make_pair(_index_range.first, _index_range.second));
                    }

                    // refresh _index_range.
                    _index_range.first  = -1;
                    _index_range.second = -1;
                }
            }
        }
    }

    if (fabs(_index_range.second - _index_range.first + 1) > 2 - EPSILON) {
        // the length of index range >= 2.
        // add range to _index_ranges.
        _index_ranges.push_back(_index_range);
    }

    return _index_ranges;
}


// get index ranges for infill discrete points.
std::vector<std::tuple<int, int, int>> get_index_ranges(const std::vector<Point> &             _internal_perimeter_discrete_pts,
                                                        const ExtrusionPaths &                 _perimeters,
                                                        std::map<int, std::pair<int, double>> &_pt_perimeter_mapping)
{
    std::vector<std::tuple<int, int, int>> _index_ranges;
    std::tuple<int, int, int>              _index_range = std::make_tuple(-1, -1, -1);

    // get index ranges for internal perimeter discrete points.
    size_t _discrete_pts_count = _internal_perimeter_discrete_pts.size();
    int _last_mapped_perimeter_idx = -1;
    for (int _pt_idx = 0; _pt_idx < _discrete_pts_count; _pt_idx++) {
        auto _mapped_perimeter_idx = _pt_perimeter_mapping[_pt_idx].first;
        if (fabs(std::get<0>(_index_range) + 1) < EPSILON && fabs(std::get<1>(_index_range) + 1) < EPSILON) {
            // _index_range is empty.
            _index_range = std::make_tuple(_pt_idx, _pt_idx, _mapped_perimeter_idx);
        } else {
            // _index_range is not empty.
            if (_pt_idx >= _discrete_pts_count - 2) {
                // current pt is not at the tail of internal perimeter.
                _index_ranges.push_back(std::make_tuple(std::get<0>(_index_range), _discrete_pts_count - 1, _mapped_perimeter_idx));

                // init _index_range.
                _index_range = std::make_tuple(-1, -1, -1);

                break;
            } else {
                // current pt is not at the tail of internal perimeter.
                if (_mapped_perimeter_idx == _last_mapped_perimeter_idx) {
                    // current point and last point are mapped to the same perimeter.
                    std::get<1>(_index_range) = _pt_idx;
                } else {
                    if (_pt_idx - std::get<0>(_index_range) <= 1 + EPSILON) {
                        std::get<1>(_index_range) = _pt_idx;
                        std::get<2>(_index_range) = _mapped_perimeter_idx;
                    } else {
                        // current point and last point are mapped to different perimeters.
                        _index_ranges.push_back(std::make_tuple(std::get<0>(_index_range), _pt_idx, _last_mapped_perimeter_idx));

                        // init _index_range.
                        _index_range = std::make_tuple(_pt_idx, _pt_idx, _mapped_perimeter_idx);
                    }
                }
            }
        }

        // update _last_mapped_perimeter_idx.
        _last_mapped_perimeter_idx = _mapped_perimeter_idx;
    }

    if (fabs(std::get<0>(_index_range) - std::get<1>(_index_range) + 1) > 2 - EPSILON) {
        // the length of index range >= 2.
        // add range to _index_ranges.
        _index_ranges.push_back(_index_range);
    }

    return _index_ranges;
}


// get polyline from index range.
Polyline get_polyline_from_index_range(const std::pair<int, int> & _range,
                                       const std::vector<Point> & _pts,
                                       std::map<int, int> & _index_mapping,
                                       const int polyline_vert_count)
{ 
    Polyline _polyline;

    // init _vert_count_mapping.
    std::map<int, int> _vert_count_mapping;
    for (int _vert_idx = 0; _vert_idx < polyline_vert_count; _vert_idx++) {
        _vert_count_mapping.insert(std::make_pair(_vert_idx, 0));
    }

    for (int _pt_idx = _range.first; _pt_idx <= _range.second; _pt_idx++) {
        auto _pt = _pts[_pt_idx];
        if (_pt_idx == _range.second) {
            // _pt is at the end of the range.
            _polyline.append(_pt);
            break;
        }

        auto _vert_count      = _vert_count_mapping[_index_mapping[_pt_idx]]; 
        if (_vert_count == 0) {
            // _pt is at the head of the line.
            _polyline.append(_pt);
        } else if (_vert_count >= 1) {
            // _pt is not at the head of line.
            auto _next_pt_idx = (_pt_idx + 1) % _index_mapping.size();
            if (_index_mapping[_pt_idx] == _index_mapping[_next_pt_idx]) {
                // _pt is in the middle of line.
            } else {
                // _pt is at the tail of the line.
                _polyline.append(_pt);
            }
        }

        // update _vert_count_mapping.
        _vert_count_mapping[_index_mapping[_pt_idx]] += 1;
    }
    
    return _polyline;
}


// get infill width.
double get_infill_width(const double                           _src_infill_width,
                        const ExtrusionPaths &                 _perimeters,
                        const std::pair<int, int> &            _range,
                        std::map<int, std::pair<int, double>> &_pt_perimeter_mapping)
{
    // evaluate sum of dist from infill discrete points to perimeter.
    double _sum_dist  = 0.0;
    std::map<int, int> _count_mapping;

    for (int _pt_idx = _range.first; _pt_idx <= _range.second; _pt_idx++) {
        auto _mapped_perimeter_info = _pt_perimeter_mapping[_pt_idx];
        
        // update _sum_dist.
        _sum_dist += _mapped_perimeter_info.second;
        
        // update _count_mapping.
        if (_count_mapping.find(_mapped_perimeter_info.first) == _count_mapping.end()) {
            _count_mapping.insert(std::make_pair(_mapped_perimeter_info.first, 1));
        } else {
            _count_mapping[_mapped_perimeter_info.first] += 1;
        }
    }
    // get mean dist from infill polyline to perimeter.
    double _mean_dist = _sum_dist / (_range.second - _range.first + 1);

    // get nearest perimeter.
    int _nearest_perimeter_idx = -1;
    int _max_count             = -1;
    for (auto _count_it : _count_mapping) {
        if (_count_it.second > _max_count) {
            _max_count             = _count_it.second;
            _nearest_perimeter_idx = _count_it.first;
        }
    }

    //ExtrusionPath _nearest_perimeter;
    double _nearest_perimeter_width = scale_(1.0) * _perimeters[_nearest_perimeter_idx].width();

    // evaluate target width of infill.
    double _width = SCALING_FACTOR *(_mean_dist - 0.5 * _nearest_perimeter_width) / (0.5 - INFILL_PERIMETER_OVERLAP_SCALE);
    if (_width < INFILL_MIN_WIDTH) {
        _width = INFILL_MIN_WIDTH;
    } else if (_width > _src_infill_width) {
        _width = _src_infill_width;
    }

    return _width;
}


// modify width of infill polyline.
ExtrusionPaths segment_infill_path(ExtrusionPath *_infill_path, const ExtrusionPaths &_perimeters)
{
    if (_infill_path->role() == ExtrusionRole::TopSolidInfill) {
        // _infill_path is TopSolidInfill.
        return ExtrusionPaths();
    }

    // get point count of infill polyline.
    int _infill_pt_count = _infill_path->polyline.size();
    auto _infill_attributes = _infill_path->attributes();

    // discretize polyline with given length.
    std::map<int, int> _index_mapping;
    std::vector<Point> _infill_discrete_pts = get_polyline_discrete_points(_infill_path->polyline, _index_mapping, DISCTETE_STEP_LEN);

    // get mapping from index of point to perimeter.
    std::map<int, Vec2d>                  _closest_pt_mapping;
    std::map<int, std::pair<int, double>> _pt_perimeter_mapping = get_point_perimeter_mapping(_infill_discrete_pts, _perimeters, _closest_pt_mapping);

    // get index ranges for infill discrete points.
    auto _index_ranges = get_index_ranges(_infill_attributes.width, _infill_discrete_pts, _perimeters, _pt_perimeter_mapping, _closest_pt_mapping);

    // get infill segments.
    ExtrusionPaths _infill_segments;

    for (auto _range_idx = 0; _range_idx < _index_ranges.size(); _range_idx++) {
        auto _near_range = _index_ranges[_range_idx];

        // get near polyline from index range.
        Polyline _near_polyline = get_polyline_from_index_range(_near_range, _infill_discrete_pts, _index_mapping, _infill_pt_count);
        
        // init _near_attributes.
        ExtrusionAttributes _near_attributes = _infill_attributes;

        // get width of _near_segment.
        double _near_segment_width = get_infill_width(_infill_path->width(), _perimeters, _near_range, _pt_perimeter_mapping);

        // modify mm3_per_mm and width of _near_attributes.
        _near_attributes.mm3_per_mm               = _near_attributes.mm3_per_mm * _near_segment_width / _near_attributes.width;
        _near_attributes.width                    = _near_segment_width;
        _near_attributes.is_infill_near_perimeter = true;

        // construct _near_segment.
        ExtrusionPath _near_segment(_near_polyline, _near_attributes);

        if (_range_idx == 0) {
            // first range.
            if (_near_range.first == 0) {
                // first range starts from the head of infill polyline.
                // add _away_segment to _infill_segments.
                _infill_segments.push_back(_near_segment);
            } else {
                // first range starts from middle of infill polyline.
                // get _away_range.
                auto _away_range = std::make_pair(0, _near_range.first);

                // get _away_polyline before _near_polyline.
                Polyline _away_polyline = get_polyline_from_index_range(_away_range, _infill_discrete_pts, _index_mapping, _infill_pt_count);
                // construct _away_segment.
                ExtrusionPath _away_segment(_away_polyline, _infill_attributes);
                
                // add _away_segment to _infill_segments.
                _infill_segments.push_back(_away_segment);
                // add _near_segment to _infill_segments.
                _infill_segments.push_back(_near_segment);
            }
        }
        else {
            auto _last_range_idx = _range_idx - 1;
            auto _last_range     = _index_ranges[_last_range_idx];
            auto _away_range     = std::make_pair(_last_range.second, _near_range.first);

            // get _away_polyline before _near_polyline.
            Polyline _away_polyline = get_polyline_from_index_range(_away_range, _infill_discrete_pts, _index_mapping, _infill_pt_count);
            // construct _away_segment.
            ExtrusionPath _away_segment(_away_polyline, _infill_attributes);

            // add _away_segment to _infill_segments.
            _infill_segments.push_back(_away_segment);

            // add _near_segment to _infill_segments.
            _infill_segments.push_back(_near_segment);
        }

        if (_range_idx == _index_ranges.size() - 1) {
            // last range.
            if (_near_range.second < _infill_discrete_pts.size() - 1) {
                // get last away range.
                auto _last_away_range = std::make_pair(_near_range.second, _infill_discrete_pts.size() - 1);

                // get last away polyline after _near_polyline.
                Polyline      _last_away_polyline = get_polyline_from_index_range(_last_away_range, _infill_discrete_pts, _index_mapping, _infill_pt_count);
                // construct _last_away_segment.
                ExtrusionPath _last_away_segment(_last_away_polyline, _infill_attributes);

                // add _last_away_segment to _infill_segments.
                _infill_segments.push_back(_last_away_segment);
            }
        }
    }

    return _infill_segments;
}


// segment internal perimeter path.
ExtrusionPaths segment_internal_perimeter_path(ExtrusionPath* _internal_perimeter, const ExtrusionPaths& _external_perimeters) {
    // get point count of internal perimeter polyline.
    int  _internal_perimeter_pt_count   = _internal_perimeter->polyline.size();
    auto _internal_perimeter_attributes = _internal_perimeter->attributes();

    // discretize polyline with given length.
    std::map<int, int> _index_mapping;
    std::vector<Point> _internal_perimeter_discrete_pts = get_polyline_discrete_points(_internal_perimeter->polyline, _index_mapping, DISCTETE_STEP_LEN);

    // get mapping from index of point to perimeter.
    std::map<int, Vec2d>                  _closest_pt_mapping;
    std::map<int, std::pair<int, double>> _pt_external_perimeter_mapping = get_point_perimeter_mapping(_internal_perimeter_discrete_pts,
                                                                                                       _external_perimeters,
                                                                                                       _closest_pt_mapping);

    // get index ranges for discrete points of internal perimeter.
    auto _index_ranges = get_index_ranges(_internal_perimeter_discrete_pts, _external_perimeters, _pt_external_perimeter_mapping);

    // get internal perimeter segments.
    ExtrusionPaths _internal_perimeter_segments;
    for (auto _range_idx = 0; _range_idx < _index_ranges.size(); _range_idx++) {
        auto _index_range = _index_ranges[_range_idx];

        std::pair<int, int> _pair_index = std::make_pair(std::get<0>(_index_range), std::get<1>(_index_range));
        int _mapped_external_perimeter_idx = std::get<2>(_index_range);

        // get near polyline from index range.
        Polyline _near_polyline = get_polyline_from_index_range(_pair_index, _internal_perimeter_discrete_pts, _index_mapping,
                                                                _internal_perimeter_pt_count);

        // init _near_attributes.
        ExtrusionAttributes _segment_attributes = _internal_perimeter_attributes;
        _segment_attributes.overhang_attributes = _external_perimeters[_mapped_external_perimeter_idx].attributes().overhang_attributes;

        // construct _near_segment.
        ExtrusionPath _segment(_near_polyline, _segment_attributes);

        // append _segment.
        _internal_perimeter_segments.push_back(_segment);
    }

    return _internal_perimeter_segments;
}


}

