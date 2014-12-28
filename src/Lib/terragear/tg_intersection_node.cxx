#include <simgear/sg_inlines.h>

#include "tg_polygon.hxx"
#include "tg_shapefile.hxx" 
#include "tg_intersection_node.hxx"
#include "tg_intersection_edge.hxx"
#include "tg_misc.hxx"
#include "tg_euclidean.hxx"
      
#define DEBUG_INTERSECTIONS (0)
#define LOG_INTERSECTION    (SG_DEBUG)

tgIntersectionNode::tgIntersectionNode( const SGGeod& pos )
{
    position = pos;
    edgeList.clear();
    start_v = NODE_UNTEXTURED;
}

void tgIntersectionNode::AddEdge( bool originated, tgIntersectionEdge* edge ) 
{
    edgeList.insert( new tgIntersectionEdgeInfo( originated, edge ) );
}

void tgIntersectionNode::DelEdge( bool originated, tgIntersectionEdge* edge ) 
{
    SG_LOG(SG_GENERAL, LOG_INTERSECTION, "tgIntersectionNode::DelEdge: enter - node has " << edgeList.size() << " edges " );
    
    for (tgintersectionedgeinfo_it cur = edgeList.begin(); cur != edgeList.end(); cur++) {
        // edge info is unique from node to node - but the edge itself is shared
        if( (*cur)->GetEdge() == edge ) {
            // we found the current edge info - delete it
            edgeList.erase(cur);
            break;
        }
    }

    SG_LOG(SG_GENERAL, LOG_INTERSECTION, "tgIntersectionNode::DelEdge: exit - node has " << edgeList.size() << " edges ");    
}


tgIntersectionEdgeInfo* tgIntersectionNode::GetPrevEdgeInfo( tgIntersectionEdgeInfo* cur_info, const tgRay& bisector, const SGGeod& bisect_pos, const char* prefix )
{
    tgintersectionedgeinfo_it prv, cur;
    tgIntersectionEdgeInfo*   prv_info = NULL;
    const tgIntersectionEdge* ce       = cur_info->GetEdge();
    int                       ce_id    = ce->id;
    char                      layer[256];
    
    if ( edgeList.size() > 1 ) {
        for (cur = edgeList.begin(); cur != edgeList.end(); cur++) {
            // edge info is unique from node to node - but the edge itself is shared
            if( (*cur)->GetEdge() == ce ) {
                // we found the current edge info - decrement to prv
                if ( cur != edgeList.begin() ) {
                    prv = cur;
                    prv--;
                } else {
                    prv = edgeList.end();
                    prv--;
                }
            }
        }
    
        // now, loop around until we find an edge where the top/bottom intersects the bisector at the same position
        while ( !prv_info && prv != cur ) {
            bool ve_originating    = (*prv)->IsOriginating();
            tgIntersectionEdge* ve = (*prv)->GetEdge();
            int ve_id              = ve->id;

            SG_LOG(SG_GENERAL, LOG_INTERSECTION, "tgIntersectionNode::GetPrevEdgeInfo: verify prev edge for ce " << ce_id << " which could be " << ve_id << " has proper intersection with bisector - check BottomRight" );
            // bisect the shared constraint to verify
            if ( !prv_info ) {
                SGGeod              verify_intersect;
                char                cons_name[16];
                
                tgRay ve_start = ve->GetBottomLeftConstraint( ve_originating );
                
                if ( ve_originating ) {
                    strcpy( cons_name, "BL" );
                } else {
                    strcpy( cons_name, "TR" );
                }

                if ( bisector.Intersect( ve_start, verify_intersect ) ) {
                    if ( SGGeod_isEqual2D( bisect_pos, verify_intersect ) ) {
                        // true found the correct edge
                        prv_info = (*prv);

#if DEBUG_INTERSECTIONS                        
                        sprintf(layer, "%s_ve_bisector_id_%04d_%s", prefix, ve_id, cons_name );
                        tgShapefile::FromRay( bisector, ce->GetDatasource(), layer, "cons" );
                        
                        sprintf(layer, "%s_ve_start_id_%04d_%s", prefix, ve_id, cons_name );
                        tgShapefile::FromRay( ve_start, ce->GetDatasource(), layer, "cons" );
                        
                        sprintf(layer, "%s_ve_intersect_id_%04d_%s", prefix, ve_id, cons_name );
                        tgShapefile::FromGeod( verify_intersect, ce->GetDatasource(), layer, "pt" );
#endif                        
                    } else {
                        SG_LOG(SG_GENERAL, LOG_INTERSECTION, "tgIntersectionNode::GetPrevEdgeInfo: ve " << ve_id << " bisector intersects ve_start, but not at the same pos as ce." );

#if DEBUG_INTERSECTIONS                        
                        sprintf(layer, "%s_ve_bisector_id_%04d_%s", prefix, ve_id, cons_name );
                        tgShapefile::FromRay( bisector, ce->GetDatasource(), layer, "cons" );
                        
                        sprintf(layer, "%s_ve_start_id_%04d_%s", prefix, ve_id, cons_name );
                        tgShapefile::FromRay( ve_start, ce->GetDatasource(), layer, "cons" );
                        
                        sprintf(layer, "%s_ve_intersect_id_%04d_%s", prefix, ve_id, cons_name );
                        tgShapefile::FromGeod( verify_intersect, ce->GetDatasource(), layer, "pt" );
#endif                        
                    }
                } else {
                    SG_LOG(SG_GENERAL, LOG_INTERSECTION, "tgIntersectionNode::GetPrevEdgeInfo: ve " << ve_id << " bisector DOES NOT intersect ve_start." );

#if DEBUG_INTERSECTIONS                        
                    sprintf(layer, "%s_ve_bisector_id_%04d_%s", prefix, ve_id, cons_name );
                    tgShapefile::FromRay( bisector, ce->GetDatasource(), layer, "cons" );

                    sprintf(layer, "%s_ve_start_id_%04d_%s", prefix, ve_id, cons_name );
                    tgShapefile::FromRay( ve_start, ce->GetDatasource(), layer, "cons" );
#endif                    
                }
            }

            SG_LOG(SG_GENERAL, LOG_INTERSECTION, "tgIntersectionNode::GetPrevEdgeInfo: verify prev edge for ce " << ce_id << " which could be " << ve_id << " has proper intersection with bisector - check BottomLeft" );
            // bisect the shared constraint to verify
            if ( !prv_info ) {
                SGGeod              verify_intersect;
                char                cons_name[16];
                
                tgRay ve_start = ve->GetBottomRightConstraint( ve_originating );

                if ( ve_originating ) {
                    strcpy( cons_name, "BR" );
                } else {
                    strcpy( cons_name, "TL" );
                }

                if ( bisector.Intersect( ve_start, verify_intersect ) ) {
                    if ( SGGeod_isEqual2D( bisect_pos, verify_intersect ) ) {
                        // true found teh correct edge
                        prv_info = (*prv);

#if DEBUG_INTERSECTIONS                        
                        sprintf(layer, "%s_ve_bisector_id_%04d_%s", prefix, ve_id, cons_name );
                        tgShapefile::FromRay( bisector, ce->GetDatasource(), layer, "cons" );
                        
                        sprintf(layer, "%s_ve_start_id_%04d_%s", prefix, ve_id, cons_name );
                        tgShapefile::FromRay( ve_start, ce->GetDatasource(), layer, "cons" );
                        
                        sprintf(layer, "%s_ve_intersect_id_%04d_%s", prefix, ve_id, cons_name );
                        tgShapefile::FromGeod( verify_intersect, ce->GetDatasource(), layer, "pt" );
#endif                        
                        
                    } else {
                        SG_LOG(SG_GENERAL, LOG_INTERSECTION, "tgIntersectionNode::GetPrevEdgeInfo: ve " << ve_id << " bisector intersects ve_start, but not at the same pos as ce." );

#if DEBUG_INTERSECTIONS                                                
                        sprintf(layer, "%s_ve_bisector_id_%04d_%s", prefix, ve_id, cons_name );
                        tgShapefile::FromRay( bisector, ce->GetDatasource(), layer, "cons" );
                        
                        sprintf(layer, "%s_ve_start_id_%04d_%s", prefix, ve_id, cons_name );
                        tgShapefile::FromRay( ve_start, ce->GetDatasource(), layer, "cons" );
                        
                        sprintf(layer, "%s_ve_intersect_id_%04d_%s", prefix, ve_id, cons_name );
                        tgShapefile::FromGeod( verify_intersect, ce->GetDatasource(), layer, "pt" );
#endif                        
                    }
                } else {
                    SG_LOG(SG_GENERAL, LOG_INTERSECTION, "tgIntersectionNode::GetPrevEdgeInfo: ve " << ve_id << " bisector DOES NOT intersect ve_start." );

#if DEBUG_INTERSECTIONS                        
                    sprintf(layer, "%s_ve_bisector_id_%04d_%s", prefix, ve_id, cons_name );
                    tgShapefile::FromRay( bisector, ce->GetDatasource(), layer, "cons" );
                    
                    sprintf(layer, "%s_ve_start_id_%04d_%s", prefix, ve_id, cons_name );
                    tgShapefile::FromRay( ve_start, ce->GetDatasource(), layer, "cons" );                        
#endif                    
                }
            }

            if ( prv != edgeList.begin() ) {
                prv--;
            } else {
                prv = edgeList.end();
                prv--;
            }            
        }                                    
    }
    
    return prv_info;
}

tgIntersectionEdgeInfo* tgIntersectionNode::GetNextEdgeInfo( tgIntersectionEdgeInfo* cur_info, const tgRay& bisector, const SGGeod& bisect_pos, const char* prefix )
{
    tgintersectionedgeinfo_it cur, nxt;
    tgIntersectionEdgeInfo*   nxt_info = NULL;
    const tgIntersectionEdge* ce       = cur_info->GetEdge();
    int                       ce_id    = ce->id;
    char                      layer[256];
    
    if ( edgeList.size() > 1 ) {
        for (cur = edgeList.begin(); cur != edgeList.end(); cur++) {
            // edge info is unique from node to node - but the edge itself is shared
            if( (*cur)->GetEdge() == ce ) {
                // we found the current edge info - increment to next
                nxt = cur;
                nxt++;

                if ( nxt == edgeList.end() ) {
                    nxt = edgeList.begin();
                }
            }
        }

        // now, loop around until we find an edge where the top/bottom intersects the bisector at the same position
        while ( !nxt_info && nxt != cur ) {
            bool ve_originating    = (*nxt)->IsOriginating();
            tgIntersectionEdge* ve = (*nxt)->GetEdge();
            int ve_id              = ve->id;
            
            SG_LOG(SG_GENERAL, LOG_INTERSECTION, "tgIntersectionNode::GetNextEdgeInfo: verify next edge for ce " << ce_id << " which could be " << ve_id << " has proper intersection with bisector - check BottomRight" );
            // bisect the shared constraint to verify
            if ( !nxt_info ) {
                SGGeod              verify_intersect;
                char                cons_name[16];

                tgRay ve_start = ve->GetBottomRightConstraint( ve_originating );

                if ( ve_originating ) {
                    strcpy( cons_name, "BR" );
                } else {
                    strcpy( cons_name, "TL" );
                }

                if ( bisector.Intersect( ve_start, verify_intersect ) ) {
                    if ( SGGeod_isEqual2D( bisect_pos, verify_intersect ) ) {
                        // true found the correct edge
                        nxt_info = (*nxt);

#if DEBUG_INTERSECTIONS                        
                        sprintf(layer, "%s_ve_bisector_id_%04d_%s", prefix, ve_id, cons_name );
                        tgShapefile::FromRay( bisector, ce->GetDatasource(), layer, "cons" );
                        
                        sprintf(layer, "%s_ve_start_id_%04d_%s", prefix, ve_id, cons_name );
                        tgShapefile::FromRay( ve_start, ce->GetDatasource(), layer, "cons" );
                        
                        sprintf(layer, "%s_ve_intersect_id_%04d_%s", prefix, ve_id, cons_name );
                        tgShapefile::FromGeod( verify_intersect, ce->GetDatasource(), layer, "pt" );
#endif
                        
                    } else {
                        SG_LOG(SG_GENERAL, LOG_INTERSECTION, "tgIntersectionNode::GetNextEdgeInfo: ve " << ve_id << " bisector intersects ve_start, but not at the same pos as ce." );

#if DEBUG_INTERSECTIONS                        
                        sprintf(layer, "%s_ve_bisector_id_%04d_%s", prefix, ve_id, cons_name );
                        tgShapefile::FromRay( bisector, ce->GetDatasource(), layer, "cons" );
                        
                        sprintf(layer, "%s_ve_start_id_%04d_%s", prefix, ve_id, cons_name );
                        tgShapefile::FromRay( ve_start, ce->GetDatasource(), layer, "cons" );
                        
                        sprintf(layer, "%s_ve_intersect_id_%04d_%s", prefix, ve_id, cons_name );
                        tgShapefile::FromGeod( verify_intersect, ce->GetDatasource(), layer, "pt" );
#endif
                        
                    }
                } else {
                    SG_LOG(SG_GENERAL, LOG_INTERSECTION, "tgIntersectionNode::GetNextEdgeInfo: ve " << ve_id << " bisector DOES NOT intersect ve_start." );

#if DEBUG_INTERSECTIONS                        
                    sprintf(layer, "%s_ve_bisector_id_%04d_%s", prefix, ve_id, cons_name );
                    tgShapefile::FromRay( bisector, ce->GetDatasource(), layer, "cons" );
                    
                    sprintf(layer, "%s_ve_start_id_%04d_%s", prefix, ve_id, cons_name );
                    tgShapefile::FromRay( ve_start, ce->GetDatasource(), layer, "cons" );
#endif                    
                }
            }
            
            SG_LOG(SG_GENERAL, LOG_INTERSECTION, "tgIntersectionNode::GetNextEdgeInfo: verify next edge for ce " << ce_id << " which could be " << ve_id << " has proper intersection with bisector - check BottomLeft" );
            // bisect the shared constraint to verify
            if ( !nxt_info ) {
                SGGeod              verify_intersect;
                char                cons_name[16];

                tgRay ve_start = ve->GetBottomLeftConstraint( ve_originating );

                if ( ve_originating ) {
                    strcpy( cons_name, "BL" );
                } else {
                    strcpy( cons_name, "TR" );
                }

                if ( bisector.Intersect( ve_start, verify_intersect ) ) {
                    if ( SGGeod_isEqual2D( bisect_pos, verify_intersect ) ) {
                        // true found teh correct edge
                        nxt_info = (*nxt);

#if DEBUG_INTERSECTIONS                        
                        sprintf(layer, "%s_ve_bisector_id_%04d_%s", prefix, ve_id, cons_name );
                        tgShapefile::FromRay( bisector, ce->GetDatasource(), layer, "cons" );
                        
                        sprintf(layer, "%s_ve_start_id_%04d_%s", prefix, ve_id, cons_name );
                        tgShapefile::FromRay( ve_start, ce->GetDatasource(), layer, "cons" );
                        
                        sprintf(layer, "%s_ve_intersect_id_%04d_%s", prefix, ve_id, cons_name );
                        tgShapefile::FromGeod( verify_intersect, ce->GetDatasource(), layer, "pt" );
#endif                        
                    } else {
                        SG_LOG(SG_GENERAL, LOG_INTERSECTION, "tgIntersectionNode::GetNextEdgeInfo: ve " << ve_id << " bisector intersects ve_start, but not at the same pos as ce." );

#if DEBUG_INTERSECTIONS                        
                        sprintf(layer, "%s_ve_bisector_id_%04d_%s", prefix, ve_id, cons_name );
                        tgShapefile::FromRay( bisector, ce->GetDatasource(), layer, "cons" );
                        
                        sprintf(layer, "%s_ve_start_id_%04d_%s", prefix, ve_id, cons_name );
                        tgShapefile::FromRay( ve_start, ce->GetDatasource(), layer, "cons" );
                        
                        sprintf(layer, "%s_ve_intersect_id_%04d_%s", prefix, ve_id, cons_name );
                        tgShapefile::FromGeod( verify_intersect, ce->GetDatasource(), layer, "pt" );
#endif
                        
                    }
                } else {
                    SG_LOG(SG_GENERAL, LOG_INTERSECTION, "tgIntersectionNode::GetNextEdgeInfo: ve " << ve_id << " bisector DOES NOT intersect ve_start." );

#if DEBUG_INTERSECTIONS                        
                    sprintf(layer, "%s_ve_bisector_id_%04d_%s", prefix, ve_id, cons_name );
                    tgShapefile::FromRay( bisector, ce->GetDatasource(), layer, "cons" );
                    
                    sprintf(layer, "%s_ve_start_id_%04d_%s", prefix, ve_id, cons_name );
                    tgShapefile::FromRay( ve_start, ce->GetDatasource(), layer, "cons" );
#endif                    
                }
            }

            nxt++;
            if ( nxt == edgeList.end() ) {
                nxt = edgeList.begin();
            }        
        }                                    
    }
    
    return nxt_info;
}

tgIntersectionEdgeInfo* tgIntersectionNode::GetUntexturedEdge( double heading ) 
{
    tgIntersectionEdgeInfo* untextured = NULL;
    
    // for now, just return the next untextured edge info
    for (tgintersectionedgeinfo_it cur = edgeList.begin(); cur != edgeList.end(); cur++) {
        if( !(*cur)->IsTextured() ) {
            untextured = (*cur);
            break;
        }
    }
    
    return untextured;
}

bool tgIntersectionNode::IsTextureComplete( void ) 
{
    bool textureComplete = true;
    
    // for now, just return the next untextured edge info
    for (tgintersectionedgeinfo_it cur = edgeList.begin(); cur != edgeList.end(); cur++) {
        if( !(*cur)->IsTextured() ) {
            textureComplete = false;
            break;
        }
    }
    
    return textureComplete;
}

bool tgIntersectionNode::GetNextConnectedNodeAndEdgeInfo( tgIntersectionEdgeInfo*& info, tgIntersectionNode*& node, std::stack<tgIntersectionNode*>& stack, bool& resetV )
{
    // this function is used when texturing.  it takes an edge info representing the edge we are leaving by.
    // if there are any untextured edges on this current node, then it is pushed onto the stack.
    // we return the node at the other end of edge info.
    // we try to return the continuation of this edge based on heading when traversing the next node
    
    // if the next node is a cap or fully textured node, ( i.e. end of the feature ), then we pop the next node, 
    // and untextured edge info off the stack
    
    // if there isn't anything on the stack, we are done with this network - return true
    // otherwise, return false - we are not done yet
    tgIntersectionNode*     next_node;
    tgIntersectionEdgeInfo* next_info;
    bool                    done = false;
        
    if ( !IsTextureComplete() ) {
        SG_LOG(SG_GENERAL, SG_DEBUG, "tgIntersectionNode::GetNextConnectedNodeAndEdgeInfo push cur node " << node << " ino is " << info );
        
        stack.push(this);
    } else {
        SG_LOG(SG_GENERAL, SG_DEBUG, "tgIntersectionNode::GetNextConnectedNodeAndEdgeInfo cur node " << node << " complete info is " << info );
    }
    
    if ( info->IsOriginating() ) {
        next_node = info->GetEdge()->end;
    } else {
        next_node = info->GetEdge()->start;
    }

    if ( next_node ) {    
        SG_LOG(SG_GENERAL, SG_DEBUG, "tgIntersectionNode::GetNextConnectedNodeAndEdgeInfo next node " << next_node );
    
        // if next node is done, pop
        resetV = false;
        if ( next_node->IsTextureComplete() || next_node->IsCap() ) {
            if ( stack.size() ) {
                next_node = stack.top(); stack.pop();
                next_info = next_node->GetUntexturedEdge();
                resetV    = true;

                SG_LOG(SG_GENERAL, SG_DEBUG, "tgIntersectionNode::GetNextConnectedNodeAndEdgeInfo node complete - popped node (1) " << next_node << " next_info is " << next_info );
            
            } else {
                next_node = NULL;
                next_info = NULL;
                done = true;
            
                SG_LOG(SG_GENERAL, SG_DEBUG, "tgIntersectionNode::GetNextConnectedNodeAndEdgeInfo node complete - stack empty(1) - we're done " );
            }
        } else {
            next_info = next_node->GetUntexturedEdge( info->GetHeading() );

            SG_LOG(SG_GENERAL, SG_DEBUG, "tgIntersectionNode::GetNextConnectedNodeAndEdgeInfo use next node - next info is " << next_info );
            if ( next_info->GetEdge() == NULL ) {
                SG_LOG(SG_GENERAL, SG_DEBUG, "tgIntersectionNode::GetNextConnectedNodeAndEdgeInfo use next node - next info edge is NULL " );
            }                
        }
    } else {
        SG_LOG(SG_GENERAL, SG_DEBUG, "tgIntersectionNode::GetNextConnectedNodeAndEdgeInfo next node is NULL " );        

        if ( stack.size() ) {
            next_node = stack.top(); stack.pop();
            next_info = next_node->GetUntexturedEdge();
            resetV    = true;
            
            SG_LOG(SG_GENERAL, SG_DEBUG, "tgIntersectionNode::GetNextConnectedNodeAndEdgeInfo node complete - popped node (2)" << next_node );
            
        } else {
            next_node = NULL;
            next_info = NULL;
            done = true;
            
            SG_LOG(SG_GENERAL, SG_DEBUG, "tgIntersectionNode::GetNextConnectedNodeAndEdgeInfo node complete - stack empty (2) - we're done " );
        }
    }
    
    info = next_info;
    node = next_node;
    
    return done;
}

void tgIntersectionNode::AddCapEdges( tgIntersectionNodeList& nodelist, tgintersectionedge_list& edgelist ) 
{
    // if this node has just one edge, we may need to add a cap edge
    if ( edgeList.size() == 1 ) {
        tgintersectionedgeinfo_it cur = edgeList.begin();
                
        // we need to get the edge info to determine the edge origination info
        tgIntersectionEdgeInfo* cur_info = (*cur);
        double cur_heading = cur_info->GetHeading();
        
        tgIntersectionEdge* cur_edge = cur_info->GetEdge();
        double cur_length = cur_edge->GetLength();
        
        // euclidean distance is in degrees :( - TODO - need distanceM and distanceE
        if ( cur_length > 1.0 ) {
            // Add a new node 0.5 M away
            tgIntersectionNode* newNode = nodelist.Add( TGEuclidean::direct( position, cur_heading, 0.5 ) );
            tgIntersectionEdge* newEdge = cur_edge->Split( cur_info->IsOriginating(), newNode );
            edgelist.push_back( newEdge );
        }    
    }
}

void tgIntersectionNode::ConstrainEdges( void ) 
{
    tgintersectionedgeinfo_it cur, prev;
    
    switch( edgeList.size() ) {
        case 1:
            // add cap
            GenerateCapRays();
            break;
            
        case 2:
        default:
        {
            // Step 1 
            // Generate the bisector rays and add to the edges
            GenerateBisectRays();            
            break;
        }    
    }
}

// Step 2 - create edge shapes
void tgIntersectionNode::GenerateEdges()
{
    switch( edgeList.size() ) {
        case 1:
            // add cap
            IntersectBisectRays();
            break;
            
        case 2:
        default:
        {              
            // Step 2
            // Intersect constraints / edges
            IntersectBisectRays();
            break;
        }    
    }
}

void tgIntersectionNode::CompleteSpecialIntersections( void )
{
    tgintersectionedgeinfo_it prv, cur, nxt;
    
    switch( edgeList.size() ) {
        //case 1:
        //    // TODO : is it possible for caps to participate in ms intersections?
        //    // looks like sharp edge could cause odd geometry here.
        //    CompleteCap( (*edgeList.begin()) );
        //   break;
            
        default:
            for (cur = edgeList.begin(); cur != edgeList.end(); cur++) {
                nxt = cur;
                nxt++;
                if ( nxt == edgeList.end() ) {
                    nxt = edgeList.begin();
                }

                CompleteMultiSegmentIntersections( (*cur), (*nxt) );
            }
            break;
    }
}

#if 1
void tgIntersectionNode::GenerateBisectRays( void ) 
{
    tgintersectionedgeinfo_it prv, cur, nxt;

    for (cur = edgeList.begin(); cur != edgeList.end(); cur++) {
        // Get previous edge info - with wrap around
        if ( cur == edgeList.begin() ) {
            prv = edgeList.end();
        } else {
            prv = cur;
        }
        prv--;

        // get next edge info - with wrap around
        nxt = cur;
        nxt++;
        if ( nxt == edgeList.end() ) {
            nxt = edgeList.begin();
        }
        
        // we need to get the edge info to determine the edge origination info
        tgIntersectionEdgeInfo* prv_info = (*prv);
        tgIntersectionEdgeInfo* cur_info = (*cur);
        tgIntersectionEdgeInfo* nxt_info = (*nxt);

        double prv_heading;
        double cur_heading;
        double nxt_heading;
        
        double bprv_heading;
        double bnxt_heading;
        
        // the edge info always has heading with respect to the node
        cur_heading = cur_info->GetHeading();
        prv_heading = prv_info->GetHeading();
        nxt_heading = nxt_info->GetHeading();
        
        // we want right turns - so calc angle a,b,c from cur,origin,prv
        bprv_heading = Bisect( position, cur_heading, prv_heading, true );
        tgRay bp_ray = tgRay( position, bprv_heading );

        // we want right turns - so calc angle a,b,c from nxt,origin,cur
        bnxt_heading = Bisect( position, nxt_heading, cur_heading, true );
        tgRay bn_ray = tgRay( position, bnxt_heading );
                
        // Add the bisecting ray between prev and current as a constraint
        tgIntersectionEdge* cur_edge = cur_info->GetEdge();
        
        if ( cur_edge ) {
            double dist = SGGeodesy::distanceM( cur_edge->start->GetPosition(), cur_edge->end->GetPosition() );
            if ( dist < 0.5 ) {
                SG_LOG(SG_GENERAL, LOG_INTERSECTION, "Edge: " << cur_edge->id << " length is " << dist );
            }

            if ( cur_info->IsOriginating() ) {
                //SG_LOG(SG_GENERAL, LOG_INTERSECTION, "GenerateBisectRays: Add BR constraint to edge " << cur_info->GetEdge()->id << " from " << position << " heading " << std::setprecision(16) << bnxt_heading << " from cur (" << std::setprecision(16) << cur_heading << ", originating) and next (" << std::setprecision(16) << nxt_heading << ")" );
                cur_edge->AddBottomRightConstraint(bn_ray);

                //SG_LOG(SG_GENERAL, LOG_INTERSECTION, "GenerateBisectRays: Add BL constraint to edge " << cur_info->GetEdge()->id << " from " << position << " heading " << std::setprecision(16) << bprv_heading << " from cur (" << std::setprecision(16) << cur_heading << ", originating) and prev (" << std::setprecision(16) << prv_heading << ")" );
                cur_edge->AddBottomLeftConstraint(bp_ray);
            } else {
                //SG_LOG(SG_GENERAL, LOG_INTERSECTION, "GenerateBisectRays: Add TL constraint to edge " << cur_info->GetEdge()->id << " from " << position << " heading " << std::setprecision(16) << bnxt_heading << " from cur (" << std::setprecision(16) << cur_heading << ", originating) and next (" << std::setprecision(16) << nxt_heading << ")" );
                cur_edge->AddTopLeftConstraint(bn_ray);

                //SG_LOG(SG_GENERAL, LOG_INTERSECTION, "GenerateBisectRays: Add TR constraint to edge " << cur_info->GetEdge()->id << " from " << position << " heading " << std::setprecision(16) << bprv_heading << " from cur (" << std::setprecision(16) << cur_heading << ", originating) and prev (" << std::setprecision(16) << prv_heading << ")" );
                cur_edge->AddTopRightConstraint(bp_ray);
            }
        } else {
            SG_LOG(SG_GENERAL, LOG_INTERSECTION, "GenerateBisectRays: no cur edge!!! " );
        }
    }
}
#else
void tgIntersectionNode::GenerateBisectRays( void ) 
{
    // use just CGAL for precision
    tgintersectionedgeinfo_it prv, cur, nxt;

    for (cur = edgeList.begin(); cur != edgeList.end(); cur++) {
        // Get previous edge info - with wrap around
        if ( cur == edgeList.begin() ) {
            prv = edgeList.end();
        } else {
            prv = cur;
        }
        prv--;
        
        // get next edge info - with wrap around
        nxt = cur;
        nxt++;
        if ( nxt == edgeList.end() ) {
            nxt = edgeList.begin();
        }
        
        // we need to get the edge info to determine the edge origination info
        tgIntersectionEdgeInfo* prv_info = (*prv);
        tgIntersectionEdgeInfo* cur_info = (*cur);
        tgIntersectionEdgeInfo* nxt_info = (*nxt);
          
        tgRay prv_heading = prv_info->GetDirectionRay();
        tgRay cur_heading = cur_info->GetDirectionRay();
        tgRay nxt_heading = nxt_info->GetDirectionRay();
          
        tgRay bp_ray = Bisect( cur_heading, prv_heading, true );        
        tgRay bn_ray = Bisect( nxt_heading, cur_heading, true );
        
        // Add the bisecting ray between prev and current as a constraint
        tgIntersectionEdge* cur_edge = cur_info->GetEdge();
        
        if ( cur_edge ) {
            double dist = SGGeodesy::distanceM( cur_edge->start->GetPosition(), cur_edge->end->GetPosition() );
            if ( dist < 0.5 ) {
                SG_LOG(SG_GENERAL, LOG_INTERSECTION, "Edge: " << cur_edge->id << " length is " << dist );
            }
            
            if ( cur_info->IsOriginating() ) {
                //SG_LOG(SG_GENERAL, LOG_INTERSECTION, "GenerateBisectRays: Add BR constraint to edge " << cur_info->GetEdge()->id << " from " << position << " heading " << std::setprecision(16) << bnxt_heading << " from cur (" << std::setprecision(16) << cur_heading << ", originating) and next (" << std::setprecision(16) << nxt_heading << ")" );
                cur_edge->AddBottomRightConstraint(bn_ray);
                
                //SG_LOG(SG_GENERAL, LOG_INTERSECTION, "GenerateBisectRays: Add BL constraint to edge " << cur_info->GetEdge()->id << " from " << position << " heading " << std::setprecision(16) << bprv_heading << " from cur (" << std::setprecision(16) << cur_heading << ", originating) and prev (" << std::setprecision(16) << prv_heading << ")" );
                cur_edge->AddBottomLeftConstraint(bp_ray);
            } else {
                //SG_LOG(SG_GENERAL, LOG_INTERSECTION, "GenerateBisectRays: Add TL constraint to edge " << cur_info->GetEdge()->id << " from " << position << " heading " << std::setprecision(16) << bnxt_heading << " from cur (" << std::setprecision(16) << cur_heading << ", originating) and next (" << std::setprecision(16) << nxt_heading << ")" );
                cur_edge->AddTopLeftConstraint(bn_ray);
                
                //SG_LOG(SG_GENERAL, LOG_INTERSECTION, "GenerateBisectRays: Add TR constraint to edge " << cur_info->GetEdge()->id << " from " << position << " heading " << std::setprecision(16) << bprv_heading << " from cur (" << std::setprecision(16) << cur_heading << ", originating) and prev (" << std::setprecision(16) << prv_heading << ")" );
                cur_edge->AddTopRightConstraint(bp_ray);
            }
        } else {
            SG_LOG(SG_GENERAL, LOG_INTERSECTION, "GenerateBisectRays: no cur edge!!! " );
        }
    }
}
#endif

void tgIntersectionNode::GenerateCapRays( void ) 
{
    tgintersectionedgeinfo_it cur = edgeList.begin();
                
    // we need to get the edge info to determine the edge origination info
    tgIntersectionEdgeInfo* cur_info = (*cur);

    // for caps, use geodesy to make nice 90 deg angles
    double cur_heading = cur_info->GetGeodesyHeading();    
    
    tgRay l_ray = tgRay( position, SGMiscd::normalizePeriodic( 0, 360, cur_heading-90 ) );
    tgRay r_ray = tgRay( position, SGMiscd::normalizePeriodic( 0, 360, cur_heading+90 ) );
                
    // Add the cap rays as constraints
    tgIntersectionEdge* cur_edge = cur_info->GetEdge();
        
    if ( cur_edge ) {
        if ( cur_info->IsOriginating() ) {
            cur_edge->AddBottomRightConstraint(r_ray);
            cur_edge->AddBottomLeftConstraint(l_ray);
        } else {
            cur_edge->AddTopLeftConstraint(r_ray);
            cur_edge->AddTopRightConstraint(l_ray);
        }
    } else {
        SG_LOG(SG_GENERAL, LOG_INTERSECTION, "GenerateCapRays: no cur edge!!! " );
    }
}

// this should be an edge traversal, not node traversal
// TODO : Add a tgIntersectionEdge function to handle each constraint
// we do the same thing 4 times in this function
void tgIntersectionNode::IntersectBisectRays( void )
{
    tgintersectionedgeinfo_it cur;
    
    for (cur = edgeList.begin(); cur != edgeList.end(); cur++) { 
        tgIntersectionEdgeInfo* cur_info = (*cur);
        tgIntersectionEdge*     cur_edge = cur_info->GetEdge();
        
        if ( cur_edge ) {
            cur_edge->IntersectConstraintsAndSides( cur_info ); 
        } else {
            SG_LOG(SG_GENERAL, SG_ALERT, "IntersectBisectRays: NO cur edge" );
        }
    }
}

// this may or may not be multisegment

// TODO: don't set constraints until we know we are successful
// 1) we reach intersection with a side
// 2) the edges merge again - intersection at a node
// 3) and edge ends at a cap

void tgIntersectionNode::CompleteMultiSegmentIntersections( tgIntersectionEdgeInfo* cur_info, tgIntersectionEdgeInfo* nxt_info )
{
    char   original_int_name[64];
    
    // list of edges we traverse
    tgintersectionedge_list applyList;
    
    // current position and heading of the bisector
    double ce_heading = 0.0, ne_heading = 0.0, bisect_heading;
    SGGeod bisect_position = position;

    std::list<SGGeod> ce_constraint;
    std::list<SGGeod> ne_constraint;

    std::list<SGGeod> ce_projectlist;
    std::list<SGGeod> ne_projectlist;
    
    // remember how to traverse current and next edge iterators (prev or next)
    bool ce_originating;
    bool ne_originating;
    
    tgIntersectionEdge* ce;
    tgIntersectionEdge* ne;
    
    char ce_layer[128];
    char ne_layer[128];
    
    char ce_cons_name[64];
    char ne_cons_name[64];
    
    int  cur_iteration = 0;

    SG_LOG(SG_GENERAL, LOG_INTERSECTION, "tgIntersectionNode::CompleteIntersection: ENTER: current edge " << cur_info->GetEdge()->id << " and next edge " << nxt_info->GetEdge()->id );
    
    // iterate until the end of the intersection is found, or we discover this is NOT multisegment
    bool done  = false;
    bool apply = false;

    sprintf( original_int_name, "INT_CE_%04ld_WITH_NE_%04ld", cur_info->GetEdge()->id, nxt_info->GetEdge()->id );
    
    ce_originating = cur_info->IsOriginating();
    ne_originating = nxt_info->IsOriginating();
    
    ce = cur_info->GetEdge();
    ne = nxt_info->GetEdge();

    int ce_id = ce->id;
    int ne_id = ne->id;
    
    while (!done) {        
        // first, calculate the bisect heading
        ce_heading = ce->GetHeading( ce_originating );
        ne_heading = ne->GetHeading( ne_originating );
        bisect_heading = Bisect( bisect_position, ne_heading, ce_heading, true );
        
        if ( ce_originating ) {
            strcpy( ce_cons_name, "BotRight" );
        } else {
            strcpy( ce_cons_name, "TopLeft" );
        }
        
        if ( ne_originating ) {
            strcpy( ne_cons_name, "BotLeft" );
        } else {
            strcpy( ne_cons_name, "TopRight" );
        }

        SG_LOG(SG_GENERAL, LOG_INTERSECTION, "tgIntersectionNode::CompleteIntersection: ce " << ce_id << " heading is " << ce_heading << " ne " << ne_id << " heading is " << ne_heading << " bisct heading is " << bisect_heading);

        // add the bisect position to constraints        
        ce_constraint.push_back( bisect_position );
        SG_LOG(SG_GENERAL, LOG_INTERSECTION, "tgIntersectionNode::CompleteIntersection: ce " << ce_id << " Added intersection position to " << ce_cons_name << " constraint.  size is " << ce_constraint.size() );

        ne_constraint.push_back( bisect_position );
        SG_LOG(SG_GENERAL, LOG_INTERSECTION, "tgIntersectionNode::CompleteIntersection: ne " << ne_id << " Added intersection position to " << ne_cons_name << " constraint.  size is " << ne_constraint.size() );
                
        tgRay bisector( bisect_position, bisect_heading );
        
        // find the edge whose constraint we intersect with first
        // current top right, or next top left (w.r.t. originating)
        tgRay ce_end        = ce->GetTopRightConstraint( ce_originating );
        tgRay ne_end        = ne->GetTopLeftConstraint( ne_originating );
        
        tgLine ce_side      = ce->GetRightSide( ce_originating );
        tgLine ne_side      = ne->GetLeftSide(  ne_originating );
        
        SGGeod ce_end_intersect, ne_end_intersect;
        double ce_end_dist = -1000.00, ne_end_dist = -1000.00;
        
        SGGeod ce_side_intersect, ne_side_intersect;
        double ce_side_dist = -1000.00, ne_side_dist = -1000.00;
        
        // add the debug to correct edges
        sprintf( ce_layer, "%s_%03ld_BISECTOR_iter_%d", original_int_name, ne->id, cur_iteration );
        sprintf( ne_layer, "%s_%03ld_BISECTOR_iter_%d", original_int_name, ce->id, cur_iteration );
        
#if DEBUG_INTERSECTIONS
        tgShapefile::FromRay( bisector, ce->GetDatasource(), ce_layer, "bisector" );
        tgShapefile::FromRay( bisector, ne->GetDatasource(), ne_layer, "bisector" );          
#endif

        sprintf( ce_layer, "%s_%03ld_INTERSECTIONS_iter_%d", original_int_name, ne->id, cur_iteration );
        sprintf( ne_layer, "%s_%03ld_INTERSECTIONS_iter_%d", original_int_name, ce->id, cur_iteration );
        
        if ( bisector.Intersect( ce_end, ce_end_intersect ) ) {
#if DEBUG_INTERSECTIONS                                    
            tgShapefile::FromGeod( ce_end_intersect, ce->GetDatasource(), ce_layer, "ce_end_intersect" );
#endif            
            // if we have a point intersection, what is the distance between 
            // the node and the intersection
            ce_end_dist = TGEuclidean::distanceM( bisect_position, ce_end_intersect );
        } else {
            SG_LOG(SG_GENERAL, LOG_INTERSECTION, "tgIntersectionNode::CompleteIntersection: ERROR - can't intersect bisector " << ce_layer << " with ce " << ce_id << " top right constraint");
            
            done = true;
        }
        
        if ( bisector.Intersect( ne_end, ne_end_intersect ) ) {
#if DEBUG_INTERSECTIONS                        
            tgShapefile::FromGeod( ne_end_intersect, ce->GetDatasource(), ce_layer, "ne_end_intersect" );
#endif
            // if we have a point intersection, what is the distance between 
            // the node and the intersection
            ne_end_dist = TGEuclidean::distanceM( bisect_position, ne_end_intersect );
        } else {
            SG_LOG(SG_GENERAL, LOG_INTERSECTION, "tgIntersectionNode::CompleteIntersection: ERROR - can't intersect bisector " << ce_layer << " with ne " << ne_id << " top left constraint");
            
            done = true;
        }
        
        if ( bisector.Intersect( ce_side, ce_side_intersect ) ) {
#if DEBUG_INTERSECTIONS                        
            tgShapefile::FromGeod( ce_side_intersect, ce->GetDatasource(), ce_layer, "ce_side_intersect" );
#endif
            // if we have a point intersection, what is the distance between 
            // the node and the intersection
            ce_side_dist = TGEuclidean::distanceM( bisect_position, ce_side_intersect );
        }
        
        if ( bisector.Intersect( ne_side, ne_side_intersect ) ) {
#if DEBUG_INTERSECTIONS                        
            tgShapefile::FromGeod( ne_side_intersect, ce->GetDatasource(), ce_layer, "ne_side_intersect" );
#endif
            // if we have a point intersection, what is the distance between 
            // the node and the intersection
            ne_side_dist = TGEuclidean::distanceM( bisect_position, ne_side_intersect );
        }

        sprintf( ce_layer, "%s_%03ld_GETNEXT_iter_%d", original_int_name, ne->id, cur_iteration );
        sprintf( ne_layer, "%s_%03ld_GETPREV_iter_%d", original_int_name, ce->id, cur_iteration );
        
        if ( ce_end_dist > 0.0 && ne_end_dist > 0.0 ) {
            // if the side intersection distances are lower than the constraint distances, than we are done
            if ( (ce_side_dist >= 0.0) && (ce_side_dist < ce_end_dist) && (ne_side_dist >= 0.0) && (ne_side_dist < ne_end_dist) ) {
                
                // verify multiseg ( current_iteration > 1 )
                if ( cur_iteration > 0 ) {
                    SG_LOG(SG_GENERAL, LOG_INTERSECTION, "tgIntersectionNode::CompleteIntersection: all sides are closer than ends, and we've traversed at least one segment - multisegment complete");
                    
                    if ( cur_iteration == 1 ) {
                        
                    }
                    
                    ce_constraint.push_back(ce_side_intersect);
                    ce_projectlist.push_back(ce_side_intersect);
                    SG_LOG(SG_GENERAL, LOG_INTERSECTION, "tgIntersectionNode::CompleteIntersection: ce " << ce_id << " Adding intersection with side to " << ce_cons_name << " constraint.  size is " << ce_constraint.size() );
                
                    ne_constraint.push_back(ne_side_intersect);
                    ne_projectlist.push_back(ne_side_intersect);
                    SG_LOG(SG_GENERAL, LOG_INTERSECTION, "tgIntersectionNode::CompleteIntersection: ne " << ne_id << " Adding intersection with side to " << ne_cons_name << " constraint.  size is " << ne_constraint.size() );
                
                    // set the constraint ( and flag that we intersected the sides )
                    SG_LOG(SG_GENERAL, LOG_INTERSECTION, "tgIntersectionNode::CompleteIntersection: ce " << ce_id << " Saving " << ce_cons_name << " Constraint with " << ce_constraint.size() << " nodes "); 
                    ce->SetRightConstraint( ce_originating, ce_constraint );
                    ce->SetRightProjectList( ce_originating, ce_projectlist );
                    applyList.push_back( ce );
                    
                    SG_LOG(SG_GENERAL, LOG_INTERSECTION, "tgIntersectionNode::CompleteIntersection: ne " << ne_id << " Saving " << ne_cons_name << " Constraint with " << ne_constraint.size() << " nodes "); 
                    ne->SetLeftConstraint( ne_originating, ne_constraint );
                    ne->SetLeftProjectList( ne_originating, ne_projectlist );
                    applyList.push_back( ne );
                    
                    done = true;
                    apply = true;
                } else {
                    SG_LOG(SG_GENERAL, LOG_INTERSECTION, "tgIntersectionNode::CompleteIntersection: all sides are closer than ends - looks like a normal edge - not multisegment");
                    done = true;
                }
            } else {
                // Finish the correct edge, and increment to next unfinished edge
                if ( ce_end_dist < ne_end_dist ) {
                    SG_LOG(SG_GENERAL, LOG_INTERSECTION, "tgIntersectionNode::CompleteIntersection: ce " << ce_id << " end distance < ne " << ne_id << " distance - finish " << ce_id );
                    
                    bisect_position = ce_end_intersect;
                                        
                    SG_LOG(SG_GENERAL, LOG_INTERSECTION, "tgIntersectionNode::CompleteIntersection: ce " << ce_id << " Adding end bisect position to " << ce_cons_name << " constraint.  size is " << ce_constraint.size() );
                    ce_constraint.push_back(bisect_position);
                    
                    // ne will get bisector position added to it's constraint at the beginning of the next loop.
                    // But we want to project this vertex to the other side of next edge, so remember it
                    ne_projectlist.push_back(bisect_position);
                    
                    if ( ce_originating ) {
                        // get the next edge (CCW) from the END node : not enough - need to find an edge that intersects the same bisector at the same point...
                        // this function should return NULL if such an edge does not exist
                        cur_info = ce->end->GetNextEdgeInfo( cur_info, bisector, bisect_position, ce_layer );
                    } else {
                        // get the next edge (CCW) from the START node
                        cur_info = ce->start->GetNextEdgeInfo( cur_info, bisector, bisect_position, ce_layer );
                    }
                    
                    if (cur_info == NULL) {
                        SG_LOG(SG_GENERAL, LOG_INTERSECTION, "tgIntersectionNode::CompleteIntersection: can't find next edge for ce " << ce_id );
                        done = true;
                    } else {
                        // set the constraint and project list
                        SG_LOG(SG_GENERAL, LOG_INTERSECTION, "tgIntersectionNode::CompleteIntersection: " << ce->id << " Saving " << ce_cons_name << " Constraint with " << ce_constraint.size() << " nodes "); 
                        ce->SetRightConstraint( ce_originating, ce_constraint );
                        ce->SetRightProjectList( ce_originating, ce_projectlist );
                        applyList.push_back( ce );
                        
                        ce_constraint.clear();
                        ce_projectlist.clear();

                        ce_originating  = cur_info->IsOriginating();
                        ce              = cur_info->GetEdge();
                        ce_id           = ce->id;
                    }
                } else {
                    SG_LOG(SG_GENERAL, LOG_INTERSECTION, "tgIntersectionNode::CompleteIntersection: ne " << ne_id << " end distance < ce " << ce_id << " distance - finish " << ne_id );

                    bisect_position = ne_end_intersect;
                    
                    ne_constraint.push_back( ne_end_intersect );
                    SG_LOG(SG_GENERAL, LOG_INTERSECTION, "tgIntersectionNode::CompleteIntersection: ne " << ne_id << " Added end bisect position to " << ne_cons_name << " constraint.  size is " << ne_constraint.size() );
                    
                    // ce will get bisector position added to it's constraint at the beginning of the next loop.
                    // But we want to project this vertex to the other side of cur edge, so remember it
                    ce_projectlist.push_back(bisect_position);

                    if ( ne_originating ) {
                        // get the prev edge (CW) from the END node
                        nxt_info = ne->end->GetPrevEdgeInfo( nxt_info, bisector, bisect_position, ne_layer );
                    } else {
                        // get the prev edge (CW) from the START node
                        nxt_info = ne->start->GetPrevEdgeInfo( nxt_info, bisector, bisect_position, ne_layer );
                    }
                    
                    if (nxt_info == NULL) {
                        SG_LOG(SG_GENERAL, LOG_INTERSECTION, "tgIntersectionNode::CompleteIntersection: can't find next edge for ne " << ne_id );
                        done = true;
                    } else {
                        // set the constraint
                        SG_LOG(SG_GENERAL, LOG_INTERSECTION, "tgIntersectionNode::CompleteIntersection: " << ne->id << " Saving " << ne_cons_name << " Constraint with " << ne_constraint.size() << " nodes "); 
                        ne->SetLeftConstraint( ne_originating, ne_constraint );
                        ne->SetLeftProjectList( ne_originating, ne_projectlist );
                        applyList.push_back( ne );
                        
                        ne_constraint.clear();
                        ne_projectlist.clear();
                        
                        ne_originating  = nxt_info->IsOriginating();
                        ne              = nxt_info->GetEdge();
                        ne_id           = ne->id;
                    }
                }
            }
        } else {
            SG_LOG(SG_GENERAL, LOG_INTERSECTION, "tgIntersectionNode::CompleteIntersection: bisector does not intersect both ends");
            done = true;
        }        
        
        cur_iteration++;
    }
    
    for ( tgintersectionedge_it it = applyList.begin(); it != applyList.end(); it++ ) {
        (*it)->ApplyConstraint( apply );
    }
}

void tgIntersectionNode::CompleteCap( tgIntersectionEdgeInfo* cur_info )
{
    tgIntersectionEdge* ce = cur_info->GetEdge();
    bool ce_originating = cur_info->IsOriginating();
    
    std::list<SGGeod> cons;
    
    if ( ce_originating ) {
        cons.clear();
        cons.push_back( ce->start->GetPosition() );
        cons.push_back( ce->conBotRight );
        ce->SetRightConstraint( ce_originating, cons );
                
        cons.clear();
        cons.push_back( ce->start->GetPosition() );
        cons.push_back( ce->conBotLeft );
        ce->SetLeftConstraint( ce_originating, cons );        
    } else {
        cons.clear();
        cons.push_back( ce->end->GetPosition() );
        cons.push_back( ce->conTopLeft );
        ce->SetRightConstraint( ce_originating, cons );
        
        cons.clear();
        cons.push_back( ce->end->GetPosition() );
        cons.push_back( ce->conTopRight );
        ce->SetLeftConstraint( ce_originating, cons );        
    }
    
    ce->ApplyConstraint( true );    
}


