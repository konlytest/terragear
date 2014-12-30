// main.cxx -- top level construction routines
//
// Written by Peter Sadrozinski, started Dec 2014.
//
// Copyright (C) 2014  Curtis L. Olson  - http://www.flightgear.org/~curt
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
#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif

#ifdef _MSC_VER
#  include <windows.h>
#endif

#include <stdio.h>


#include <CGAL/Simple_cartesian.h>
#include <CGAL/Polyhedron_incremental_builder_3.h>
#include <CGAL/Polyhedron_3.h>

#include <simgear/bucket/newbucket.hxx>
#include <simgear/misc/sg_path.hxx>
#include <simgear/io/sg_binobj.hxx>
#include <simgear/debug/logstream.hxx>

#include <terragear/BucketBox.hxx>
#include <terragear/tg_shapefile.hxx>
#include <Include/version.h>


// Very preliminary BTG -> CGAL Mesh converted
// to be used to simplify meshes from High detail to low detail.
// as a side benefit - I see it spots mesh degeneracy in current BTG files...
// need to be able to save the transformed mesh back to a BTG - 
// and maybe highlight the rejected triangles ( with a bright red material )
// to debug...

template <class HDS>
class Build_BTG_Mesh : public CGAL::Modifier_base<HDS> {
public:
    Build_BTG_Mesh(const SGBinObject& o) { obj = o; }
    void operator()( HDS& hds) {
        int num_vertices = obj.get_wgs84_nodes().size();
        int num_indices  = 0;
        
        // just read in triangles. ignoring fans and strips
        int num_groups = obj.get_tris_v().size();
        for ( int g=0; g<num_groups; g++ ) {
            num_indices += obj.get_tris_v()[g].size();
        }
        
        // Postcondition: hds is a valid polyhedral surface.
        CGAL::Polyhedron_incremental_builder_3<HDS> B( hds, true);
        B.begin_surface( num_vertices, num_indices/3, 0);
        typedef typename HDS::Vertex   Vertex;
        typedef typename Vertex::Point Point;
        
        for ( int v=0; v<num_vertices; v++ ) {
            SGVec3d sgn = obj.get_wgs84_nodes()[v];
            B.add_vertex( Point( sgn.x(), sgn.y(), sgn.z() ) );
        }

        for ( int grp=0; grp<num_groups; grp++ ) {
            const int_list& tris_v(obj.get_tris_v()[grp]);
            for (unsigned i = 2; i < tris_v.size(); i += 3) {
                std::vector< std::size_t> indices;
                indices.push_back( tris_v[i-2] );
                indices.push_back( tris_v[i-1] );
                indices.push_back( tris_v[i-0] );

                if ( B.test_facet( indices.begin(), indices.end() ) ) {
                    B.add_facet( indices.begin(), indices.end() );
                } else {
                    SG_LOG(SG_GENERAL, SG_ALERT, "Couldn't add triangle w/indices " << indices[0] << ", " << indices[1] << ", " <<  indices[2] );
                }
            }
        }
        B.end_surface();
    }
private:
    SGBinObject obj;
};

typedef CGAL::Simple_cartesian<double>     Kernel;
typedef CGAL::Polyhedron_3<Kernel>         Polyhedron;
typedef Polyhedron::HalfedgeDS             HalfedgeDS;

// display usage and exit
static void usage( const std::string name ) {
    SG_LOG(SG_GENERAL, SG_ALERT, "Usage: " << name);
    exit(-1);
}

// usage tglod minx, miny, maxx, maxy, level input_dir output_dir
// 

// first test : malta - generate 2 level 8 ( 0.25 x 0.25 ) tiles
//              14.00,35.75 - 14.25,36.00
//              14.25,35.75 - 14.50,36.00

int main(int argc, char **argv) 
{
    std::string output_dir = ".";
    std::string work_dir = ".";
    
    // default = whole earth - hehe
    double min_lon = -180;
    double min_lat =  -90;
    double max_lon =  180;
    double max_lat =   90;
    int    level = 0;
    
    sglog().setLogLevels( SG_ALL, SG_INFO );

    //
    // Parse the command-line arguments.
    //
    int arg_pos;
    for (arg_pos = 1; arg_pos < argc; arg_pos++) {
        std::string arg = argv[arg_pos];

        if (arg.find("--output-dir=") == 0) {
            output_dir = arg.substr(13);
        } else if (arg.find("--work-dir=") == 0) {
            work_dir = arg.substr(11);
        } else if ( arg.find("--min-lon=") == 0 ) {
            min_lon = atof( arg.substr(10).c_str() );
        } else if ( arg.find("--max-lon=") == 0 ) {
            max_lon = atof( arg.substr(10).c_str() );
        } else if ( arg.find("--min-lat=") == 0 ) {
            min_lat = atof( arg.substr(10).c_str() );
        } else if ( arg.find("--max-lat=") == 0 ) {
            max_lat = atof( arg.substr(10).c_str() );
        } else if (arg.find("--level=") == 0) {
            level = atoi( arg.substr(8).c_str() );
        } else {
            SG_LOG( SG_GENERAL, SG_ALERT, "unknown param " << arg );
        }
    }
    
    
    double width  = max_lon - min_lon;
    double height = max_lat - min_lat;

    // generate a bucketbox covering the area
    BucketBox box( min_lon, min_lat, width, height );
    SG_LOG( SG_GENERAL, SG_ALERT, "Box is: " << box << " level is " << box.getStartLevel() );
    
    if ( box.getStartLevel() < 8 ) {
        // we are not in the bucket stage        
        BucketBox subdivide[32];    // Pretty sure the maximum number of sub tiles is 25 
                                    // when we traverse between level 1 and 2
                                    // level 2 is  36.000 x  12.000     
                                    // level 1 is 180.000 x  60.000 ( 25 level 2 tiles = 1 level 1 tile )
    
        // subdivide the box to get higher res mesh
        unsigned numTiles = box.getSubDivision(subdivide, 32);
    
        if ( numTiles ) {
            SG_LOG( SG_GENERAL, SG_ALERT, "subdivided box " << box << " into " << numTiles << " tiles " );
    
            for( unsigned int i=0; i<numTiles; i++ ) {
                // TODO : after subdividing - see if the mesh (.SPT format) is available 
                
                // if not - we need to generate it ( for now - recursively - I'd like to not respawn, however )
                char cmdline[256];
        
                SG_LOG( SG_GENERAL, SG_ALERT, "           box " << i << " : " << subdivide[i] << " level " << subdivide[i].getStartLevel() << " height is " << subdivide[i].getHeightDeg() );
                sprintf( cmdline, "/home/psadro/Development/terragear/release/src/BuildTiles/tglod/tg-lod --work-dir=%s --min-lon=%f --min-lat=%f --max-lon=%f --max-lat=%f", 
                        work_dir.c_str(),
                        subdivide[i].getLongitudeDeg(), 
                        subdivide[i].getLatitudeDeg(), 
                        subdivide[i].getLongitudeDeg() + subdivide[i].getWidthDeg(), 
                        subdivide[i].getLatitudeDeg()  + subdivide[i].getHeightDeg() );
                
                system( cmdline );
            }
        }
    } 
    else 
    {
        // we've reached the bucket stage - read in all of the bucket triangles to generate a simplified mesh
        BucketBox subdivide[32];
    
        // subdivide the box to get higher res mesh
        unsigned numTiles = box.getSubDivision(subdivide, 32);
    
        if ( numTiles ) {
            for( unsigned int i=0; i<numTiles; i++ ) {
                SGBucket    b = subdivide[i].getBucket();
                SGPath      file = work_dir + "/" + b.gen_base_path() + "/" + b.gen_index_str() + ".btg.gz";
                SGBinObject obj;
                
                SG_LOG( SG_GENERAL, SG_ALERT, "           read SG bucket " << file.str() );
                if ( obj.read_bin( file.str() ) ) {
                    SG_LOG( SG_GENERAL, SG_ALERT, "           SUCCESS READING obj" );

                    Polyhedron P;
                    Build_BTG_Mesh<HalfedgeDS> mesh(obj);
                    P.delegate( mesh);
                    
                    // now let's traverse the facets and create a new .btg
                    
                    // TODO: keep a triangle list of errors as well in Build_BTG_Mesh object
                    // - so new BTG can get bright red error triangles...
                }
            }
        }
        
    }
        
    return 0;
}
