// init.hpp (tiny space v3 - experiment: snapshot serialization) (C++11)
// AUTHOR: xixas | DATE: 2022.02.21 | LICENSE: WTFPL/PDM/CC0... your choice


#ifndef _TINYSPACE_INIT_HPP_
#define _TINYSPACE_INIT_HPP_


#include "types.hpp"


namespace tinyspace {


sectors_t initSectors( v2size_t const& bounds, dimensions_t const& size );
jumpgates_t initJumpgates( sectors_t& sectors, bool useJumpgates );
stations_t initStations( sectors_t& sectors );
ships_t initShips(
    size_t const& shipCount,
    sectors_t& sectors,
    bool useJumpgates,
    float const& wallBuffer=0.1f );


} // tinyspace


#endif // _TINYSPACE_INIT_HPP_
