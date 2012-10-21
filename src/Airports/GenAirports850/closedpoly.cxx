#include <stdlib.h>
#include <list>

#include <simgear/debug/logstream.hxx>
#include <simgear/math/sg_geodesy.hxx>

#include <Polygon/polygon.hxx>
#include <Polygon/chop.hxx>
#include <Geometry/poly_support.hxx>

#include "global.hxx"
#include "beznode.hxx"
#include "closedpoly.hxx"

#define NO_BEZIER       (0)

static void stringPurifier( string& s )
{
    for ( string::iterator it = s.begin(), itEnd = s.end(); it!=itEnd; ++it) {
        if ( static_cast<unsigned int>(*it) < 32 || static_cast<unsigned int>(*it) > 127 ) {
            (*it) = ' ';
        }
    }
}

ClosedPoly::ClosedPoly( char* desc )
{
    is_pavement = false;

    if ( desc )
    {
        description = desc;
        stringPurifier(description);
    }
    else
    {
        description = "none";
    }
        
    boundary = NULL;
    cur_contour = NULL;
    cur_feature = NULL;
}

ClosedPoly::ClosedPoly( int st, float s, float th, char* desc )
{
    surface_type = st;
    smoothness   = s;
    texture_heading = th;
    is_pavement = true;

    if ( desc )
    {
        description = desc;
        stringPurifier(description);
    }
    else
    {
        description = "none";
    }
        
    boundary = NULL;
    cur_contour = NULL;
    cur_feature = NULL;
}

ClosedPoly::~ClosedPoly()
{
    SG_LOG( SG_GENERAL, SG_DEBUG, "Deleting ClosedPoly " << description );
}

void ClosedPoly::AddNode( BezNode* node )
{
    // if this is the first node of the contour - create a new contour
    if (!cur_contour)
    {
        cur_contour = new BezContour;
    }
    cur_contour->push_back( node );

    SG_LOG(SG_GENERAL, SG_DEBUG, "CLOSEDPOLY::ADDNODE : " << node->GetLoc() );

    // For pavement polys, add a linear feature for each contour
    if (is_pavement)
    {
        if (!cur_feature)
        {
            string feature_desc = description + " - ";
            if (boundary)
            {
                feature_desc += "hole";
            }
            else
            {
                feature_desc += "boundary";
            }

            SG_LOG(SG_GENERAL, SG_DEBUG, "   Adding node " << node->GetLoc() << " to current linear feature " << cur_feature);
            cur_feature = new LinearFeature(feature_desc, 1.0f );
        } 
        cur_feature->AddNode( node );
    }
}

void ClosedPoly::CloseCurContour()
{
    SG_LOG(SG_GENERAL, SG_DEBUG, "Close Contour");

    // if we are recording a pavement marking - it must be closed - 
    // add the first node of the poly
    if (cur_feature)
    {
        SG_LOG(SG_GENERAL, SG_DEBUG, "We still have an active linear feature - add the first node to close it");
        cur_feature->Finish(true, features.size() );

        features.push_back(cur_feature);
        cur_feature = NULL;        
    }

    // add the contour to the poly - first one is the outer boundary
    // subsequent contours are holes
    if ( boundary == NULL )
    {
        boundary = cur_contour;

        // generate the convex hull from the bezcontour node locations
        // CreateConvexHull();

        cur_contour = NULL;
    }
    else
    {
        holes.push_back( cur_contour );
        cur_contour = NULL;
    }
}

void ClosedPoly::ConvertContour( BezContour* src, point_list *dst )
{
    BezNode*    curNode;
    BezNode*    nextNode;

    SGGeod curLoc;
    SGGeod nextLoc;
    SGGeod cp1;
    SGGeod cp2;

    int       curve_type = CURVE_LINEAR;
    double    total_dist;
    int       num_segs = BEZIER_DETAIL;

    SG_LOG(SG_GENERAL, SG_DEBUG, "Creating a contour with " << src->size() << " nodes");

    // clear anything in this point list
    dst->empty();

    // iterate through each bezier node in the contour
    for (unsigned int i = 0; i <= src->size()-1; i++)
    {
        SG_LOG(SG_GENERAL, SG_DEBUG, "\nHandling Node " << i << "\n\n");

        curNode = src->at(i);
        if (i < src->size() - 1)
        {
            nextNode = src->at(i+1);
        }
        else
        {
            // for the last node, next is the first. as all contours are closed
            nextNode = src->at(0);
        }

        // now determine how we will iterate from current node to next node 
        if( curNode->HasNextCp() )
        {
            // next curve is cubic or quadratic
            if( nextNode->HasPrevCp() )
            {
                // curve is cubic : need both control points
                curve_type = CURVE_CUBIC;
                cp1 = curNode->GetNextCp();
                cp2 = nextNode->GetPrevCp();
                total_dist = CubicDistance( curNode->GetLoc(), cp1, cp2, nextNode->GetLoc() );
            }
            else
            {
                // curve is quadratic using current nodes cp as the cp
                curve_type = CURVE_QUADRATIC;
                cp1 = curNode->GetNextCp();
                total_dist = QuadraticDistance( curNode->GetLoc(), cp1, nextNode->GetLoc() );
            }
        }
        else
        {
            // next curve is quadratic or linear
            if( nextNode->HasPrevCp() )
            {
                // curve is quadratic using next nodes cp as the cp
                curve_type = CURVE_QUADRATIC;
                cp1 = nextNode->GetPrevCp();
                total_dist = QuadraticDistance( curNode->GetLoc(), cp1, nextNode->GetLoc() );
            }
            else
            {
                // curve is linear
                curve_type = CURVE_LINEAR;
                total_dist = LinearDistance( curNode->GetLoc(), nextNode->GetLoc() );
            }
        }

        if (total_dist < 8.0f)
        {
            if (curve_type != CURVE_LINEAR)
            {
                // If total distance is < 4 meters, then we need to modify num Segments so that each segment >= 2 meters
                num_segs = ((int)total_dist + 1);
                SG_LOG(SG_GENERAL, SG_DEBUG, "Segment from " << curNode->GetLoc() << " to " << nextNode->GetLoc() );
                SG_LOG(SG_GENERAL, SG_DEBUG, "        Distance is " << total_dist << " ( < 16.0) so num_segs is " << num_segs );
            }
            else
            {
                num_segs = 1;
            }
        }
        else if (total_dist > 800.0f)
        {
            // If total distance is > 800 meters, then we need to modify num Segments so that each segment <= 100 meters
            num_segs = total_dist / 100.0f + 1;
            SG_LOG(SG_GENERAL, SG_DEBUG, "Segment from " << curNode->GetLoc() << " to " << nextNode->GetLoc() );
            SG_LOG(SG_GENERAL, SG_DEBUG, "        Distance is " << total_dist << " ( > 100.0) so num_segs is " << num_segs );
        }
        else
        {
            if (curve_type != CURVE_LINEAR)
            {            
                num_segs = 8;
                SG_LOG(SG_GENERAL, SG_DEBUG, "Segment from " << curNode->GetLoc() << " to " << nextNode->GetLoc() );
                SG_LOG(SG_GENERAL, SG_DEBUG, "        Distance is " << total_dist << " (OK) so num_segs is " << num_segs );
            }
            else
            {
                // make sure linear segments don't got over 100m
                num_segs = total_dist / 100.0f + 1;
            }
        }

#if NO_BEZIER
        num_segs = 1;
#endif

        // if only one segment, revert to linear
        if (num_segs == 1)
        {
            curve_type = CURVE_LINEAR;
        }

        // initialize current location
        curLoc = curNode->GetLoc();
        if (curve_type != CURVE_LINEAR)
        {
            for (int p=0; p<num_segs; p++)
            {
                // calculate next location
                if (curve_type == CURVE_QUADRATIC)
                {
                    nextLoc = CalculateQuadraticLocation( curNode->GetLoc(), cp1, nextNode->GetLoc(), (1.0f/num_segs) * (p+1) );
                }
                else
                {
                    nextLoc = CalculateCubicLocation( curNode->GetLoc(), cp1, cp2, nextNode->GetLoc(), (1.0f/num_segs) * (p+1) );
                }

                // add the pavement vertex
                // convert from lat/lon to geo
                // (maybe later) - check some simgear objects...
                dst->push_back( Point3D::fromSGGeod(curLoc) );

                if (p==0)
                {
                    SG_LOG(SG_GENERAL, SG_DEBUG, "adding Curve Anchor node (type " << curve_type << ") at " << curLoc );
                }
                else
                {
                    SG_LOG(SG_GENERAL, SG_DEBUG, "   add bezier node (type  " << curve_type << ") at " << curLoc );
                }

                // now set set cur location for the next iteration
                curLoc = nextLoc;
            }
        }
        else
        {
            if (num_segs > 1)
            {
                for (int p=0; p<num_segs; p++)
                {
                    // calculate next location
                    nextLoc = CalculateLinearLocation( curNode->GetLoc(), nextNode->GetLoc(), (1.0f/num_segs) * (p+1) );                    

                    // add the feature vertex
                    dst->push_back( Point3D::fromSGGeod(curLoc) );

                    if (p==0)
                    {
                        SG_LOG(SG_GENERAL, SG_DEBUG, "adding Linear anchor node at " << curLoc );
                    }
                    else
                    {
                        SG_LOG(SG_GENERAL, SG_DEBUG, "   add linear node at " << curLoc );
                    }

                    // now set set prev and cur locations for the next iteration
                    curLoc = nextLoc;
                }
            }
            else
            {
                nextLoc = nextNode->GetLoc();

                // just add the one vertex - dist is small
                dst->push_back( Point3D::fromSGGeod(curLoc) );

                SG_LOG(SG_GENERAL, SG_DEBUG, "adding Linear Anchor node at " << curLoc );

                curLoc = nextLoc;
            }
        }
    }
}

// finish the poly - convert to TGPolygon, and tesselate
void ClosedPoly::Finish()
{
    point_list          dst_contour;

    // error handling
    if (boundary == NULL)
    {
        SG_LOG(SG_GENERAL, SG_ALERT, "no boundary");
    }

    SG_LOG(SG_GENERAL, SG_DEBUG, "Converting a poly with " << holes.size() << " holes");
    
    if (boundary != NULL)
    {
        // create the boundary
        ConvertContour( boundary, &dst_contour );
    
        // and add it to the geometry 
        pre_tess.add_contour( dst_contour, 0 );

        // Then convert the hole contours
        for (unsigned int i=0; i<holes.size(); i++)
        {
            dst_contour.clear();
            ConvertContour( holes[i], &dst_contour );
            pre_tess.add_contour( dst_contour, 1 );
        }

        pre_tess = snap( pre_tess, gSnap );
        pre_tess = remove_dups( pre_tess );
    }

    // save memory by deleting unneeded resources
    for (unsigned int i=0; i<boundary->size(); i++)
    {
        delete boundary->at(i);
    }
    delete boundary;
    boundary = NULL;

    // and the hole contours
    for (unsigned int i=0; i<holes.size(); i++)
    {
        for (unsigned int j=0; j<holes[i]->size(); j++)
        {
            delete holes[i]->at(j);
        }
    }
    holes.clear();
}

int ClosedPoly::BuildBtg( superpoly_list* rwy_polys, texparams_list* texparams, poly_list& slivers, TGPolygon* apt_base, TGPolygon* apt_clearing, bool make_shapefiles )
{
    TGPolygon base, safe_base;

    string    material;
    void*     ds_id = NULL;        // If we are going to build shapefiles
    void*     l_id  = NULL;        // datasource and layer IDs

    if ( make_shapefiles ) {
        char ds_name[128];
        sprintf(ds_name, "./cp_debug");
        ds_id = tgShapefileOpenDatasource( ds_name );
    }

    if (is_pavement)
    {
        switch( surface_type )
        {
            case 1:
                material = "pa_tiedown";
                break;

            case 2:
                material = "pc_tiedown";
                break;

            case 3:
                material = "grass_rwy";
                break;
            
            // TODO Differentiate more here:
            case 4:
            case 5:
            case 12:
            case 13:
            case 14:
            case 15:
                material = "grass_rwy";
                break;

            default:
                SG_LOG(SG_GENERAL, SG_ALERT, "ClosedPoly::BuildBtg: unknown surface type " << surface_type );
                exit(1);
        }

        // verify the poly has been generated
        if ( pre_tess.contours() )    
        {
            SG_LOG(SG_GENERAL, SG_DEBUG, "BuildBtg: original poly has " << pre_tess.contours() << " contours" << " and " << pre_tess.total_size() << " points" );

            // do this before clipping and generating the base
            // pre_tess = tgPolygonSimplify( pre_tess );
            // pre_tess = reduce_degeneracy( pre_tess );
    
            TGSuperPoly sp;
            TGTexParams tp;

            if ( make_shapefiles ) {
                char layer_name[128];
                char feature_name[128];

                sprintf( layer_name, "original" );
                l_id = tgShapefileOpenLayer( ds_id, layer_name );
                sprintf( feature_name, "original" );
                tgShapefileCreateFeature( ds_id, l_id, pre_tess, feature_name );
            }
    
            TGPolygon clipped = tgPolygonDiffClipperWithAccumulator( pre_tess );
            tgPolygonFindSlivers( clipped, slivers );

            SG_LOG(SG_GENERAL, SG_DEBUG, "clipped = " << clipped.contours());

            sp.erase();
            sp.set_poly( clipped );
            sp.set_material( material );
            //sp.set_flag("taxi");

            rwy_polys->push_back( sp );

            tgPolygonAddToClipperAccumulator( pre_tess, false );

            /* If debugging this poly, write the poly, and clipped poly and the accum buffer into their own layers */
            if ( make_shapefiles ) {
                char layer_name[128];
                char feature_name[128];

                sprintf( layer_name, "clipped" );
                l_id = tgShapefileOpenLayer( ds_id, layer_name );
                sprintf( feature_name, "clipped" );
                tgShapefileCreateFeature( ds_id, l_id, clipped, feature_name );
            }

            SG_LOG(SG_GENERAL, SG_DEBUG, "tp construct");

            tp = TGTexParams( pre_tess.get_pt(0,0).toSGGeod(), 5.0, 5.0, texture_heading );
            texparams->push_back( tp );

            if ( apt_base )
            {           
                base = tgPolygonExpand( pre_tess, 20.0); 
                if ( make_shapefiles ) {
                    char layer_name[128];
                    char feature_name[128];

                    sprintf( layer_name, "exp_base" );
                    l_id = tgShapefileOpenLayer( ds_id, layer_name );
                    sprintf( feature_name, "exp_base" );
                    tgShapefileCreateFeature( ds_id, l_id, base, feature_name );
                }

                safe_base = tgPolygonExpand( pre_tess, 50.0);        
                if ( make_shapefiles ) {
                    char layer_name[128];
                    char feature_name[128];

                    SG_LOG(SG_GENERAL, SG_INFO, "expanded safe poly: " << safe_base);

                    sprintf( layer_name, "exp_safe_base" );
                    l_id = tgShapefileOpenLayer( ds_id, layer_name );
                    sprintf( feature_name, "exp_safe_base" );
                    tgShapefileCreateFeature( ds_id, l_id, safe_base, feature_name );
                }                
    
                // add this to the airport clearing
                *apt_clearing = tgPolygonUnionClipper( safe_base, *apt_clearing);

                // and add the clearing to the base
                *apt_base = tgPolygonUnionClipper( base, *apt_base );
            }

            if ( make_shapefiles )
            {
                tgShapefileCloseDatasource( ds_id );
            }
        }
    }

    // clean up to save ram : we're done here...
    return 1;
}

// Just used for user defined border - add a little bit, as some modelers made the border exactly on the edges 
// - resulting in no base, which we can't handle
int ClosedPoly::BuildBtg( TGPolygon* apt_base, TGPolygon* apt_clearing, bool make_shapefiles )
{
    TGPolygon base, safe_base;

    // verify the poly has been generated
    if ( pre_tess.contours() )
    {
        SG_LOG(SG_GENERAL, SG_DEBUG, "BuildBtg: original poly has " << pre_tess.contours() << " contours");
    
        base = tgPolygonExpand( pre_tess, 2.0); 
        safe_base = tgPolygonExpand( pre_tess, 5.0);        
        
        // add this to the airport clearing
        *apt_clearing = tgPolygonUnionClipper( safe_base, *apt_clearing);

        // and add the clearing to the base
        *apt_base = tgPolygonUnionClipper( base, *apt_base );
    }

    return 1;
}
