// TGLandclass.hxx -- Class toSimnplify dealing with shape heiarchy:
//                    landclass contains each area (layer) of a tile
//                    Each area is a list of shapes
//                    A shape has 1 or more segments
//                    (when the shape represents line data)
//                    And the segment is a superpoly, containing
//                      - a polygon, triangulation, point normals, face normals, etc.
//
// Written by Curtis Olson, started May 1999.
//
// Copyright (C) 1999  Curtis L. Olson  - http://www.flightgear.org/~curt
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License as
// published by the Free Software Foundation; either version 2 of the
// License, or (at your option) any later version.
//
// This program is distributed in the hope that it will be useful, but
// WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
// General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program; if not, write to the Free Software
// Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301, USA.
//
// $Id: construct.hxx,v 1.13 2004-11-19 22:25:49 curt Exp $

#ifndef _TGLANDCLASS_HXX
#define _TGLANDCLASS_HXX

#ifndef __cplusplus
# error This library requires C++
#endif

#define TG_MAX_AREA_TYPES       128

#include "tgshape.hxx"

class TGLandclass
{
public:
    void clear(void);

    inline unsigned int area_size( unsigned int area )
    {
        return shapes[area].size();
    }
    inline unsigned int shape_size( unsigned int area, unsigned int shape )
    {
        return shapes[area][shape].sps.size();
    }

    inline void add_shape( unsigned int area, TGShape shape )
    {
        shapes[area].push_back( shape );
    }
    inline TGShape& get_shape( unsigned int area, unsigned int shape )
    {
        return shapes[area][shape];
    }

    inline TGPolygon get_mask( unsigned int area, unsigned int shape )
    {
        return shapes[area][shape].clip_mask;
    }
    inline void set_mask( unsigned int area, unsigned int shape, TGPolygon mask )
    {
        shapes[area][shape].clip_mask = mask;
    }

    inline bool get_textured( unsigned int area, unsigned int shape )
    {
        return shapes[area][shape].textured;
    }
    inline void set_textured( unsigned int area, unsigned int shape, bool t )
    {
        shapes[area][shape].textured = t;
    }

    inline TGSuperPoly& get_superpoly( unsigned int area, unsigned int shape, unsigned int segment )
    {
        return shapes[area][shape].sps[segment];
    }
    inline void set_superpoly( unsigned int area, unsigned int shape, unsigned int segment, TGSuperPoly sp )
    {
        shapes[area][shape].sps[segment] = sp;
    }

    inline TGPolygon get_poly( unsigned int area, unsigned int shape, unsigned int segment )
    {
        return shapes[area][shape].sps[segment].get_poly();
    }
    inline void set_poly( unsigned int area, unsigned int shape, unsigned int segment, TGPolygon poly )
    {
        return shapes[area][shape].sps[segment].set_poly( poly );
    }

    inline TGPolygon get_tris( unsigned int area, unsigned int shape, unsigned int segment )
    {
        return shapes[area][shape].sps[segment].get_tris();
    }
    inline void set_tris( unsigned int area, unsigned int shape, unsigned int segment, TGPolygon tris )
    {
        shapes[area][shape].sps[segment].set_tris( tris );
    }

    inline Point3D get_face_normal( unsigned int area, unsigned int shape, unsigned int segment, unsigned int tri )
    {
        return shapes[area][shape].sps[segment].get_face_normal( tri );
    }

    inline double get_face_area( unsigned int area, unsigned int shape, unsigned int segment, unsigned int tri )
    {
        return shapes[area][shape].sps[segment].get_face_area( tri );
    }

    inline std::string get_flag( unsigned int area, unsigned int shape, unsigned int segment )
    {
        return shapes[area][shape].sps[segment].get_flag();
    }

    inline std::string get_material( unsigned int area, unsigned int shape, unsigned int segment )
    {
        return shapes[area][shape].sps[segment].get_material();
    }
    inline TGTexParams& get_texparams( unsigned int area, unsigned int shape, unsigned int segment )
    {
        return shapes[area][shape].tps[segment];
    }


    inline TGPolygon get_texcoords( unsigned int area, unsigned int shape, unsigned int segment )
    {
        return shapes[area][shape].sps[segment].get_texcoords();
    }
    inline void set_texcoords( unsigned int area, unsigned int shape, unsigned int segment, TGPolygon tcs )
    {
        return shapes[area][shape].sps[segment].set_texcoords( tcs );
    }

    inline TGPolyNodes get_tri_idxs( unsigned int area, unsigned int shape, unsigned int segment )
    {
        return shapes[area][shape].sps[segment].get_tri_idxs();
    }
    inline void set_tri_idxs( unsigned int area, unsigned int shape, unsigned int segment, TGPolyNodes tis )
    {
        return shapes[area][shape].sps[segment].set_tri_idxs( tis );
    }

    // Friends for serialization
    friend std::istream& operator>> ( std::istream&, TGLandclass& );
    friend std::ostream& operator<< ( std::ostream&, const TGLandclass& );

private:
    shape_list     shapes[TG_MAX_AREA_TYPES];
};

#endif // _TGLANDCLASS_HXX