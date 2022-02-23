// random.hpp (tiny space v3 - experiment: snapshot serialization) (C++11)
// AUTHOR: xixas | DATE: 2022.02.12 | LICENSE: WTFPL/PDM/CC0... your choice


#ifndef _TINYSPACE_RANDOM_HPP_
#define _TINYSPACE_RANDOM_HPP_


#include "types.hpp"


namespace tinyspace {


// Returns a value between min and max (inclusive)
static inline float randFloat( float min, float max )
{
    return min + ( static_cast<float>( rand() ) / static_cast<float>( RAND_MAX/( max-min )));
}
static inline float randFloat( float max=1.f )
{
    return randFloat( 0.f, max );
}


static inline position_t randPosition(position_t min, position_t max)
{
    return { randFloat( min.x, max.x ), randFloat( min.y, max.y ) };
}
static inline position_t randPosition( Vector2<position_t> minMax )
{
    return randPosition( minMax.x, minMax.y );
}
static inline position_t randPosition( dimensions_t dimensions, float wallBuffer=0.0f )
{
    return randPosition( { 0.0f+wallBuffer, 0.0f+wallBuffer },
                         { dimensions.x-wallBuffer, dimensions.y-wallBuffer } );
}


static inline direction_t randDirection() {
    direction_t direction;
    do
    {
        direction = { randFloat( -1.0f, 1.0f ), randFloat( -1.0f, 1.0f ) };
    } while ( ! direction.magnitude() );
    return direction.normalized();
}


static inline speed_t randSpeed(speed_t max)
{
    return randFloat( 0.f, max );
}


static inline ShipType randShipType()
{
    return static_cast<ShipType>( 1 + ( rand() % ( ShipType_END - 1 )));
}


static inline string randCode()
{
    char buf[8];
    size_t i;
    for ( i=0; i<3; ++i )
    {
        buf[i] = 'A' + static_cast<char>( rand() % ( 'Z'-'A'+1 ));
    }
    buf[3] = '-';
    for ( i=4; i<7; ++i )
    {
        buf[i] = '0' + static_cast<char>( rand() % ( '9'-'0'+1 ));
    }
    buf[7] = '\0';
    return buf;
}


static inline string randName( ShipType const& shipType )
{
    static size_t scoutCur     = 0;
    static size_t corvetteCur  = 0;
    static size_t frigateCur   = 0;
    static size_t transportCur = 0;
    ostringstream os;
    switch ( shipType )
    {
        case ShipType_Courier:
            os << "Z"
               << std::setfill( '0' )
               << std::setw( 3 )
               << ++transportCur;
            break;
        case ShipType_Transport:
            os << "T"
               << std::setfill( '0' )
               << std::setw( 3 )
               << ++transportCur;
            break;
        case ShipType_Scout:
            os << "S"
               << std::setfill( '0' )
               << std::setw( 3 )
               << ++scoutCur;
            break;
        case ShipType_Corvette:
            os << "C"
               << std::setfill( '0' )
               << std::setw( 3 )
               << ++corvetteCur;
            break;
        case ShipType_Frigate:
            os << "F"
               << std::setfill( '0' )
               << std::setw( 3 )
               << ++frigateCur;
            break;
        default:
            break;
    }
    return os.str();
}


static destination_ptr_t randDestination(
    Sector& sector,
    bool useJumpgates,
    float miscChance=0.f,
    vector<HasSectorAndPosition*>* excludes=nullptr )
{
    bool isMisc = false;
    if ( miscChance )
    {
        isMisc = randFloat() <= miscChance;
    }
    if ( ! isMisc )
    {
        vector<HasIDAndSectorAndPosition*> potentialDestinations;
        
        potentialDestinations.reserve(
            sector.stations.size() + sector.jumpgates.count() );

        potentialDestinations.insert(
            potentialDestinations.end(),
            sector.stations.begin(),
            sector.stations.end() );

        if ( useJumpgates )
        {
            auto jumpgates = sector.jumpgates.all();
            potentialDestinations.insert(
                potentialDestinations.end(),
                jumpgates.begin(),
                jumpgates.end() );
        }

        if ( excludes )
        {
            for ( auto exclude : *excludes )
            {
                auto it = std::find(
                    potentialDestinations.begin(),
                    potentialDestinations.end(),
                    exclude );

                if ( it != potentialDestinations.end() )
                {
                    potentialDestinations.erase(it);
                }
            }
        }

        if ( ! potentialDestinations.empty() )
        {
            size_t idx = rand() % potentialDestinations.size();
            auto destinationObject = potentialDestinations[idx];
            auto r = destination_ptr( new Destination( *destinationObject ));
            return r;
        }
    }
    return destination_ptr( new Destination( sector,
                                             randPosition( { 0,0 },
                                             sector.size )));
}


} //tinyspace


#endif // _TINYSPACE_RANDOM_HPP_
