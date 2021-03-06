//
//  VoronoiSphere.h
//  Voronoi
//
//  Created by Ellis Sparky Hoag on 6/3/16.
//  Copyright © 2016 Ellis Sparky Hoag. All rights reserved.
//

#ifndef VoronoiSphere_h
#define VoronoiSphere_h

#include <iostream>
#include <vector>
#include <queue>
#include <cmath>
#include <thread>
#include <assert.h>

//#include <boost/multiprecision/float128.hpp>

#include <iomanip>

#define MAX_SKIPLIST_HEIGHT 15

#define TWO_PI_3 2.0943951023931954923084289221863353 // 2 * PI / 3
#define FOUR_PI_3 4.1887902047863909846168578443726705 // 4 * PI / 3

#define SIN_TWO_PI_3 0.86602540378443864676372317075293618 // sqrt(3) / 2
#define SIN_FOUR_PI_3 -SIN_TWO_PI_3
#define COS_TWO_PI_3 -0.5
#define COS_FOUR_PI_3 COS_TWO_PI_3

#define ARCTAN_2_ROOT_2 1.2309594173407746821349291782479874 // arctan(2 * sqrt(2))

#define ARCSIN_ONE_THIRD_PLUS_PI_2 1.9106332362490185563277142050315155 // arcsin(1/3) + PI/2

#define SIN_ARCSIN_ONE_THIRD_PLUS_PI_2 0.94280904158206336586779248280646539 // 2 * sqrt(2) / 3
#define COS_ARCSIN_ONE_THIRD_PLUS_PI_2 -0.33333333333333333333333333333333333
#define SIN_NEG_ARCSIN_ONE_THIRD_PLUS_PI_2 -SIN_ARCSIN_ONE_THIRD_PLUS_PI_2
#define COS_NEG_ARCSIN_ONE_THIRD_PLUS_PI_2 COS_ARCSIN_ONE_THIRD_PLUS_PI_2

namespace Voronoi
{
    //typedef boost::multiprecision::float128 Real;
    /*
     *  To compile: g++ main.cpp voronoi_sphere.cpp -std=c++11 -fext-numeric-literals -framework OpenGL -framework SDL2 -lquadmath -Ofast
     */
    typedef double Real;
    
    struct Edge;
    struct VoronoiDiagramSphere;
    struct PointCartesian;
    struct CircleEventSphere;
    struct ArcSphere;
    struct PointSphere;
    struct VoronoiCellSphere;
    struct HalfEdgeSphere;
    struct CompareTopDown;
    struct CompareBottomUp;
    
    enum THREAD_NUMBER
    {
        ONE_THREAD = 1,
        TWO_THREADS = 2,
        FOUR_THREADS = 4
    };
    
    VoronoiDiagramSphere generate_voronoi(std::vector<std::tuple<Real, Real, Real>> * verts, THREAD_NUMBER num_threads = ONE_THREAD, void (*render)(VoronoiDiagramSphere, ArcSphere *, std::vector<VoronoiCellSphere> *, Real) = NULL, bool (*is_sleeping)() = NULL);
    
    struct Edge
    {
        Edge(int start_idx = 0, int end_idx = 0)
        {
            vidx[0] = start_idx;
            vidx[1] = end_idx;
        }
        unsigned int vidx[2];
    };
    
    struct VoronoiDiagramSphere
    {
        std::vector<PointCartesian> sites, voronoi_vertices;
        
        std::vector<Edge> voronoi_edges, delaunay_edges;
    };
    
    struct PointCartesian
    {
        PointCartesian(Real a = 0, Real b = 0, Real c = 0) : x(a), y(b), z(c) {}
                
        inline friend PointCartesian operator-(const PointCartesian left, const PointCartesian right) {return PointCartesian(left.x - right.x, left.y - right.y, left.z - right.z);}
        
        friend std::ostream & operator<<(std::ostream & out, const PointCartesian & p) {return out << "(" << p.x << ", " << p.y << ", " << p.z << ")\n";}
        
        inline void normalize()
        {
            Real r = sqrt(x*x + y*y + z*z);
            if (r == 0)
            {
                std::cout << "r = 0\n";
                x = y = z = 0;
            }
            else
            {
                x /= r;
                y /= r;
                z /= r;
            }
        }
        
        inline static PointCartesian cross_product(const PointCartesian & left, const PointCartesian & right)
        {
            Real x = left.y * right.z - left.z * right.y;
            Real y = left.z * right.x - left.x * right.z;
            Real z = left.x * right.y - left.y * right.x;
            return PointCartesian(x, y, z);
        }
        
        Real x, y, z;
    };
    
    /*
     *  Points on the sphere are stored as (theta, phi).
     *  0 <= theta < 2PI is the angle from the north pole.
     *  -PI < phi <= PI is the angle around the sphere from the x-axis.
     */
    struct PointSphere
    {
        Real theta, phi;
        
        Real x, y, z;
        
        PointSphere(Real t = 0, Real p = 0) : theta(t), phi(p), x(0), y(0), z(0), has_cartesian(false) {}
        
        PointSphere(Real _x, Real _y, Real _z) : theta(acos(_z)), phi(atan2(_y, _x)), x(_x), y(_y), z(_z), has_cartesian(true) {}
        
        PointSphere(PointCartesian point) : PointSphere(point.x, point.y, point.z) {}
        
        inline friend bool operator<(const PointSphere & left, const PointSphere & right) {return left.theta == right.theta ? left.phi < right.phi : left.theta < right.theta;}
        inline friend bool operator>(const PointSphere & left, const PointSphere & right) {return left.theta == right.theta ? left.phi > right.phi : left.theta > right.theta;}
        
        friend std::ostream & operator<<(std::ostream & out, const PointSphere & p) {return out << "(" << p.theta << ", " << p.phi << ")\n";}
        
        inline PointCartesian get_cartesian()
        {
            set_cartesian();
            return PointCartesian(x, y, z);
        }
        
        inline static Real angle_between(PointSphere & a, PointSphere & b)
        {
            a.set_cartesian();
            b.set_cartesian();
            return acos(a.x * b.x + a.y * b.y + a.z * b.z);
        }
        
    private:
        
        inline void set_cartesian()
        {
            if (!has_cartesian)
            {
                Real sin_theta = sin(theta);
                x = sin_theta * cos(phi);
                y = sin_theta * sin(phi);
                z = cos(theta);
                //has_cartesian = true;
            }
        }
        
        bool has_cartesian;
    };
    
    struct ArcSphere
    {
        ArcSphere(int _cell_idx, int _height) : cell_idx(_cell_idx), height(_height), event(NULL), left_edge_idx(-1), right_edge_idx(-1) {}
        
        unsigned int cell_idx;
        
        int height;
        
        ArcSphere **prev, **next;
        
        CircleEventSphere * event;
        
        int left_edge_idx, right_edge_idx;
    };
    
    struct CircleEventSphere
    {
        CircleEventSphere(ArcSphere * a, PointSphere c, Real l) : arc(a), circumcenter(c), lowest_theta(l), is_valid(true) {}
        
        PointSphere circumcenter;
        
        bool is_valid;
        
        ArcSphere * arc;
        
        Real lowest_theta;
    };

    struct VoronoiCellSphere
    {
        VoronoiCellSphere(PointSphere point, int _idx) : site(point), cell_idx(_idx) {}
        
        VoronoiCellSphere(PointCartesian point, int _idx) : site(point), cell_idx(_idx) {}
        
        PointSphere site;
        
        unsigned int cell_idx;
        
        std::vector<int> edge_ids;
    };
    
    struct HalfEdgeSphere
    {
        HalfEdgeSphere(int vidx) : start_idx(vidx), is_finished(false) {}
        
        int start_idx, end_idx;
        
        bool is_finished;
    };
    
    /*
     *  Sort by theta. Smallest theta should be first.
     */
    struct PriorityQueueCompare
    {
        bool operator()(VoronoiCellSphere & left, VoronoiCellSphere & right) {return left.site > right.site;}
        bool operator()(CircleEventSphere * left, CircleEventSphere * right) {return left->lowest_theta > right->lowest_theta;}
    };
    
    VoronoiDiagramSphere generate_voronoi_one_thread(std::vector<std::tuple<Real, Real, Real>> * verts, void (*render)(VoronoiDiagramSphere, ArcSphere *, std::vector<VoronoiCellSphere> *, Real) = NULL, bool (*is_sleeping)() = NULL);
    
    VoronoiDiagramSphere generate_voronoi_two_threads(std::vector<std::tuple<Real, Real, Real>> * verts);
    
    VoronoiDiagramSphere generate_voronoi_four_threads(std::vector<std::tuple<Real, Real, Real>> * verts);
        
    void compute_priority_queues(VoronoiDiagramSphere * voronoi_diagram, std::vector<std::tuple<Real, Real, Real>> * verts, Real bound_theta);
    
    void handle_site_event(VoronoiCellSphere cell, VoronoiDiagramSphere * voronoi_diagram, std::vector<VoronoiCellSphere> * cells, std::vector<HalfEdgeSphere> * half_edges, std::priority_queue<CircleEventSphere *, std::vector<CircleEventSphere *>, PriorityQueueCompare> * circle_event_queue_ptr, ArcSphere * & beach_head, Real sweep_line, Real sin_sweep_line, Real cos_sweep_line);
    
    void handle_circle_event(CircleEventSphere * event, VoronoiDiagramSphere * voronoi_diagram, std::vector<VoronoiCellSphere> * cells, std::vector<HalfEdgeSphere> * half_edges, std::priority_queue<CircleEventSphere *, std::vector<CircleEventSphere *>, PriorityQueueCompare> * circle_event_queue_ptr, ArcSphere * & beach_head);
    
    bool parabolic_intersection(PointSphere left, PointSphere right, Real & phi_intersection, ArcSphere * & beach_head, Real sweep_line, Real sin_sweep_line, Real cos_sweep_line);
    
    inline PointSphere phi_to_point(PointSphere arc, Real phi, Real sweep_line, Real sin_sweep_line, Real cos_sweep_line);
    
    void check_circle_event(ArcSphere * arc, std::priority_queue<CircleEventSphere *, std::vector<CircleEventSphere *>, PriorityQueueCompare> * circle_event_queue_ptr, std::vector<VoronoiCellSphere> * cells);
    
    inline void make_circle(PointSphere a, PointSphere b, PointSphere c, PointSphere & circumcenter, Real & lowest_theta);
    
    void add_half_edge_sphere(VoronoiDiagramSphere * voronoi_diagram, std::vector<VoronoiCellSphere> * cells, std::vector<HalfEdgeSphere> * half_edges, PointCartesian start, ArcSphere * left, ArcSphere * right);
    
    void finish_half_edge_sphere(VoronoiDiagramSphere * voronoi_diagram, std::vector<HalfEdgeSphere> * half_edges, int edge_idx, PointCartesian end);
    
    void add_initial_arc_sphere(int cell_id, ArcSphere * & beach_head);
    
    void add_arc_sphere(int cell_id, ArcSphere * left, ArcSphere * right, ArcSphere * & beach_head);
    
    int random_height();
    
    void remove_arc_sphere(ArcSphere * arc, ArcSphere * & beach_head);
    
    ArcSphere * traverse_skiplist_to_site(ArcSphere * arc, Real phi, std::vector<VoronoiCellSphere> * cells);
    
    inline std::tuple<Real, Real, Real> rotate_y(std::tuple<Real, Real, Real> point, Real sin_theta, Real cos_theta);
    
    inline std::tuple<Real, Real, Real> rotate_z(std::tuple<Real, Real, Real> point, Real sin_theta, Real cos_theta);
}

#endif /* VoronoiSphere_h */