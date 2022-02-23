// rand.hpp (tiny space v3 - experiment: snapshot serialization) (C++11)
// AUTHOR: xixas | DATE: 2022.02.21 | LICENSE: WTFPL/PDM/CC0... your choice


#ifndef _TINYSPACE_RAND_HPP_
#define _TINYSPACE_RAND_HPP_


#include "types.hpp"
#include "models.hpp"


namespace tinyspace {


// Returns a value between min and max (inclusive)
float randFloat( float min, float max );
float randFloat( float max=1.0f );

position_t randPosition( position_t min, position_t max );
position_t randPosition( Vector2<position_t> minMax );
position_t randPosition( dimensions_t dimensions, float wallBuffer=0.0f );

direction_t randDirection();

ShipType randShipType();

string randCode();

string randName( ShipType const& shipType );

destination_ptr_t randDestination(
    Sector& sector,
    bool useJumpgates,
    float miscChance=0.f,
    location_ptrs_t* excludes=nullptr);

float chanceToHit(
    WeaponType weaponType,
    bool isTurret,
    TargetType targetType,
    distance_t distance );

float chanceToHit(
    Weapon const& weapon,
    bool isTurret,
    WeaponPosition weaponPosition=WeaponPosition_Bow,
    location_ptr_t potentialTarget=nullptr );


} // tinyspace


#endif // _TINYSPACE_RAND_HPP_
