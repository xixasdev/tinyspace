// rand.cpp (tiny space v3 - experiment: snapshot serialization) (C++11)
// AUTHOR: xixas | DATE: 2022.02.21 | LICENSE: WTFPL/PDM/CC0... your choice


#include "rand.hpp"

#include <algorithm>
#include <iomanip>
#include <sstream>
#include "constants.hpp"


namespace tinyspace {


// Returns a value between min and max (inclusive)
float randFloat( float min, float max )
{
    return min + ( static_cast<float>( rand() ) / static_cast<float>( RAND_MAX/( max-min )));
}


// Returns a value between zero and max (inclusive)
float randFloat( float max )
{
    return randFloat( 0.f, max );
}


position_t randPosition( position_t min, position_t max )
{
    return { randFloat( min.x, max.x ), randFloat( min.y, max.y ) };
}


position_t randPosition( Vector2<position_t> minMax )
{
    return randPosition( minMax.x, minMax.y );
}


position_t randPosition( dimensions_t dimensions, float wallBuffer )
{
    return randPosition( { 0.0f+wallBuffer,         0.0f+wallBuffer },
                         { dimensions.x-wallBuffer, dimensions.y-wallBuffer } );
}


direction_t randDirection() {
    direction_t direction;
    do
    {
        direction = { randFloat( -1.0f, 1.0f ), randFloat( -1.0f, 1.0f ) };
    }
    while ( ! direction.magnitude() );
    return direction.normalized();
}


ShipType randShipType()
{
    return static_cast<ShipType>( 1 + ( rand() % ( ShipType_END - 1 )));
}


string randCode()
{
    char buf[8];
    size_t i;
    for ( i = 0; i < 3; ++i )
    {
        buf[ i ] = 'A' + static_cast<char>( rand() % ( 'Z'-'A'+1 ));
    }
    buf[3] = '-';
    for ( i = 4; i < 7; ++i )
    {
        buf[ i ] = '0' + static_cast<char>( rand() % ( '9'-'0'+1 ));
    }
    buf[7] = '\0';
    return buf;
}


string randName( ShipType const& shipType )
{
    static size_t scoutCur     = 0;
    static size_t corvetteCur  = 0;
    static size_t frigateCur   = 0;
    static size_t transportCur = 0;
    std::ostringstream os;
    switch ( shipType )
    {
        case ShipType_Courier:   os << "Z" << std::setfill( '0' ) << std::setw( 3 ) << ++transportCur; break;
        case ShipType_Transport: os << "T" << std::setfill( '0' ) << std::setw( 3 ) << ++transportCur; break;
        case ShipType_Scout:     os << "S" << std::setfill( '0' ) << std::setw( 3 ) << ++scoutCur;     break;
        case ShipType_Corvette:  os << "C" << std::setfill( '0' ) << std::setw( 3 ) << ++corvetteCur;  break;
        case ShipType_Frigate:   os << "F" << std::setfill( '0' ) << std::setw( 3 ) << ++frigateCur;   break;
        default:                                                                                       break;
    }
    return os.str();
}


destination_ptr_t randDestination(
    Sector& sector,
    bool useJumpgates,
    float miscChance,
    location_ptrs_t* excludes )
{
    bool isMisc = false;

    if ( miscChance )
    {
        isMisc = randFloat() <= miscChance;
    }

    if ( ! isMisc)
    {
        vector<HasIDAndSectorAndPosition*> potentialDestinations;
        potentialDestinations.reserve( sector.stations.size() + sector.jumpgates.count() );

        potentialDestinations.insert( potentialDestinations.end(),
                                      sector.stations.begin(),
                                      sector.stations.end() );
        if ( useJumpgates )
        {
            auto jumpgates = sector.jumpgates.all();
            potentialDestinations.insert( potentialDestinations.end(),
                                          jumpgates.begin(),
                                          jumpgates.end());
        }

        if ( excludes )
        {
            for ( auto exclude : *excludes )
            {
                auto it = std::find( potentialDestinations.begin(),
                                     potentialDestinations.end(),
                                     exclude );
                if ( it != potentialDestinations.end() )
                {
                    potentialDestinations.erase( it );
                }
            }
        }

        if ( ! potentialDestinations.empty() )
        {
            auto destinationObject = potentialDestinations[ rand() % potentialDestinations.size() ];
            auto r = destination_ptr_t( new Destination( *destinationObject ));
            return r;
        }
    }
    return destination_ptr_t( new Destination( sector, randPosition( { 0,0 }, sector.size )));
}


float chanceToHit(
    WeaponType weaponType,
    bool isTurret,
    TargetType targetType,
    distance_t distance )
{
    distance_t range;
    switch ( weaponType )
    {
        case WeaponType_Pulse:  range = PULSE_RANGE;  break;
        case WeaponType_Cannon: range = CANNON_RANGE; break;
        case WeaponType_Beam:   range = BEAM_RANGE;   break;
        default:                range = 0;            break;
    }
    if ( isTurret )
    {
        range *= TURRET_RANGE_SCALE;
    }
    if ( range < distance )
    {
        return 0.f;
    }

    float chance;
    switch ( weaponType )
    {
        case WeaponType_Pulse:  chance = PULSE_ACCURACY;  break;
        case WeaponType_Cannon: chance = CANNON_ACCURACY; break;
        case WeaponType_Beam:   chance = BEAM_ACCURACY;   break;
        default:                chance = 0;               break;
    }
    switch ( targetType )
    {
        case TargetType_Courier:    chance *= COURIER_ACCURACY_MULTIPLIER;   break;
        case TargetType_Transport:  chance *= TRANSPORT_ACCURACY_MULTIPLIER; break;
        case TargetType_Scout:      chance *= SCOUT_ACCURACY_MULTIPLIER;     break;
        case TargetType_Corvette:   chance *= CORVETTE_ACCURACY_MULTIPLIER;  break;
        case TargetType_Frigate:    chance *= FRIGATE_ACCURACY_MULTIPLIER;   break;
        default:                                                             break;
    }
    return chance;
}


// weaponPosition designates forward mount (0), port (-1), or starboard (1); 
float chanceToHit(
    Weapon const& weapon,
    bool isTurret,
    WeaponPosition weaponPosition,
    location_ptr_t potentialTarget )
{
    location_ptr_t parent = weapon.parent;
    location_ptr_t target = potentialTarget ? potentialTarget : weapon.target;
    if ( ! target || ! parent || parent->sector != target->sector )
    {
        return 0.f;
    }
    direction_t targetVector = target->position - parent->position;
    if ( targetVector.magnitude() > MAX_TO_HIT_RANGE )
    {
        return 0.f;
    }
    if ( ! isTurret )
    {
        if ( Ship* ship = dynamic_cast<Ship*>( parent ))
        {
            direction_t const& dir = ship->direction;
            direction_t aim = weaponPosition == WeaponPosition_Port      ? dir.port()
                            : weaponPosition == WeaponPosition_Starboard ? dir.starboard()
                            :                                              dir;
            // +/-45 = 90 degree aim window
            if ( abs( aim.angleDeg( targetVector )) > 45 )
            {
                return 0.f;
            }
        }
        else
        {
            return 0.f;
        }
    }
    TargetType targetType = TargetType_NONE;
    if ( Ship* targetShip = dynamic_cast<Ship*>( target ))
    {
        targetType = shipTypeToTargetType( targetShip->type );
    }
    return chanceToHit( weapon.type, isTurret, targetType, targetVector.magnitude() );
}


} //tinyspace
