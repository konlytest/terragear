#include <simgear/debug/logstream.hxx>
#include "linked_objects.hxx"

Windsock::Windsock( char* definition )
{
    sscanf(definition, "%lf %lf %d", &lat, &lon, &lit);

    SG_LOG(SG_GENERAL, SG_DEBUG, "Read Windsock: (" << lon << "," << lat << ") lit: " << lit  );
}

Beacon::Beacon( char* definition )
{
    sscanf(definition, "%lf %lf %d", &lat, &lon, &code);

    SG_LOG(SG_GENERAL, SG_DEBUG, "Read Beacon: (" << lon << "," << lat << ") code: " << code  );
}

Sign::Sign( char* definition )
{
    char sgdef[256];
    double def_heading;

    sscanf(definition, "%lf %lf %lf %d %d %s", &lat, &lon, &def_heading, &reserved, &size, sgdef );

    // 850 format sign heading is the heading which points away from the visible numbers
    // Flightgear wants the heading to be the heading in which the sign is read
    heading = -def_heading + 360.0;

    SG_LOG(SG_GENERAL, SG_DEBUG, "Read Sign: (" << lon << "," << lat << ") heading " << def_heading << " size " << size << " definition: " << sgdef << " calc view heading: " << heading );

    sgn_def = sgdef;
}

