// constants.hpp (tiny space v3 - experiment: snapshot serialization) (C++11)
// AUTHOR: xixas | DATE: 2022.02.12 | LICENSE: WTFPL/PDM/CC0... your choice


#ifndef _TINYSPACE_CONSTANTS_HPP_
#define _TINYSPACE_CONSTANTS_HPP_


#include "models.hpp"
#include "types.hpp"
#include "vector2.hpp"


namespace tinyspace {


// ---------------------------------------------------------------------------
// Static Constants
// ---------------------------------------------------------------------------


v2size_t     const SECTOR_BOUNDS           = {10, 10};
dimensions_t const SECTOR_SIZE             = {20, 20};
size_t       const SECTOR_MAP_LEFT_PADDING = 6;
size_t       const SHIP_COUNT              = 500;
float        const JUMPGATE_EDGE_BUFFER    = 0.25f;
size_t       const MAX_STATIONS_PER_SECTOR = 5;
float        const NO_STATIONS_FREQUENCY   = 0.25;
float        const PLAYER_FREQUENCY        = 0.01f;
float        const FRIEND_FREQUENCY        = 0.2f;
float        const ENEMY_FREQUENCY         = 0.1f;
float        const MISC_DESTINATION_CHANCE = 0.1f;
size_t       const TICK_TIME               = 300;  // milliseconds
float        const DOCK_TIME               = 3.f;  // seconds
float        const RESPAWN_TIME            = 10.f; // seconds

Vector2<position_t> const
    GATE_RANGE_NORTH {{ SECTOR_SIZE.x/3.f + 0.1f, 0.25f },               { 2*SECTOR_SIZE.x/3.f - 0.1f, SECTOR_SIZE.y/5.f }},
    GATE_RANGE_EAST  {{ 4*SECTOR_SIZE.x/5.f, SECTOR_SIZE.y/3.f + 0.1f }, { SECTOR_SIZE.x - 0.25f, 2*SECTOR_SIZE.y/3.f - 0.1f }},
    GATE_RANGE_SOUTH {{ SECTOR_SIZE.x/3.f + 0.1f, 4*SECTOR_SIZE.y/5.f }, { 2*SECTOR_SIZE.x/3.f - 0.1f, SECTOR_SIZE.y - 0.25f }},
    GATE_RANGE_WEST  {{ 0.25f, SECTOR_SIZE.y/3.f + 0.1f },               { SECTOR_SIZE.x/5.f, 2*SECTOR_SIZE.y/3.f - 0.1f }};

speed_t const DISTANCE_MULTIPLIER = 0.002f;

speed_t const
    COURIER_SPEED   = 600 * DISTANCE_MULTIPLIER,
    TRANSPORT_SPEED = 300 * DISTANCE_MULTIPLIER,
    SCOUT_SPEED     = 500 * DISTANCE_MULTIPLIER,
    CORVETTE_SPEED  = 400 * DISTANCE_MULTIPLIER,
    FRIGATE_SPEED   = 200 * DISTANCE_MULTIPLIER;

unsigned int const
    COURIER_HULL   = 300,
    TRANSPORT_HULL = 800,
    SCOUT_HULL     = 500,
    CORVETTE_HULL  = 1200,
    FRIGATE_HULL   = 1800;

distance_t const
    PULSE_RANGE    = 1000 * DISTANCE_MULTIPLIER,
    CANNON_RANGE   = 2000 * DISTANCE_MULTIPLIER,
    BEAM_RANGE     =  750 * DISTANCE_MULTIPLIER;

distance_t const MAX_TO_HIT_RANGE = 2000 * DISTANCE_MULTIPLIER; // match longest range weapon

// Cooldown in seconds
float const
    PULSE_COOLDOWN  = 1.f/3,
    CANNON_COOLDOWN = 1.f,
    BEAM_COOLDOWN   = 0.f;

unsigned int const
    PULSE_DAMAGE   = 20, // per shot
    CANNON_DAMAGE  = 60, // per shot
    BEAM_DAMAGE    = 20; // per second

float const
    PULSE_ACCURACY  = 0.8f,
    CANNON_ACCURACY = 0.5f,
    BEAM_ACCURACY   = 0.95f;

float const  // for targets
    COURIER_ACCURACY_MULTIPLIER   = 0.75f,
    TRANSPORT_ACCURACY_MULTIPLIER = 1.f,
    SCOUT_ACCURACY_MULTIPLIER     = 0.6f,
    CORVETTE_ACCURACY_MULTIPLIER  = 1.2f,
    FRIGATE_ACCURACY_MULTIPLIER   = 1.8f;

float const TURRET_RANGE_SCALE = 0.5;
float const TURRET_DAMAGE_SCALE = 0.7;


// ---------------------------------------------------------------------------
// Dynamic Constants
// ---------------------------------------------------------------------------


inline string shipClass( ShipType const& shipType )
{
    switch ( shipType )
    {
        case ShipType_Courier:   return "Courier";
        case ShipType_Transport: return "Transport";
        case ShipType_Scout:     return "Scout";
        case ShipType_Corvette:  return "Corvette";
        case ShipType_Frigate:   return "Frigate";
        default:                 return "";
    }
}


inline string paddedShipClass( ShipType const& shipType )
{
    switch ( shipType )
    {
        case ShipType_Courier:   return "Courier  ";
        case ShipType_Transport: return "Transport";
        case ShipType_Scout:     return "Scout    ";
        case ShipType_Corvette:  return "Corvette ";
        case ShipType_Frigate:   return "Frigate  ";
        default:                 return "         ";
    }
}


inline speed_t shipSpeed( ShipType shipType )
{
    switch ( shipType )
    {
        case ShipType_Courier:   return COURIER_SPEED;
        case ShipType_Transport: return TRANSPORT_SPEED;
        case ShipType_Scout:     return SCOUT_SPEED;
        case ShipType_Corvette:  return CORVETTE_SPEED;
        case ShipType_Frigate:   return FRIGATE_SPEED;
        default:                 return 0;
    }
}


inline unsigned int shipHull( ShipType shipType )
{
    switch ( shipType )
    {
        case ShipType_Courier:   return COURIER_HULL;
        case ShipType_Transport: return TRANSPORT_HULL;
        case ShipType_Scout:     return SCOUT_HULL;
        case ShipType_Corvette:  return CORVETTE_HULL;
        case ShipType_Frigate:   return FRIGATE_HULL;
        default:                 return 0;
    }
}


// Returns whether a ship has side-mounted weapons (as opposed to forward-mounted)
inline bool isShipSideFire( ShipType shipType )
{
    switch ( shipType )
    {
        case ShipType_Frigate: return true;
        default:               return false;
    }
}


inline weapontypes_t shipWeapons( ShipType shipType )
{
    switch ( shipType )
    {
//        case ShipType_Courier:   return {};
//        case ShipType_Transport: return {};
        case ShipType_Scout:     return { WeaponType_Pulse, WeaponType_Pulse };
        case ShipType_Corvette:  return { WeaponType_Pulse, WeaponType_Pulse, WeaponType_Cannon };
        case ShipType_Frigate:   return { WeaponType_Cannon, WeaponType_Cannon }; // Frigates fire from the side, so these weapons are doubled
        default:                 return {};
    }
}


inline weapontypes_t shipTurrets( ShipType shipType )
{
    switch ( shipType )
    {
        case ShipType_Courier:   return { WeaponType_Pulse };
        case ShipType_Transport: return { WeaponType_Pulse, WeaponType_Pulse };
//        case ShipType_Scout:     return {};
        case ShipType_Corvette:  return { WeaponType_Pulse, WeaponType_Pulse };
        case ShipType_Frigate:   return { WeaponType_Pulse, WeaponType_Pulse, WeaponType_Beam, WeaponType_Beam };
        default:                 return {};
    }
}


inline TargetType shipTypeToTargetType( ShipType shipType ) {
    switch ( shipType )
    {
        case ShipType_Courier:   return TargetType_Courier;
        case ShipType_Transport: return TargetType_Transport;
        case ShipType_Scout:     return TargetType_Scout;
        case ShipType_Corvette:  return TargetType_Corvette;
        case ShipType_Frigate:   return TargetType_Frigate;
        default:                 return TargetType_NONE;
    }
}


inline float weaponDamage( WeaponType weaponType, bool isTurret )
{
    float r;
    switch ( weaponType )
    {
        case WeaponType_Pulse:  r = PULSE_DAMAGE;  break;
        case WeaponType_Cannon: r = CANNON_DAMAGE; break;
        case WeaponType_Beam:   r = BEAM_DAMAGE;   break;
        default:                return 0.f;
    }
    if ( isTurret )
    {
        r *= TURRET_DAMAGE_SCALE;
    }
    return r;
}


inline float weaponCooldown( WeaponType weaponType )
{
    switch ( weaponType )
    {
        case WeaponType_Pulse:  return PULSE_COOLDOWN;
        case WeaponType_Cannon: return CANNON_COOLDOWN;
        case WeaponType_Beam:   return BEAM_COOLDOWN;
        default:                return 0.f;
    }
}

inline float isWeaponDamageOverTime( WeaponType weaponType )
{
    switch ( weaponType )
    {
        case WeaponType_Beam: return true;
        default:              return false;
    }
}


} //tinyspace


#endif // _TINYSPACE_CONSTANTS_HPP_
