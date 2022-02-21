// ui.cpp (tiny space v3 - experiment: snapshot serialization) (C++11)
// AUTHOR: xixas | DATE: 2022.02.21 | LICENSE: WTFPL/PDM/CC0... your choice


#include "ui.hpp"

#include <iomanip>
#include <map>
#include <sstream>
#include <vector>
#include "constants.hpp"


namespace tinyspace {


string beginColorString( unsigned int color, bool useColor, bool bold )
{
    if ( ! useColor ) return "";
    std::ostringstream os;
    os << "\033[" << (bold ? '1' : '0') << ";" << color << 'm';
    return os.str();
}


string endColorString( bool useColor, unsigned int defaultColor )
{
    if ( ! useColor ) return "";
    if ( defaultColor ) return beginColorString( defaultColor );
    return "\033[0m";
}


string colorString(
    unsigned int color,
    string const& str,
    bool useColor,
    bool bold,
    unsigned int defaultColor )
{
    if ( ! useColor ) return str;
    std::ostringstream os;
    os << beginColorString( color, true, bold ) << str << endColorString( true, defaultColor );
    return os.str();
}


string shipString( Ship const& ship, bool useColor, unsigned int color )
{
    useColor = useColor && color != 0;
    std::ostringstream os;
    if ( useColor )
    {
        os << beginColorString( color );
    }
    if ( ship.code->size() )
    {
        os << /*" code:"*/ " " << ship.code;
    }
    float hull = ship.currentHull / static_cast<float>( ship.maxHull );
    if ( useColor )
    {
        os << " "
           << colorString( hull > 0.0f ? COLOR_BRIGHT_CYAN : COLOR_BRIGHT_BLACK, "|", true, true, color )
           << colorString( hull > 0.2f ? COLOR_BRIGHT_CYAN : COLOR_BRIGHT_BLACK, "|", true, true, color )
           << colorString( hull > 0.4f ? COLOR_BRIGHT_CYAN : COLOR_BRIGHT_BLACK, "|", true, true, color )
           << colorString( hull > 0.6f ? COLOR_BRIGHT_CYAN : COLOR_BRIGHT_BLACK, "|", true, true, color )
           << colorString( hull > 0.8f ? COLOR_BRIGHT_CYAN : COLOR_BRIGHT_BLACK, "|", true, true, color );
    }
    else
    {
        os << " "
           << ( hull > 0.0f ? "|" : " " )
           << ( hull > 0.2f ? "|" : " " )
           << ( hull > 0.4f ? "|" : " " )
           << ( hull > 0.6f ? "|" : " " )
           << ( hull > 0.8f ? "|" : " " );
    }
    auto loc = ship.position - ( ship.sector ? ship.sector->size/2 : dimensions_t{ 0, 0 } );
    auto& dir = ship.direction;
    os << std::fixed << std::setprecision( 0 )
       << /*" loc:("*/ " ["
       << (  loc.x >= 0 ? " " : "" ) << loc.x << ","
       << ( -loc.y >= 0 ? " " : "" ) << -loc.y << "]";
    os << /*" dir:"*/ " "
       << ( dir.y <= -0.3 ? "N" : dir.y >= 0.3 ? "S" : " " )
       << ( dir.x <= -0.3 ? "W" : dir.x >= 0.3 ? "E" : " " );
    os << /*" class:"*/ " " << paddedShipClass( ship.type );
    if ( ship.target && ship.sector == ship.target->sector )
    {
        if ( auto target = dynamic_cast<Ship*>( ship.target() ))
        {
            os << " -> "
               << beginColorString( target->faction == ShipFaction_Player ? PLAYER_COLOR
                                  : target->faction == ShipFaction_Friend ? FRIEND_COLOR
                                  : target->faction == ShipFaction_Foe    ? ENEMY_COLOR
                                  :                                         NEUTRAL_COLOR
                                  ,
                                  useColor )
               << paddedShipClass( target->type );
            os << /*" code:"*/ " " << target->code
               << endColorString( useColor, color );
            float targetHull = target->currentHull / static_cast<float>( target->maxHull );
            if ( useColor )
            {
                os << " "
                   << colorString( targetHull > 0.0f ? COLOR_BRIGHT_CYAN : COLOR_BRIGHT_BLACK, "|", true, true, color )
                   << colorString( targetHull > 0.2f ? COLOR_BRIGHT_CYAN : COLOR_BRIGHT_BLACK, "|", true, true, color )
                   << colorString( targetHull > 0.4f ? COLOR_BRIGHT_CYAN : COLOR_BRIGHT_BLACK, "|", true, true, color )
                   << colorString( targetHull > 0.6f ? COLOR_BRIGHT_CYAN : COLOR_BRIGHT_BLACK, "|", true, true, color )
                   << colorString( targetHull > 0.8f ? COLOR_BRIGHT_CYAN : COLOR_BRIGHT_BLACK, "|", true, true, color );
            }
            else
            {
                os << " "
                   << ( targetHull > 0.0f ? "|" : " " )
                   << ( targetHull > 0.2f ? "|" : " " )
                   << ( targetHull > 0.4f ? "|" : " " )
                   << ( targetHull > 0.6f ? "|" : " " )
                   << ( targetHull > 0.8f ? "|" : " " );
            }
        }
    }
    if ( useColor )
    {
        os << endColorString();
    }
    return os.str();
}


vector<string> createSectorShipsList( Sector& sector, Ship* playerShip, bool const useColor )
{
    vector<string> shipsList;
    for ( auto ship : sector.ships() )
    {
        if ( ship->docked ) continue;
        std::ostringstream os;
        bool isPlayerShip    = ship == playerShip;
        bool isPlayerFaction = ship->faction == ShipFaction_Player;
        bool isFriend        = ship->faction == ShipFaction_Friend;
        bool isEnemy         = ship->faction == ShipFaction_Foe;
        unsigned int color = 0;
        if ( useColor )
        {
            color = ship->currentHull <= 0 ? COLOR_BRIGHT_BLACK
                  : isPlayerFaction        ? PLAYER_COLOR
                  : isFriend               ? FRIEND_COLOR
                  : isEnemy                ? ENEMY_COLOR
                  :                          NEUTRAL_COLOR
                  ;
        }
        string shipStr = shipString( *ship, useColor, color );
        os << ' '
           << ( isPlayerShip    ? '>'
              : isPlayerFaction ? '.'
              : isFriend        ? '+'
              : isEnemy         ? '-'
              :                   ' '
              )
           << shipStr;
        shipsList.push_back( os.str() );
    }
    return shipsList;
}


vector<string> createSectorMap( Sector& sector, Ship* playerShip, bool useColor )
{
    static string leftPadding( SECTOR_MAP_LEFT_PADDING, ' ' );
    vector<string> sectorMap;
    sectorMap.reserve( static_cast<size_t>( sector.size.x+3 ));
    { // header
        std::ostringstream os;
        os << leftPadding << '+' << string(( sector.size.x+1 ) * 3, '-' ) << '+';
        sectorMap.push_back( os.str() );
    }
    for ( size_t i = 0; i <= sector.size.y; ++i )
    {
        std::ostringstream os;
        os << leftPadding << '|' << string(( sector.size.x+1 ) * 3, ' ' ) << '|';
        sectorMap.push_back( os.str() );
    }
    { // footer
        std::ostringstream os;
        os << leftPadding << "+-[ "
           << colorString( PLAYER_COLOR, sector.name, useColor )
           << " ]" << string((( sector.size.x+1 ) * 3 ) - sector.name->size() - 5, '-' )
           << '+';
        sectorMap.push_back( os.str() );
    }

    // For character replacements
    // map<(row,col), replacement_str>
    std::map<pair<size_t, size_t>, string> replacements;

    auto addReplacementString = [ &replacements, &useColor ](
        pair<size_t, size_t> const& pos,
        unsigned int const color,
        string const& str,
        bool colorEach=false )
    {
        for ( size_t i = 0, j = str.size()-1; i <= j; ++i )
        {
            std::ostringstream os;
            if ( i==0 || colorEach )
            {
                os << beginColorString( color, useColor );
            }
            os << str[i];
            if ( i==j || colorEach )
            {
                os << endColorString( useColor );
            }
            replacements[ { pos.first, pos.second+i } ] = os.str();
        }
    };

    // Display saving message
    if ( isSaving )
    {
        string saveString( "[ SAVE IN PROGRESS... BUT GAME'S STILL RUNNING! ]" );
        size_t col = sectorMap[0].size()/2 - saveString.size()/2 + 3;
        addReplacementString( { 0, col }, 0, saveString );
    }

    // ships
    for ( auto ship : sector.ships() )
    {
        bool isPlayerShip    = ship == playerShip;
        bool isPlayerFaction = ship->faction == ShipFaction_Player;
        bool isFriend        = ship->faction == ShipFaction_Friend;
        bool isEnemy         = ship->faction == ShipFaction_Foe;
        auto& pos = ship->position;
        size_t col = 0, row = 0;
        for ( size_t i = 0; i < sector.size.x+1; ++i )
        {
            if ( pos.x >= -0.5f+i && pos.x < 0.5+i )
            {
                col = leftPadding.size() + 1 + i*3 + 1;
                break;
            }
        }
        for ( size_t i = 0; i < sector.size.y+1; ++i )
        {
            if ( pos.y >= -0.5f+i && pos.y < 0.5+i )
            {
                row = 1 + i;
                break;
            }
        }
        if ( ! replacements.count( { row,col } ) || isPlayerShip )
        {
            string shipStr = ".";
            if ( isPlayerShip )
            {
                auto& dir = ship->direction;
                auto dirMax = std::max( dir.y(), 0.0f );
                shipStr = "v";
                if ( dir.x > 0 && dir.x > dirMax )
                {
                    dirMax = dir.x;
                    shipStr = ">";
                }
                if ( dir.y < 0 && std::abs( dir.y ) > dirMax )
                {
                    dirMax = std::abs(dir.y);
                    shipStr = "^";
                }
                if ( dir.x < 0 && std::abs( dir.x ) > dirMax )
                {
                    shipStr = "<";
                }
            }
            unsigned int color = 0;
            if ( useColor )
            {
                color = ship->currentHull <= 0.f ? COLOR_BRIGHT_BLACK
                      : isPlayerFaction          ? PLAYER_COLOR
                      : isFriend                 ? FRIEND_COLOR
                      : isEnemy                  ? ENEMY_COLOR
                      :                            NEUTRAL_COLOR;
            }
            addReplacementString( { row, col }, color, shipStr );
        }
    }

    // stations
    for ( auto station : sector.stations )
    {
        auto const& pos = station->position;
        size_t col=0, row=0;
        for ( size_t i = 0; i < sector.size.x+1; ++i )
        {
            if ( pos.x >= -0.5f+i && pos.x < 0.5+i )
            {
                col = leftPadding.size() + 1 + i*3 + 1;
                break;
            }
        }
        for ( size_t i = 0; i < sector.size.y+1; ++i )
        {
            if ( pos.y >= -0.5f+i && pos.y < 0.5+i )
            {
                row = 1 + i;
                break;
            }
        }
        addReplacementString( { row, col }, STATION_COLOR, "()", true );
    }

    // jumpgates
    for ( auto jumpgate : sector.jumpgates.all() )
    {
        auto const& pos = jumpgate->position;
        size_t col=0, row=0;
        for ( size_t i = 0; i < sector.size.x+1; ++i )
        {
            if ( pos.x >= -0.5f+i && pos.x < 0.5+i )
            {
                col = leftPadding.size() + 1 + i*3 + 1;
                break;
            }
        }
        for ( size_t i = 0; i < sector.size.y+1; ++i )
        {
            if ( pos.y >= -0.5f+i && pos.y < 0.5+i )
            {
                row = 1 + i;
                break;
            }
        }
        addReplacementString( { row, col }, JUMPGATE_COLOR, "0" );
    }

    // kill screen
    if ( playerShip && playerShip->currentHull <= 0 )
    {
        char respawn[20];
        snprintf( respawn, 20, "|  respawn in %2d  |", static_cast<int>( playerShip->timeout ));
        vector<string> killscreen = {
            "+-----------------+",
            "| you were killed |",
            respawn,
            "+-----------------+"
        };
        size_t row = sectorMap.size()/2 - killscreen.size()/2 + 1;
        size_t col = sectorMap[0].size()/2 - killscreen[0].size()/2 + 3;
        for ( size_t i = 0, j = killscreen.size(); i < j; ++i )
        {
            addReplacementString( { row+i, col }, COLOR_BRIGHT_BLACK, killscreen[ i ] );
        }
    }

    // Character Replacements
    for ( auto it = replacements.crbegin(); it != replacements.crend(); ++it )
    {
        sectorMap[ it->first.first ].replace( it->first.second, 1, it->second );
    }

    return sectorMap;
}


vector<string> createGlobalMap( sectors_t const& sectors, Ship* playerShip, bool const useColor )
{
    vector<string> globalMap;
    globalMap.reserve( sectors.size()+1 );

    { // column headers
        std::ostringstream os;
        os << "    ";
        for ( size_t i = 0; i < sectors[ 0 ].size(); ++i )
        {
            os << "    " << static_cast<char>( 'A' + i ) << "  ";
        }
        globalMap.push_back( os.str() );
    }

    { // header divider
        std::ostringstream os;
        os << "   ." << string( sectors[ 0 ].size() * 7, '-' );
        globalMap.push_back( os.str() );
    }

    v2size_t playerSectorIndex;
    for ( size_t i = 0; i < sectors.size(); ++i )
    {
        std::ostringstream os;
        char rowBuf[3];
        sprintf( rowBuf, "%02zu", i+1 );
        os << rowBuf << " |";
        for ( size_t j = 0; j < sectors[ i ].size(); ++j )
        {
            Sector const& sector = sectors[ i ][ j ];

            bool isPlayerSector = playerShip && &sector == playerShip->sector;
            if ( isPlayerSector )
            {
                playerSectorIndex = { i, j };
            }

            size_t shipCount = sector.ships->size();
            bool hasPlayerProperty = false;
            for ( auto ship : sector.ships() )
            {
                if ( ship->currentHull <= 0.f )
                {
                    --shipCount;
                }
                if ( ship->faction == ShipFaction_Player && ship != playerShip )
                {
                    hasPlayerProperty = true;
                }
            }

            bool usePlayerColor = useColor && ( isPlayerSector || hasPlayerProperty );
            if ( usePlayerColor )
            {
                os << beginColorString( PLAYER_COLOR );
            }
            os << ( isPlayerSector ? "[" : " " );
            if ( shipCount )
            {
                char shipCountBuf[5];
                sprintf( shipCountBuf, "%4zu", shipCount );
                os << shipCountBuf;
            }
            else
            {
                os << "    ";
            }
            if ( usePlayerColor )
            {
                os << endColorString();
            }
            os << ( hasPlayerProperty ? "." : " " );
            if ( usePlayerColor )
            {
                os << beginColorString( PLAYER_COLOR );
            }
            os << ( isPlayerSector ? "]" : " " );
            if ( usePlayerColor )
            {
                os << endColorString();
            }
        }
        globalMap.push_back( os.str() );
    }

    if ( useColor && playerShip && playerShip->sector )
    {
        string& rowStr = globalMap[ 2 + playerSectorIndex.x ];
        string& colStr = globalMap[ 0 ];
        rowStr.replace(
            0,
            2,
            colorString( PLAYER_COLOR, rowStr.substr( 0, 2 )));
        colStr.replace(
            4 + 7*playerSectorIndex.y,
            7,
            colorString( PLAYER_COLOR, colStr.substr( 4 + 7*playerSectorIndex.y, 7 )));
    }

    return globalMap;
}


void updateDisplay(
    std::ostream& os,
    sectors_t& sectors,
    Ship& playerShip,
    bool const useColor )
{
    auto shipsList = createSectorShipsList( *playerShip.sector, &playerShip, useColor );
    auto sectorMap = createSectorMap( *playerShip.sector, &playerShip, useColor );
    auto globalMap = createGlobalMap( sectors, &playerShip, useColor );
    
    for ( size_t i = 0; i<50; ++i ) os << std::endl;
    os << std::endl;
    for ( auto& line : shipsList ) os << line << std::endl;
    os << std::endl;
    for ( auto& line : sectorMap ) os << line << std::endl;
    os << std::endl;
    for ( auto& line : globalMap ) os << line << std::endl;
    os << std::endl;
}


} // tinyspace
