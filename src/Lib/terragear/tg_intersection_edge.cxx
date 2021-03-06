#include <simgear/sg_inlines.h>

#include "tg_polygon.hxx"
#include "tg_shapefile.hxx" 
#include "tg_intersection_edge.hxx"
#include "tg_intersection_node.hxx"
#include "tg_misc.hxx"

// generate intersection edge in euclidean space
tgIntersectionEdge::tgIntersectionEdge( tgIntersectionNode* s, tgIntersectionNode* e, double w, unsigned int t, const std::string& dr ) 
{
    static unsigned int ge_count = 0;

    start = s;
    end   = e;
    width = w;
    type  = t;
    
    id = ++ge_count;
    flags = 0;
    
    br_set = false;
    tr_set = false;
    tl_set = false;
    bl_set = false;
    
    msbr_set = false;
    msbr_valid = false;
    
    mstr_set = false;
    mstr_valid = false;
    
    mstl_set = false;
    mstl_valid = false;

    msbl_set = false;
    msbl_valid = false;
    
    // we need to add this edge between start and end to handle multiple edges at a node
    s->AddEdge( true, this );
    e->AddEdge( false, this );
    
    double ecourse   = SGGeodesy::courseDeg(start->GetPosition(), end->GetPosition());
    double elcourse  = SGMiscd::normalizePeriodic(0, 360, ecourse - 90);
    
    botLeft   = SGGeodesy::direct( start->GetPosition(), elcourse,  width/2 );    
    botRight  = SGGeodesy::direct( start->GetPosition(), elcourse, -width/2 );

    topLeft   = SGGeodesy::direct( end->GetPosition(), elcourse,  width/2 );
    topRight  = SGGeodesy::direct( end->GetPosition(), elcourse, -width/2 );
    
    // make sides 1 bit longer...
    SGGeod side_bl = SGGeodesy::direct( botLeft, ecourse, -10.0 );
    SGGeod side_tl = SGGeodesy::direct( topLeft, ecourse,  10.0 );
    
    SGGeod side_br = SGGeodesy::direct( botRight, ecourse, -10.0 );
    SGGeod side_tr = SGGeodesy::direct( topRight, ecourse,  10.0 );
    
    side_l    = tgLine( side_bl, side_tl );
    side_r    = tgLine( side_br, side_tr );

    debugRoot = dr;
    sprintf(datasource, "./edge_dbg/%s/edge_%02ld/", debugRoot.c_str(), id );
}
    
double tgIntersectionEdge::GetHeading( bool originating ) const 
{
    if ( originating ) {
        return SGGeodesy::courseDeg( start->GetPosition(), end->GetPosition() );
    } else {
        return SGGeodesy::courseDeg( end->GetPosition(), start->GetPosition() );
    }
}
    
double tgIntersectionEdge::GetGeodesyLength( void ) const 
{
    return SGGeodesy::distanceM( start->GetPosition(), end->GetPosition() );
}
    
tgIntersectionEdge* tgIntersectionEdge::Split( bool originating, tgIntersectionNode* newNode )
{    
    // first - inform the ending node that we aren't associated anymore
    tgIntersectionNode* oldNode = NULL;
    tgIntersectionEdge* newEdge = NULL;
    
    if ( originating ) {
        oldNode = end;
        end = newNode;
    } else {
        oldNode = start;
        start = newNode;
    }

    oldNode->DelEdge( originating, this );
    newNode->AddEdge( !originating, this );

    // then update all the geometery info for the modified edge
    double ecourse   = SGGeodesy::courseDeg(start->GetPosition(), end->GetPosition());
    double elcourse  = SGMiscd::normalizePeriodic(0, 360, ecourse - 90);

    botLeft   = SGGeodesy::direct( start->GetPosition(), elcourse,  width/2 );    
    botRight  = SGGeodesy::direct( start->GetPosition(), elcourse, -width/2 );

    topLeft   = SGGeodesy::direct( end->GetPosition(), elcourse,  width/2 );
    topRight  = SGGeodesy::direct( end->GetPosition(), elcourse, -width/2 );

    side_l    = tgLine( botLeft,  topLeft );
    side_r    = tgLine( botRight, topRight );
    
    // now create the new edge with start == new end, end = oldEnd;
    if ( originating ) {
        newEdge = new tgIntersectionEdge( newNode, oldNode, width, type, debugRoot );
    } else {
        newEdge = new tgIntersectionEdge( oldNode, newNode, width, type, debugRoot );        
    }
    
    return newEdge;
}
    
tgRectangle tgIntersectionEdge::GetBoundingBox( void ) const
{
    SGGeod min, max;

    double minx =  SG_MIN2( start->GetPosition().getLongitudeDeg(), end->GetPosition().getLongitudeDeg() );
    double miny =  SG_MIN2( start->GetPosition().getLatitudeDeg(),  end->GetPosition().getLatitudeDeg() );
    double maxx =  SG_MAX2( start->GetPosition().getLongitudeDeg(), end->GetPosition().getLongitudeDeg() );
    double maxy =  SG_MAX2( start->GetPosition().getLatitudeDeg(),  end->GetPosition().getLatitudeDeg() );

    min = SGGeod::fromDeg( minx, miny );
    max = SGGeod::fromDeg( maxx, maxy );

    return tgRectangle( min, max );
}
    
void tgIntersectionEdge::ToShapefile( void ) const
{            
    char layer[128];
    
    // draw line from start to end
    tgSegment skel = tgSegment( start->GetPosition(), end->GetPosition() );
    sprintf( layer, "%ld_skeleton", id );
    tgShapefile::FromSegment( skel, true, datasource, layer, "edge" );
    
    // just write the array of constraints
    sprintf( layer, "%ld_ends", id );
    tgShapefile::FromRay( constrain_bl, datasource, layer, "botLeft" );
    tgShapefile::FromRay( constrain_br, datasource, layer, "botRight" );
    tgShapefile::FromRay( constrain_tr, datasource, layer, "topRight" );
    tgShapefile::FromRay( constrain_tl, datasource, layer, "topLeft" );
    
    sprintf( layer, "%ld_sides", id );
    tgShapefile::FromLine( side_l, datasource, layer, "left" );
    tgShapefile::FromLine( side_r, datasource, layer, "right" );
    
    // now any multiseg constraints
    sprintf( layer, "%ld_ms_corners", id );
    DumpConstraint( layer, "botLeft",  constrain_msbl );
    DumpConstraint( layer, "botRight", constrain_msbr );
    DumpConstraint( layer, "topRight", constrain_mstr );
    DumpConstraint( layer, "topLeft",  constrain_mstl );
    
    // then the single segment corners
    sprintf( layer, "%ld_ss_corners", id );
    tgShapefile::FromGeod( conBotLeft,  datasource, layer, "botLeft" );
    tgShapefile::FromGeod( conBotRight, datasource, layer, "botRight" );
    tgShapefile::FromGeod( conTopRight, datasource, layer, "topRight" );
    tgShapefile::FromGeod( conTopLeft,  datasource, layer, "topLeft" );
    
    // finally, the final contours
    sprintf( layer, "%ld_complete", id );
    DumpConstraint( layer, "right", right_contour );
    DumpConstraint( layer, "left",  left_contour );
}

tgSegment tgIntersectionEdge::ToSegment( void ) const
{            
    return tgSegment( start->GetPosition(), end->GetPosition() );
}

void tgIntersectionEdge::IntersectConstraintsAndSides(tgIntersectionEdgeInfo* cur)
{        
    bool   ce_originating = cur->IsOriginating();
    
    if ( ce_originating ) {        
        if ( bl_set ) {
            if ( !constrain_bl.Intersect( side_l, conBotLeft ) ) {
                SG_LOG(SG_GENERAL, SG_ALERT, "tgIntersectionEdge::IntersectConstraintsAndSides: cur edge " << id << ":" << " bottom left did not intersect left side" );
        
                tgShapefile::FromRay(  constrain_bl, GetDatasource(), "NO_INT_bottom_left_with_left_side", "ray" );
                tgShapefile::FromLine( side_l, GetDatasource(), "NO_INT_bottom_left_with_left_side", "line" );                
            }
        } else {
            double dist = SGGeodesy::distanceM(start->GetPosition(), end->GetPosition() );
            SG_LOG(SG_GENERAL, SG_ALERT, "tgIntersectionEdge::IntersectConstraintsAndSides: cur edge has no bl constraints - expecting 1. Length is " << dist);                    
        }

        if ( br_set ) {
            if ( !constrain_br.Intersect( side_r, conBotRight ) ) {
                SG_LOG(SG_GENERAL, SG_ALERT, "tgIntersectionEdge::IntersectConstraintsAndSides: cur edge " << id << ":" << " bottom right did not intersect right side" );

                tgShapefile::FromRay(  constrain_br, GetDatasource(), "NO_INT_bottom_right_with_right_side", "ray" );
                tgShapefile::FromLine( side_r, GetDatasource(), "NO_INT_bottom_right_with_right_side", "line" );                
            }
        } else {
            double dist = SGGeodesy::distanceM(start->GetPosition(), end->GetPosition() );
            SG_LOG(SG_GENERAL, SG_ALERT, "tgIntersectionEdge::IntersectConstraintsAndSides: cur edge has no br constraints - expecting 1. length is " << dist );                    
        }
        
        flags |= FLAGS_INTERSECTED_BOTTOM_CONSTRAINTS;      
    } else {
        if ( tl_set ) {
            if ( !constrain_tl.Intersect( side_l, conTopLeft ) ) {
                SG_LOG(SG_GENERAL, SG_ALERT, "tgIntersectionEdge::IntersectConstraintsAndSides: cur edge " << id << ":" << " top left did not intersect left side" );
        
                tgShapefile::FromRay(  constrain_tl, GetDatasource(), "NO_INT_top_left_with_left_side", "ray" );
                tgShapefile::FromLine( side_l, GetDatasource(), "NO_INT_top_left_with_left_side", "line" );                
            }
        } else {
            double dist = SGGeodesy::distanceM(start->GetPosition(), end->GetPosition() );            
            SG_LOG(SG_GENERAL, SG_ALERT, "tgIntersectionEdge::IntersectConstraintsAndSides: cur edge has no tl constraints - expecting 1.  length is " << dist );                    
        }

        if ( tr_set ) {
            if ( !constrain_tr.Intersect( side_r, conTopRight ) ) {
                SG_LOG(SG_GENERAL, SG_ALERT, "tgIntersectionEdge::IntersectConstraintsAndSides: cur edge " << id << ":" << " top right did not intersect right side" );

                tgShapefile::FromRay(  constrain_tr, GetDatasource(), "NO_INT_top_right_with_right_side", "ray" );
                tgShapefile::FromLine( side_r, GetDatasource(), "NO_INT_top_right_with_right_side", "line" );                
            }
        } else {
            double dist = SGGeodesy::distanceM(start->GetPosition(), end->GetPosition() );
            SG_LOG(SG_GENERAL, SG_ALERT, "tgIntersectionEdge::IntersectConstraintsAndSides: cur edge has no tr constraints - expecting 1.  Length is " << dist );                    
        }
        
        flags |= FLAGS_INTERSECTED_TOP_CONSTRAINTS;
    }
}

void tgIntersectionEdge::SetLeftConstraint( bool originating, const std::list<SGGeod>& cons )
{
    if ( originating ) {
        if (!msbl_valid) {
            constrain_msbl.clear();
        
            // if we are originating, push to front of bottom left constraint
            for ( std::list<SGGeod>::const_iterator it = cons.begin(); it != cons.end(); it ++ ) {
                constrain_msbl.push_front( (*it) );
            }
            msbl_set = true;
        }
    } else {
        if (!mstr_valid) {
            constrain_mstr.clear();

            // otherwise, push to front of top right constraint
            for ( std::list<SGGeod>::const_iterator it = cons.begin(); it != cons.end(); it ++ ) {
                constrain_mstr.push_front( (*it) );
            }
            mstr_set = true;
        }
    }
}

void tgIntersectionEdge::SetRightConstraint( bool originating, const std::list<SGGeod>& cons )
{
    if ( originating ) {
        if (!msbr_valid) {
            constrain_msbr.clear();
        
            // if we are originating, push to back of bottom right constraint
            for ( std::list<SGGeod>::const_iterator it = cons.begin(); it != cons.end(); it ++ ) {
                constrain_msbr.push_back( (*it) );
            }
            msbr_set = true;
        }
    } else {
        if (!mstl_valid) {
            constrain_mstl.clear();

            // otherwise, push to back of top left constraint
            for ( std::list<SGGeod>::const_iterator it = cons.begin(); it != cons.end(); it ++ ) {
                constrain_mstl.push_back( (*it) );
            }
            mstl_set = true;
        }
    }
}

void tgIntersectionEdge::SetLeftProjectList( bool originating, const std::list<SGGeod>& pl )
{
    if ( originating ) {
        if (!msblpl_set) {
            projectlist_msbl.clear();
            
            // if we are originating, push to front of bottom left constraint
            for ( std::list<SGGeod>::const_iterator it = pl.begin(); it != pl.end(); it ++ ) {
                projectlist_msbl.push_front( (*it) );
            }
            msblpl_set = true;
        }
    } else {
        if (!mstrpl_set) {
            projectlist_mstr.clear();
            
            // otherwise, push to front of top right constraint
            for ( std::list<SGGeod>::const_iterator it = pl.begin(); it != pl.end(); it ++ ) {
                projectlist_mstr.push_front( (*it) );
            }
            mstrpl_set = true;
        }
    }
}

void tgIntersectionEdge::SetRightProjectList( bool originating, const std::list<SGGeod>& pl )
{
    if ( originating ) {
        if (!msbrpl_set) {
            projectlist_msbr.clear();
            
            // if we are originating, push to back of bottom right constraint
            for ( std::list<SGGeod>::const_iterator it = pl.begin(); it != pl.end(); it ++ ) {
                projectlist_msbr.push_back( (*it) );
            }
            msbrpl_set = true;
        }
    } else {
        if (!mstlpl_set) {
            projectlist_mstl.clear();
            
            // otherwise, push to back of top left constraint
            for ( std::list<SGGeod>::const_iterator it = pl.begin(); it != pl.end(); it ++ ) {
                projectlist_mstl.push_back( (*it) );
            }
            mstlpl_set = true;
        }
    }
}

void tgIntersectionEdge::ApplyConstraint( bool apply )
{
    if ( msbr_set ) {
        if ( apply ) {
            msbr_valid = true;
            msbr_set = false;
        } else {
            constrain_msbr.clear();
            msbr_set = false;
        }
    }
    
    if ( mstr_set ) {
        if ( apply ) {
            mstr_valid = true;
            mstr_set = false;
        } else {
            constrain_mstr.clear();
            mstr_set = false;
        }
    }
    
    if ( mstl_set ) {
        if ( apply ) {
            mstl_valid = true;
            mstl_set = false;
        } else {
            constrain_mstl.clear();
            mstl_set = false;
        }
    }
    
    if ( msbl_set ) {
        if ( apply ) {
            msbl_set = true;
            msbl_set = false;
        } else {
            constrain_msbl.clear();
            msbl_set = false;
        }
    }
}

void tgIntersectionEdge::DumpConstraint( const char* layer, const char* label, const std::list<SGGeod>& contour ) const
{
#if DEBUG_INTERSECTIONS    
    std::vector<SGGeod> gl;
    for (std::list<SGGeod>::const_iterator it = contour.begin(); it != contour.end(); it++ ) {
        gl.push_back( (*it) );
    }
    
    tgShapefile::FromGeodList( gl, true, datasource, layer, label );
#endif    
}

tgPolygon tgIntersectionEdge::CreatePolygon( int& id, double& heading, double& dist, double& w, SGGeod& tref, const char* debug_layer )
{
    // just return a poly with the four nodes
    tgPolygon poly;
    
    poly.AddNode( 0, botLeft );
    poly.AddNode( 0, botRight );
    poly.AddNode( 0, topRight );
    poly.AddNode( 0, topLeft );

    heading      = SGGeodesy::courseDeg( start->GetPosition(), end->GetPosition() );
    dist         = SGGeodesy::distanceM( start->GetPosition(), end->GetPosition() );
    id           = type;
    w            = width;
    tref         = botLeft;    
    
    return poly;
}

// TODO : This can be quite complex - interactions between ms corners and sides...
// best to come up with something as general as possible.
void tgIntersectionEdge::Complete( void )
{
#if DEBUG_INTERSECTIONS                                    
    char layer[256];
#endif
    
    // build right and left contours
    // right contour starts at bottom right corner, and ends at top left corner
    right_contour.clear();  // TODO : No right_contour until now...                
        
    if ( constrain_msbr.empty() && constrain_mstr.empty() ) {
        // no ms corners - use normal corners ( if start is present, it's the end of left contour )
        
        // right_contour.push_back( start->GetPosition() );
        right_contour.push_back( conBotRight );
        right_contour.push_back( conTopRight );
    } else if ( !constrain_msbr.empty() && constrain_mstr.empty() ) {
        // MS on bottom right, but no MS on top right

        // Check if start of msbr is equal to Start. If not, add start first
        if ( !SGGeod_isEqual2D( start->GetPosition(), *constrain_msbr.begin() ) ) {
            // right_contour.push_back( start->GetPosition() );
        }

        // add MSBR
        right_contour.insert( right_contour.end(), constrain_msbr.begin(), constrain_msbr.end() );

        // check if MSBR ends on right side
        std::list<SGGeod>::iterator last_it = constrain_msbr.end(); last_it--;

        if ( side_r.isOn( (*last_it ) ) ) {
            // yes, use conTopRight for top right corner
            right_contour.push_back( conTopRight );
        } else {
#if DEBUG_INTERSECTIONS                                    
            sprintf( layer, "NOT_ON_RIGHT_SIDE_1" );
            tgShapefile::FromGeod( (*last_it ), GetDatasource(), layer, "pt" );
#endif            
        }
    } else if ( constrain_msbr.empty() && !constrain_mstr.empty() ) {
        // no MS on bottom right, but MS on top right
        // right_contour.push_back( start->GetPosition() );
 
        // check if start of MSTR is on right side
        std::list<SGGeod>::iterator first_it = constrain_mstr.begin();

        if ( side_r.isOn( (*first_it ) ) ) {
            // yes, use conBotRight for bottom right corner
            right_contour.push_back( conBotRight );
        } else {
#if DEBUG_INTERSECTIONS                                    
            sprintf( layer, "NOT_ON_RIGHT_SIDE_2" );
            tgShapefile::FromGeod( (*first_it ), GetDatasource(), layer, "pt" );
#endif            
        }            
        
        // Add MSTR
        right_contour.insert( right_contour.end(), constrain_mstr.begin(), constrain_mstr.end() );
    } else {
        // MS on bottom right and top right

        // Check if start of msbr is equal to Start. If not, add start first
        if ( !SGGeod_isEqual2D( start->GetPosition(), *constrain_msbr.begin() ) ) {
        }
        
        // add MSBR
        right_contour.insert( right_contour.end(), constrain_msbr.begin(), constrain_msbr.end() );
        
        // add MSTR
        right_contour.insert( right_contour.end(), constrain_mstr.begin(), constrain_mstr.end() );
    }

    // left contour starts at top left corner and ends at bottom right
    left_contour.clear();

    if ( constrain_mstl.empty() && constrain_msbl.empty() ) {
        // no ms corners - use normal corners
        left_contour.push_back( conTopLeft );
        left_contour.push_back( conBotLeft );
    } else if ( !constrain_mstl.empty() && constrain_msbl.empty() ) {
        // MS on top left, but no MS on bottom left

        // add MSTL
        left_contour.insert( left_contour.end(), constrain_mstl.begin(), constrain_mstl.end() );

        // check if MSTL ends on left side
        std::list<SGGeod>::iterator last_it = constrain_mstl.end(); last_it--;

        if ( side_l.isOn( (*last_it ) ) ) {
            // yes, use conBotLeft for bottom left corner
            left_contour.push_back( conBotLeft );
        } else {
#if DEBUG_INTERSECTIONS                                    
            sprintf( layer, "NOT_ON_LEFT_SIDE_1" );
            tgShapefile::FromGeod( (*last_it ), GetDatasource(), layer, "pt" );
#endif            
        }
    } else if ( constrain_mstl.empty() && !constrain_msbl.empty() ) {
        // no MS on top left, but MS on bottom left
 
        // check if start of MSBL is on left side
        std::list<SGGeod>::iterator first_it = constrain_msbl.begin();

        if ( side_l.isOn( (*first_it ) ) ) {
            // yes, use conTopLeft for top left corner
            left_contour.push_back( conTopLeft );
        } else {
#if DEBUG_INTERSECTIONS                                    
            sprintf( layer, "NOT_ON_LEFT_SIDE_2" );
            tgShapefile::FromGeod( (*first_it ), GetDatasource(), layer, "pt" );
#endif            
        }            
        
        // Add MSBL
        left_contour.insert( left_contour.end(), constrain_msbl.begin(), constrain_msbl.end() );
    } else {
        // MS on top left and bottom left
        
        // add MSTL
        left_contour.insert( left_contour.end(), constrain_mstl.begin(), constrain_mstl.end() );
        
        // add MSBL
        left_contour.insert( left_contour.end(), constrain_msbl.begin(), constrain_msbl.end() );
    }
    
    // now check if we need to add start or end
    // if the start / end node degree is > 2, then add the start / end position
    if ( start->Degree() > 2 ) {
        left_contour.push_back( start->GetPosition() );
    }
    if ( end->Degree() > 2 ) {
        right_contour.push_back( end->GetPosition() );
    }

    // now project added nodes in multisegment intersections to opposite sides 
    // suggestion by I4DNF to generate more well behaved triangulations
    double course;
    course = SGGeodesy::courseDeg( start->GetPosition(), end->GetPosition() ) + 90;    
    for ( std::list<SGGeod>::iterator lit = projectlist_msbl.begin(); lit != projectlist_msbl.end(); lit++ ) {
        // find the opposite edge and see if we can split it
        tgLine perp( (*lit), course );
    
        std::list<SGGeod>::iterator rit_s = right_contour.begin();
        std::list<SGGeod>::iterator rit_e = rit_s; rit_e++;        
        do {
            if ( perp.OrientedSide( (*rit_s) ) != perp.OrientedSide( (*rit_e) ) ) {
                tgSegment seg( (*rit_s), (*rit_e) );
                SGGeod    intersection;
                if ( perp.Intersect( seg, intersection ) ) {
                    rit_s = right_contour.insert( rit_e, intersection );
                }
                break;
            } else {
            rit_s=rit_e;
            rit_e++;
            }
        } while ( rit_e != right_contour.end() );
    }

    for ( std::list<SGGeod>::iterator lit = projectlist_mstl.begin(); lit != projectlist_mstl.end(); lit++ ) {
        // find the opposite edge and see if we can split it
        tgLine perp( (*lit), course );
        
        std::list<SGGeod>::iterator rit_s = right_contour.begin();
        std::list<SGGeod>::iterator rit_e = rit_s; rit_e++;        
        do {
            if ( perp.OrientedSide( (*rit_s) ) != perp.OrientedSide( (*rit_e) ) ) {
                tgSegment seg( (*rit_s), (*rit_e) );
                SGGeod    intersection;
                if ( perp.Intersect( seg, intersection ) ) {
                    rit_s = right_contour.insert( rit_e, intersection );
                }
                break;
            } else {
                rit_s=rit_e;
                rit_e++;
            }
        } while ( rit_e != right_contour.end() );
    }

    course = SGGeodesy::courseDeg( start->GetPosition(), end->GetPosition() ) - 90;
    for ( std::list<SGGeod>::iterator rit = projectlist_msbr.begin(); rit != projectlist_msbr.end(); rit++ ) {
        // find the opposite edge and see if we can split it
        tgLine perp( (*rit), course );
        
        std::list<SGGeod>::iterator lit_s = left_contour.begin();
        std::list<SGGeod>::iterator lit_e = lit_s; lit_e++;        
        do {
            if ( perp.OrientedSide( (*lit_s) ) != perp.OrientedSide( (*lit_e) ) ) {
                tgSegment seg( (*lit_s), (*lit_e) );
                SGGeod    intersection;
                if ( perp.Intersect( seg, intersection ) ) {
                    lit_s = left_contour.insert( lit_e, intersection );
                }
                break;
            } else {
                lit_s=lit_e;
                lit_e++;
            }
        } while ( lit_e != left_contour.end() );
    }
    
    for ( std::list<SGGeod>::iterator rit = projectlist_mstr.begin(); rit != projectlist_mstr.end(); rit++ ) {
        // find the opposite edge and see if we can split it
        tgLine perp( (*rit), course );
        
        std::list<SGGeod>::iterator lit_s = left_contour.begin();
        std::list<SGGeod>::iterator lit_e = lit_s; lit_e++;        
        do {
            if ( perp.OrientedSide( (*lit_s) ) != perp.OrientedSide( (*lit_e) ) ) {
                tgSegment seg( (*lit_s), (*lit_e) );
                SGGeod    intersection;
                if ( perp.Intersect( seg, intersection ) ) {
                    lit_s = left_contour.insert( lit_e, intersection );
                }
                break;
            } else {
                lit_s=lit_e;
                lit_e++;
            }
        } while ( lit_e != left_contour.end() );
    }
}

bool tgIntersectionEdge::Verify( unsigned long int f )  
{ 
    char datasrc[32];
    char description[64];
    bool pass = true;
    
    sprintf( datasrc, "./edge_dbg/%s", debugRoot.c_str() );
    
    if ( f & FLAGS_INTERSECTED_BOTTOM_CONSTRAINTS ) {
        if ( (flags & FLAGS_INTERSECTED_BOTTOM_CONSTRAINTS) == 0 ) {
            SG_LOG( SG_GENERAL, SG_ALERT, "tgIntersectionEdge::Verify : edge " << id << " never got bottom contraint intersection with side");

            tgSegment seg( start->GetPosition(), end->GetPosition() );
            tgShapefile::FromSegment( seg, true, datasrc, "no_bottom_intersections", description );
            
            pass = false;
        }
    }
    
    if ( f & FLAGS_INTERSECTED_TOP_CONSTRAINTS ) {
        if ( (flags & FLAGS_INTERSECTED_TOP_CONSTRAINTS) == 0 ) {
            SG_LOG( SG_GENERAL, SG_ALERT, "tgIntersectionEdge::Verify : edge " << id << " never got top contraint intersection with side");                

            tgSegment seg( start->GetPosition(), end->GetPosition() );
            tgShapefile::FromSegment( seg, true, datasrc, "no_top_intersections", description );
            
            pass = false;
        }
    }

    if ( f & FLAGS_TEXTURED ) {
        if ( (flags & FLAGS_TEXTURED) == 0 ) {
            SG_LOG( SG_GENERAL, SG_ALERT, "tgIntersectionEdge::Verify : edge " << id << " never got textured");
            
            tgSegment seg( start->GetPosition(), end->GetPosition() );
            tgShapefile::FromSegment( seg, true, datasrc, "not_textured", description );
            
            pass = false;
            
            poly.SetVertexAttributeInt(TG_VA_CONSTANT, 0, 0);   
        }
    }
    
    return pass; 
}

tgPolygon tgIntersectionEdge::GetPoly(const char* prefix)
{    
    return poly;
}

double tgIntersectionEdge::Texture( bool originating, double v_end, tgIntersectionGeneratorTexInfoCb texInfoCb, double ratio )
{
    std::string material;
    double      texAtlasStartU, texAtlasEndU;
    double      texAtlasStartV, texAtlasEndV;
    double      v_start;
    double      v_dist;
    double      heading;
    static int  textured_idx = 0;
    
    std::list<SGGeod>::iterator i;
    
    for ( i = right_contour.begin(); i != right_contour.end(); i++) {
        poly.AddNode( 0, *i );
    }    
    for ( i = left_contour.begin(); i != left_contour.end(); i++) {
        poly.AddNode( 0, *i );
    }
    
    if ( start->IsCap() ) {
        texInfoCb( type, true, material, texAtlasStartU, texAtlasEndU, texAtlasStartV, texAtlasEndV, v_dist );
        
        if ( originating ) {
            heading = SGGeodesy::courseDeg( start->GetPosition(), end->GetPosition() );
            poly.SetTexParams( botLeft, width, 0.5, heading );
        } else {
            heading = SGGeodesy::courseDeg( end->GetPosition(), start->GetPosition() );
            poly.SetTexParams( topRight, width, 0.5, heading );        
        }
        poly.SetMaterial( material );
        poly.SetTexMethod( TG_TEX_1X1_ATLAS );
        poly.SetTexLimits( texAtlasStartU, texAtlasStartV, texAtlasEndU, texAtlasEndV );
        poly.SetVertexAttributeInt(TG_VA_CONSTANT, 0, 1);
                
#if DEBUG_TEXTURE        
        // DEBUG : add an arrow with v_start, v_end
        tgSegment seg( start->GetPosition(), end->GetPosition() );
        char from_to[128];
        sprintf( from_to, "id %ld textured start cap %d from %lf, to %lf", id, textured_idx++, v_start, v_end );
        tgShapefile::FromSegment( seg, true, "./", "Texture", from_to );
#endif
        
    } else if ( end->IsCap() ) {
        texInfoCb( type, true, material, texAtlasStartU, texAtlasEndU, texAtlasStartV, texAtlasEndV, v_dist );
        
        if ( originating ) {
            heading = SGGeodesy::courseDeg( start->GetPosition(), end->GetPosition() );
            poly.SetTexParams( botLeft, width, 0.5, heading );
        } else {
            heading = SGGeodesy::courseDeg( end->GetPosition(), start->GetPosition() );
            poly.SetTexParams( topRight, width, 0.5, heading );        
        }
        poly.SetMaterial( material );
        poly.SetTexMethod( TG_TEX_1X1_ATLAS );
        poly.SetTexLimits( texAtlasStartU, texAtlasStartV, texAtlasEndU, texAtlasEndV );
        poly.SetVertexAttributeInt(TG_VA_CONSTANT, 0, 1);
        
#if DEBUG_TEXTURE        
        // DEBUG : add an arrow with v_start, v_end
        tgSegment seg( start->GetPosition(), end->GetPosition() );
        char from_to[128];
        sprintf( from_to, "id %ld textured end start cap %d from %lf, to %lf", id, textured_idx++, v_start, v_end );
        tgShapefile::FromSegment( seg, true, "./", "Texture", from_to );
#endif
        
    } else {
        texInfoCb( type, false, material, texAtlasStartU, texAtlasEndU, texAtlasStartV, texAtlasEndV, v_dist );
        v_dist *= ratio;
        
        double dist = SGGeodesy::distanceM( start->GetPosition(), end->GetPosition() );
        v_start = fmod( v_end, 1.0 );
        v_end   = v_start + (dist/v_dist);
        
        if ( originating ) {
            SG_LOG( SG_GENERAL, LOG_TEXTURE, "tgIntersectionEdge::Texture : edge " << id << " originating : v_start=" << v_start << " v_end= " << v_end << " dist= " << dist << " v_dist= " << v_dist );
            heading = SGGeodesy::courseDeg( start->GetPosition(), end->GetPosition() );
            poly.SetTexParams( botLeft, width, dist, heading );
        } else {
            SG_LOG( SG_GENERAL, LOG_TEXTURE, "tgIntersectionEdge::Texture : edge " << id << " NOT originating : v_start=" << v_start << " v_end= " << v_end << " dist= " << dist << " v_dist= " << v_dist );
            heading = SGGeodesy::courseDeg( end->GetPosition(), start->GetPosition() );
            poly.SetTexParams( topRight, width, dist, heading );        
        }    

        poly.SetMaterial( material );
        poly.SetTexMethod( TG_TEX_BY_TPS_CLIPU, -1.0, 0.0, 1.0, 0.0 );
        poly.SetTexLimits( texAtlasStartU, v_start, texAtlasEndU, v_end );
        poly.SetVertexAttributeInt(TG_VA_CONSTANT, 0, 0);
        
#if DEBUG_TEXTURE        
        // DEBUG : add an arrow with v_start, v_end
        tgSegment seg( start->GetPosition(), end->GetPosition() );
        char from_to[128];
        sprintf( from_to, "id %ld textured %d from %lf, to %lf", id, textured_idx++, v_start, v_end );
        tgShapefile::FromSegment( seg, true, "./", "Texture", from_to );
#endif        
    }
    
    flags |= FLAGS_TEXTURED;
    
    return v_end;
}

tgIntersectionEdgeInfo::tgIntersectionEdgeInfo( bool orig, tgIntersectionEdge* e ) 
{    
    edge              = e;
    originating       = orig;
    
    if ( originating ) {
        heading         = SGGeodesy::courseDeg( edge->start->GetPosition(), edge->end->GetPosition() );
        geodesy_heading = SGGeodesy::courseDeg( edge->start->GetPosition(), edge->end->GetPosition() );
    } else {
        heading         = SGGeodesy::courseDeg( edge->end->GetPosition(), edge->start->GetPosition() );
        geodesy_heading = SGGeodesy::courseDeg( edge->end->GetPosition(), edge->start->GetPosition() );
    }
}

double tgIntersectionEdgeInfo::Texture( double vEnd, tgIntersectionGeneratorTexInfoCb texInfoCb, double ratio ) {
    return edge->Texture( originating, vEnd, texInfoCb, ratio );
}

void tgIntersectionEdgeInfo::TextureStartCap( tgIntersectionGeneratorTexInfoCb texInfoCb ) {
    if ( edge->right_contour.empty() || edge->left_contour.empty() ) {
        SG_LOG( SG_GENERAL, SG_ALERT, "tgIntersectionEdgeInfo::TextureEndCap : edge " << edge->id << " HAS NO CONTOUR ");
    } else {
        edge->Texture( originating, 0.0f, texInfoCb, 1.0f );
    }    
}

void tgIntersectionEdgeInfo::TextureEndCap( tgIntersectionGeneratorTexInfoCb texInfoCb ) {
    
    if ( edge->right_contour.empty() || edge->left_contour.empty() ) {
        SG_LOG( SG_GENERAL, SG_ALERT, "tgIntersectionEdgeInfo::TextureEndCap : edge " << edge->id << " HAS NO CONTOUR ");
    } else {
        edge->Texture( originating, 0.0f, texInfoCb, 1.0f );
    }
}

bool tgIntersectionEdgeInfo::IsTextured( void ) const {
    return edge->IsTextured(); 
}

bool tgIntersectionEdgeInfo::IsStartCap(void) const
{
    return (edge->start->Degree() == 1);
}

bool tgIntersectionEdgeInfo::IsEndCap(void) const
{
    return (edge->end->Degree() == 1);
}
    
tgRay tgIntersectionEdgeInfo::GetDirectionRay(void) const
{
    SGGeod s, e;
    
    if ( originating ) {
        s = edge->start->GetPosition();
        e = edge->end->GetPosition();
    } else {
        s = edge->end->GetPosition();
        e = edge->start->GetPosition();            
    }

    return tgRay( s, e );
}