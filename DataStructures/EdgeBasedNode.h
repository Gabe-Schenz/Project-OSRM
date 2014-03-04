#ifndef EDGE_BASED_NODE_H
#define EDGE_BASED_NODE_H

#include "../Util/MercatorUtil.h"
#include "../Util/SimpleLogger.h"
#include "../typedefs.h"

#include <osrm/Coordinate.h>

#include <boost/assert.hpp>

#include <limits>

struct EdgeBasedNode {

    EdgeBasedNode() :
        forward_edge_based_node_id(SPECIAL_NODEID),
        reverse_edge_based_node_id(SPECIAL_NODEID),
        u(SPECIAL_NODEID),
        v(SPECIAL_NODEID),
        name_id(0),
        forward_weight(std::numeric_limits<int>::max() >> 1),
        reverse_weight(std::numeric_limits<int>::max() >> 1),
        forward_offset(0),
        reverse_offset(0),
        fwd_segment_position( std::numeric_limits<unsigned short>::max() ),
        rev_segment_position( std::numeric_limits<unsigned short>::max() >> 1 ),
        belongsToTinyComponent(false)
    { }

    EdgeBasedNode(
        NodeID forward_edge_based_node_id,
        NodeID reverse_edge_based_node_id,
        NodeID u,
        NodeID v,
        unsigned name_id,
        int forward_weight,
        int reverse_weight,
        int forward_offset,
        int reverse_offset,
        unsigned short fwd_segment_position,
        unsigned short rev_segment_position,
        bool belongsToTinyComponent
    ) :
        forward_edge_based_node_id(forward_edge_based_node_id),
        reverse_edge_based_node_id(reverse_edge_based_node_id),
        u(u),
        v(v),
        name_id(name_id),
        forward_weight(forward_weight),
        reverse_weight(reverse_weight),
        forward_offset(forward_offset),
        reverse_offset(reverse_offset),
        fwd_segment_position(fwd_segment_position),
        rev_segment_position(rev_segment_position),
        belongsToTinyComponent(belongsToTinyComponent)
    { }

    inline static double ComputePerpendicularDistance(
        const FixedPointCoordinate & coord_a,
        const FixedPointCoordinate & coord_b,
        const FixedPointCoordinate & query_location,
        FixedPointCoordinate & nearest_location,
        double & r
    ) {
        BOOST_ASSERT( query_location.isValid() );

        const double x = lat2y(query_location.lat/COORDINATE_PRECISION);
        const double y = query_location.lon/COORDINATE_PRECISION;
        const double a = lat2y(coord_a.lat/COORDINATE_PRECISION);
        const double b = coord_a.lon/COORDINATE_PRECISION;
        const double c = lat2y(coord_b.lat/COORDINATE_PRECISION);
        const double d = coord_b.lon/COORDINATE_PRECISION;
        double p,q/*,mX*/,nY;
        if( std::abs(a-c) > std::numeric_limits<double>::epsilon() ){
            const double m = (d-b)/(c-a); // slope
            // Projection of (x,y) on line joining (a,b) and (c,d)
            p = ((x + (m*y)) + (m*m*a - m*b))/(1. + m*m);
            q = b + m*(p - a);
        } else {
            p = c;
            q = y;
        }
        nY = (d*p - c*q)/(a*d - b*c);

        //discretize the result to coordinate precision. it's a hack!
        if( std::abs(nY) < (1./COORDINATE_PRECISION) ) {
            nY = 0.;
        }

        r = (p - nY*a)/c;// These values are actually n/m+n and m/m+n , we need
        // not calculate the explicit values of m an n as we
        // are just interested in the ratio
        if( std::isnan(r) ) {
            r = ((coord_b.lat == query_location.lat) && (coord_b.lon == query_location.lon)) ? 1. : 0.;
        } else if( std::abs(r) <= std::numeric_limits<double>::epsilon() ) {
            r = 0.;
        } else if( std::abs(r-1.) <= std::numeric_limits<double>::epsilon() ) {
            r = 1.;
        }
        BOOST_ASSERT( !std::isnan(r) );
        if( r <= 0. ){
            nearest_location.lat = coord_a.lat;
            nearest_location.lon = coord_a.lon;
        } else if( r >= 1. ){
            nearest_location.lat = coord_b.lat;
            nearest_location.lon = coord_b.lon;
        } else {
            // point lies in between
            nearest_location.lat = y2lat(p)*COORDINATE_PRECISION;
            nearest_location.lon = q*COORDINATE_PRECISION;
        }
        BOOST_ASSERT( nearest_location.isValid() );

        // TODO: Replace with euclidean approximation when k-NN search is done
        // const double approximated_distance = FixedPointCoordinate::ApproximateEuclideanDistance(
        const double approximated_distance = FixedPointCoordinate::ApproximateDistance(
            query_location,
            nearest_location
        );
        BOOST_ASSERT( 0. <= approximated_distance );
        return approximated_distance;
    }

    static inline FixedPointCoordinate Centroid(
        const FixedPointCoordinate & a,
        const FixedPointCoordinate & b
    ) {
        FixedPointCoordinate centroid;
        //The coordinates of the midpoint are given by:
        //x = (x1 + x2) /2 and y = (y1 + y2) /2.
        centroid.lon = (std::min(a.lon, b.lon) + std::max(a.lon, b.lon))/2;
        centroid.lat = (std::min(a.lat, b.lat) + std::max(a.lat, b.lat))/2;
        return centroid;
    }

    bool IsCompressed() {
        return (fwd_segment_position + rev_segment_position) != 0;
    }

    NodeID forward_edge_based_node_id;
    NodeID reverse_edge_based_node_id;
    NodeID u;
    NodeID v;
    unsigned name_id;
    int forward_weight;
    int reverse_weight;
    int forward_offset;
    int reverse_offset;
    unsigned short fwd_segment_position;
    unsigned short rev_segment_position:15;
    bool belongsToTinyComponent:1;
};

#endif //EDGE_BASED_NODE_H
