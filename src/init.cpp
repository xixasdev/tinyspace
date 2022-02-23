// init.cpp (tiny space v3 - experiment: snapshot serialization) (C++11)
// AUTHOR: xixas | DATE: 2022.02.21 | LICENSE: WTFPL/PDM/CC0... your choice


#include "init.hpp"

#include <map>

#include "constants.hpp"
#include "models.hpp"
#include "rand.hpp"


namespace tinyspace {


sectors_t initSectors( v2size_t const& bounds, dimensions_t const& size )
{
    sectors_t sectors;

    auto& rowCount = bounds.y;
    auto& colCount = bounds.x;

    // Create sectors
    sectors.reserve( rowCount );
    {
        char name[ 4 ];
        for ( size_t row = 0; row < rowCount; ++row )
        {
            sectors.emplace_back();
            sectors[ row ].reserve( colCount );
            for ( size_t col = 0; col < colCount; ++col )
            {
                sprintf( name, "%c%02zu", 'A'+col, 1+row );
                sectors[ row ].emplace_back( pair<size_t, size_t>{ row, col }, name, size );
            }
        }
    }

    // Determine neighbors
    for ( size_t row = 0; row < rowCount; ++row )
    {
        for ( size_t col = 0; col < colCount; ++col )
        {
            auto& sector = sectors[ row ][ col ];
            auto& neighbors = sector.neighbors;

            // determine adjacent sectors
            neighbors.north = ( row == 0 )            ? nullptr : &sectors[ row-1 ][ col ];
            neighbors.south = ( row == rowCount - 1 ) ? nullptr : &sectors[ row+1 ][ col ];
            neighbors.west  = ( col == 0 )            ? nullptr : &sectors[ row ][ col-1 ];
            neighbors.east  = ( col == colCount - 1 ) ? nullptr : &sectors[ row ][ col+1 ];
        }
    }

    return sectors;
}


jumpgates_t initJumpgates( sectors_t& sectors, bool useJumpgates )
{
    jumpgates_t jumpgatesBuf;
    if ( ! useJumpgates )
    {
        return jumpgatesBuf;
    }

    jumpgatesBuf.reserve( sectors.size() * sectors[ 0 ].size() * 4 );

    auto addJumpgateNorth = [ &jumpgatesBuf ]( Sector& sector ) -> bool
    {
        if ( sector.jumpgates.north ||
             ! sector.neighbors.north ||
             sector.neighbors.north->jumpgates.south )
        {
            return false;
        }

        Sector& neighbor = *sector.neighbors.north;

        auto localPos  = randPosition( GATE_RANGE_NORTH );
        auto remotePos = randPosition( GATE_RANGE_SOUTH );

        Jumpgate& localJumpgate  = *( jumpgatesBuf.emplace(
            jumpgatesBuf.end(), &sector, localPos, nullptr ));

        Jumpgate& remoteJumpgate = *( jumpgatesBuf.emplace(
            jumpgatesBuf.end(), &neighbor, remotePos, &localJumpgate ));

        localJumpgate.target     = &remoteJumpgate;
        sector.jumpgates.north   = &localJumpgate;
        neighbor.jumpgates.south = &remoteJumpgate;

        return true;
    };

    auto addJumpgateEast = [ &jumpgatesBuf ]( Sector& sector ) -> bool
    {
        if ( sector.jumpgates.east ||
             ! sector.neighbors.east ||
             sector.neighbors.east->jumpgates.west )
        {
            return false;
        }

        Sector& neighbor = *sector.neighbors.east;

        auto localPos  = randPosition( GATE_RANGE_EAST );
        auto remotePos = randPosition( GATE_RANGE_WEST );

        Jumpgate& localJumpgate  = *( jumpgatesBuf.emplace(
            jumpgatesBuf.end(), &sector, localPos, nullptr ));

        Jumpgate& remoteJumpgate = *( jumpgatesBuf.emplace(
            jumpgatesBuf.end(), &neighbor, remotePos, &localJumpgate ));

        localJumpgate.target    = &remoteJumpgate;
        sector.jumpgates.east   = &localJumpgate;
        neighbor.jumpgates.west = &remoteJumpgate;

        return true;
    };

    auto addJumpgateSouth = [ &jumpgatesBuf ]( Sector& sector ) -> bool
    {
        if ( sector.jumpgates.south ||
             ! sector.neighbors.south ||
             sector.neighbors.south->jumpgates.north )
        {
            return false;
        }

        Sector& neighbor = *sector.neighbors.south;

        auto localPos  = randPosition( GATE_RANGE_SOUTH );
        auto remotePos = randPosition( GATE_RANGE_NORTH );

        Jumpgate& localJumpgate  = *( jumpgatesBuf.emplace(
            jumpgatesBuf.end(), &sector, localPos, nullptr ));

        Jumpgate& remoteJumpgate = *( jumpgatesBuf.emplace(
            jumpgatesBuf.end(), &neighbor, remotePos, &localJumpgate ));

        localJumpgate.target     = &remoteJumpgate;
        sector.jumpgates.south   = &localJumpgate;
        neighbor.jumpgates.north = &remoteJumpgate;

        return true;
    };

    auto addJumpgateWest = [ &jumpgatesBuf ]( Sector& sector ) -> bool
    {
        if ( sector.jumpgates.west ||
             ! sector.neighbors.west ||
             sector.neighbors.west->jumpgates.east )
        {
            return false;
        }

        Sector& neighbor = *sector.neighbors.west;

        auto localPos  = randPosition( GATE_RANGE_WEST );
        auto remotePos = randPosition( GATE_RANGE_EAST );

        Jumpgate& localJumpgate  = *( jumpgatesBuf.emplace(
            jumpgatesBuf.end(), &sector, localPos, nullptr ));

        Jumpgate& remoteJumpgate = *( jumpgatesBuf.emplace(
            jumpgatesBuf.end(), &neighbor, remotePos, &localJumpgate ));

        localJumpgate.target    = &remoteJumpgate;
        sector.jumpgates.west   = &localJumpgate;
        neighbor.jumpgates.east = &remoteJumpgate;

        return true;
    };

    auto rowCount = sectors.size();
    auto colCount = sectors[0].size();

    // determine jumpgates
    for ( size_t row = 0; row < rowCount; ++row )
    {
        for ( size_t col = 0; col < colCount; ++col)
        {
            auto& sector       = sectors[row][col];
            auto& neighbors    = sector.neighbors;
            auto& jumpgates    = sector.jumpgates;

            int jumpgatesCount = 1 + ( rand() % sector.neighbors.count() ) - sector.jumpgates.count();

            // Populate jumpgates in an XY-forward direction
            while ( jumpgatesCount > 0 &&
                    ( ( neighbors.south && !jumpgates.south ) ||
                      ( neighbors.west  && !jumpgates.west )))
            {
                // Randomly populate south jumpgate
                if ( jumpgatesCount > 0 &&
                     neighbors.south &&
                     ! jumpgates.south &&
                     rand() % 2 &&
                     addJumpgateSouth( sector ))
                {
                    --jumpgatesCount;
                }

                // Randomly populate west jumpgate
                if ( jumpgatesCount > 0 &&
                     neighbors.west &&
                     ! jumpgates.west &&
                     rand() % 2 &&
                     addJumpgateWest( sector ))
                {
                    --jumpgatesCount;
                }
            }

            // Sometimes the last sector is unreachable due to neither the
            // north or west neighbors having given it a jumpgate.
            //
            // This block catches any time no jumpgates are defined.
            // (just in case)
            //
            // If there are no jumpgates, place a joining one in whichever
            // neighboring sector has the fewest jumpgates.
            if ( ! jumpgates.count() )
            {
                auto allNeighbors = neighbors.all();
                auto it = allNeighbors.begin();
                if ( it != allNeighbors.end() )
                {
                    Sector* selectedNeighbor = *it;
                    int minJumpgates = selectedNeighbor->jumpgates.count();
                    while ( it != allNeighbors.end() )
                    {
                        Sector* neighbor = *it;
                        int count = neighbor->jumpgates.count();
                        if ( count < minJumpgates )
                        {
                            selectedNeighbor = neighbor;
                            minJumpgates = count;
                        }
                        ++it;
                    }

                    if ( selectedNeighbor == neighbors.north )
                    {
                        addJumpgateNorth( sector );
                    }
                    else if ( selectedNeighbor == neighbors.east )
                    {
                        addJumpgateEast( sector );
                    }
                    else if ( selectedNeighbor == neighbors.south )
                    {
                        addJumpgateSouth( sector );
                    }
                    else if ( selectedNeighbor == neighbors.west )
                    {
                        addJumpgateWest( sector );
                    }
                }
            }
        }
    }

    return jumpgatesBuf;
}


stations_t initStations( sectors_t& sectors )
{
    stations_t stations;

    size_t rowCount = sectors.size();
    size_t colCount = sectors[ 0 ].size();

    for ( size_t row = 0; row < rowCount; ++row )
    {
        for ( size_t col = 0; col < colCount; ++col )
        {
            Sector& sector = sectors[ row ][ col ];
            float t = randFloat( 1.f + NO_STATIONS_FREQUENCY );
            if ( t > NO_STATIONS_FREQUENCY ) 
            {
                int stationCount = 1;
                for ( size_t i = MAX_STATIONS_PER_SECTOR; i > 1; --i )
                {
                    if ( t > NO_STATIONS_FREQUENCY + 1.f - 1.f/i )
                    {
                        stationCount = i;
                        break;
                    }
                }

                int stationsPlaced = 0;
                for ( size_t i = 0; i < stationCount; ++i )
                {
                    position_t pos;
                    float objectDistance = 0.f;
                    for ( size_t tries = 0; tries < 10 && objectDistance < 2.f; ++tries )
                    { 
                        pos = randPosition( SECTOR_SIZE, 2.f );
                        objectDistance = 2.f;

                        // maintain distance from jumpgates
                        for ( auto jumpgate : sector.jumpgates.all() )
                        {
                            objectDistance = std::min(
                                objectDistance,
                                ( jumpgate->position - pos ).magnitude() );

                            if ( objectDistance < 2.f )
                            {
                                break;
                            }
                        }

                        // maintain distance from other stations
                        if ( stationsPlaced && objectDistance >= 2.f )
                        {
                            auto it = stations.rbegin();
                            for ( size_t j = 0; j < stationsPlaced; ++j )
                            {
                                auto& station = *it;
                                objectDistance = std::min(
                                    objectDistance,
                                    ( station.position - pos ).magnitude() );

                                if ( objectDistance < 2.f )
                                {
                                    break;
                                }
                                ++it;
                            }
                        }
                    }

                    if ( objectDistance >= 2.f )
                    {
                        stations.emplace(stations.end(), &sector, pos);
                        ++stationsPlaced;
                    }
                }
            }
        }
    }

    // Reference now that the stations vector size is set (no reallocations)
    for ( auto& station : stations )
    {
        station.sector->stations.insert(&station);
    }
    return stations;
}


ships_t initShips(
    size_t const& shipCount,
    sectors_t& sectors,
    bool useJumpgates,
    float const& wallBuffer )
{
    ships_t ships;
    std::map<Sector*, ship_ptrs_set_t> sectorShipRefs;

    ships.reserve( shipCount );
    for ( size_t i = 0; i < shipCount; ++i )
    {
        bool isPlayerShip = i == 0;
        auto& sector = sectors[ rand() % sectors.size() ][ rand() % sectors[ 0 ].size() ];
        auto type    = randShipType();
        auto hull    = shipHull( type );
        auto code    = randCode();
        auto name    = randName( type );
        auto pos     = randPosition( sector.size, wallBuffer );
        auto dest    = randDestination( sector, useJumpgates, ( isPlayerShip ? 0.f : MISC_DESTINATION_CHANCE ));
        auto dir     = dest ? ( dest->position - pos ).normalized() : randDirection();
        auto speed   = shipSpeed( type );
        auto weapons = shipWeapons( type );
        auto turrets = shipTurrets( type );
        auto it      = ships.emplace( ships.end(), type, hull, code, name, &sector, pos, dir, speed, dest );
        auto& ship   = *it;

        // weapons/turrets
        weapon_ptrs_t newWeapons;
        weapon_ptrs_t newTurrets;
        newWeapons.reserve( weapons.size() );
        newTurrets.reserve( turrets.size() );
        bool isSideFire = isShipSideFire( type );
        for ( size_t i = 0; i < ( isSideFire ? 2 : 1 ); ++i )
        {
            for ( WeaponType weapon : weapons )
            {
                WeaponPosition weaponPosition = isSideFire
                                              ? i ? WeaponPosition_Port
                                                  : WeaponPosition_Starboard
                                              : WeaponPosition_Bow;
                newWeapons.emplace( newWeapons.end(), weapon_ptr_t( new Weapon( weapon, false, weaponPosition, ship )));
            }
        }
        for ( WeaponType turret : turrets )
        {
            newTurrets.emplace( newTurrets.end(), weapon_ptr_t( new Weapon( turret, true, WeaponPosition_Bow, ship )));
        }
        ship.setWeapons( std::move( newWeapons ));
        ship.setTurrets( std::move( newTurrets ));

        // friend/foe
        if ( isPlayerShip )
        {
            ship.faction = ShipFaction_Player;
        }
        else
        {
            float rnd = randFloat();
            if ( rnd < PLAYER_FREQUENCY )
            {
                ship.faction = ShipFaction_Player;
            }
            else if ( rnd < PLAYER_FREQUENCY + FRIEND_FREQUENCY )
            {
                ship.faction = ShipFaction_Friend;
            }
            else if ( rnd < PLAYER_FREQUENCY + FRIEND_FREQUENCY + ENEMY_FREQUENCY )
            {
                ship.faction = ShipFaction_Foe;
            }
        }

        // add ship to sector
        if ( ! sectorShipRefs.count( &sector ))
        {
            sectorShipRefs[ &sector ] = ship_ptrs_set_t();
        }
        sectorShipRefs[ &sector ].insert( &ship );
    }

    // Store sector ships
    for ( auto& sectorShipRef : sectorShipRefs )
    {
        sectorShipRef.first->setShips( std::move( sectorShipRef.second ));
    }

    return ships;
}


} // tinyspace
