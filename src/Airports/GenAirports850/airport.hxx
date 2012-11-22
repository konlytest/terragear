#ifndef _AIRPORT_H_
#define _AIRPORT_H_

#include <simgear/timing/timestamp.hxx>
#include <simgear/math/sg_types.hxx>

#include "runway.hxx"
#include "object.hxx"
#include "helipad.hxx"
#include "taxiway.hxx"
#include "closedpoly.hxx"
#include "linearfeature.hxx"
#include "linked_objects.hxx"

typedef std::map<std::string, std::vector<int>, std::less<std::string> > debug_map;
typedef debug_map::iterator debug_map_iterator;
typedef debug_map::const_iterator debug_map_const_iterator;

class Airport
{
public:
    Airport( int c, char* def);
    ~Airport();

    void AddRunway( Runway* runway )
    {
        runways.push_back( runway );
    }

    void AddWaterRunway( WaterRunway* waterrunway )
    {
        waterrunways.push_back( waterrunway );
    }

    void AddObj( LightingObj* lightobj )
    {
        lightobjects.push_back( lightobj );
    }

    void AddHelipad( Helipad* helipad )
    {
        helipads.push_back( helipad );
    }

    void AddTaxiway( Taxiway* taxiway )
    {
        taxiways.push_back( taxiway );
    }

    void AddPavement( ClosedPoly* pavement )
    {
        pavements.push_back( pavement );
    }

    void AddFeature( LinearFeature* feature )
    {
        features.push_back( feature );
    }

    void AddFeatures( FeatureList* feature_list )
    {
        for (unsigned int i=0; i<feature_list->size(); i++)
        {
            features.push_back( feature_list->at(i) );
        }
    }

    int NumFeatures( void )
    {
        return features.size();
    }

    void AddBoundary( ClosedPoly* bndry )
    {
        boundary.push_back( bndry );
    }

    void AddWindsock( Windsock* windsock )
    {
        windsocks.push_back( windsock );
    }

    void AddBeacon( Beacon* beacon )
    {
        beacons.push_back( beacon );
    }

    void AddSign( Sign* sign )
    {
        signs.push_back( sign );
    }

    std::string GetIcao( )
    {
        return icao;
    }

    void GetBuildTime( SGTimeStamp& tm )
    {
        tm = build_time;
    }

    void GetTriangulationTime( SGTimeStamp& tm )
    {
        tm = triangulation_time;
    }

    void GetCleanupTime( SGTimeStamp& tm )
    {
        tm = cleanup_time;
    }

    void merge_slivers( tgpolygon_list& polys, tgcontour_list& slivers );
    void BuildBtg( const std::string& root, const string_list& elev_src );

    void DumpStats( void );

    void set_debug( std::string& path,
                    debug_map& dbg_runways, 
                    debug_map& dbg_pavements,
                    debug_map& dbg_taxiways,
                    debug_map& dbg_features ) {
        debug_path      = path;
        debug_runways   = dbg_runways;
        debug_pavements = dbg_pavements;
        debug_taxiways  = dbg_taxiways;
        debug_features  = dbg_features;
    };

    bool isDebugRunway  ( int i );
    bool isDebugPavement( int i );
    bool isDebugTaxiway ( int i );
    bool isDebugFeature ( int i );

private:
    int          code;               // airport, heliport or sea port
    int          altitude;           // in meters
    std::string  icao;               // airport code
    std::string  description;        // description

    PavementList    pavements;
    FeatureList     features;
    RunwayList      runways;
    WaterRunwayList waterrunways;
    TaxiwayList     taxiways;
    LightingObjList lightobjects;
    WindsockList    windsocks;
    BeaconList      beacons;
    SignList        signs;
    HelipadList     helipads;
    PavementList    boundary;

    // stats
    SGTimeStamp build_time;
    SGTimeStamp cleanup_time;
    SGTimeStamp triangulation_time;

    // debug
    std::string          debug_path;
    debug_map       debug_runways;
    debug_map       debug_pavements;
    debug_map       debug_taxiways;
    debug_map       debug_features;
};

typedef std::vector <Airport *> AirportList;

#endif
