// ui.hpp (tiny space v3 - experiment: snapshot serialization) (C++11)
// AUTHOR: xixas | DATE: 2022.02.21 | LICENSE: WTFPL/PDM/CC0... your choice


#ifndef _TINYSPACE_UI_HPP_
#define _TINYSPACE_UI_HPP_


#include <iostream>
#include "types.hpp"
#include "models.hpp"


namespace tinyspace {


const unsigned int
    COLOR_RED           = 31,
    COLOR_GREEN         = 32,
    COLOR_YELLOW        = 33,
    COLOR_BLUE          = 34,
    COLOR_CYAN          = 36,
    COLOR_BRIGHT_BLACK  = 90,
    COLOR_BRIGHT_RED    = 91,
    COLOR_BRIGHT_GREEN  = 92,
    COLOR_BRIGHT_YELLOW = 93,
    COLOR_BRIGHT_BLUE   = 94,
    COLOR_BRIGHT_CYAN   = 36,

    PLAYER_COLOR   = COLOR_BRIGHT_GREEN,
    NEUTRAL_COLOR  = COLOR_BLUE,
    FRIEND_COLOR   = COLOR_CYAN,
    ENEMY_COLOR    = COLOR_RED,
    JUMPGATE_COLOR = COLOR_BRIGHT_BLUE,
    STATION_COLOR  = COLOR_BRIGHT_BLACK;


string beginColorString( unsigned int color, bool useColor=true, bool bold=false );
string endColorString( bool useColor=true, unsigned int defaultColor=0 );
string colorString(
    unsigned int color,
    string const& str,
    bool useColor=true,
    bool bold=false,
    unsigned int defaultColor=0 );

string shipString( Ship const& ship, bool useColor=false, unsigned int color=0 );

vector<string> createSectorShipsList( Sector& sector, Ship* playerShip, bool const useColor );
vector<string> createSectorMap( Sector& sector, Ship* playerShip, bool useColor );
vector<string> createGlobalMap( sectors_t const& sectors, Ship* playerShip, bool const useColor );

void updateDisplay(
    std::ostream& os,
    sectors_t& sectors,
    Ship& playerShip,
    bool const useColor );


} // tinyspace


#endif // _TINYSPACE_UI_HPP_
