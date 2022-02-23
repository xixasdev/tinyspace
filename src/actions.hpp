// actions.hpp (tiny space v3 - experiment: snapshot serialization) (C++11)
// AUTHOR: xixas | DATE: 2022.02.21 | LICENSE: WTFPL/PDM/CC0... your choice


#ifndef _TINYSPACE_ACTIONS_HPP_
#define _TINYSPACE_ACTIONS_HPP_


#include "types.hpp"


namespace tinyspace {


void moveShips(
    double delta, // seconds
    ships_t& ships,
    Ship* playerShip,
    bool useJumpgates );

void acquireTargets( Sector& sector );
void acquireTargets( sectors_t& sectors );

void fireWeapons(
    double delta, //seconds
    ships_t& ships );

void respawnShips(
    ships_t& ships,
    Ship* playerShip,
    stations_t& stations,
    bool useJumpgates );


} // tinyspace


#endif // _TINYSPACE_ACTIONS_HPP_
