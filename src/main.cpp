// main.cpp
// tiny space v3 - experiment: snapshot serialization (C++11)
// AUTHOR: xixas | DATE: 2022.02.12 | LICENSE: WTFPL/PDM/CC0... your choice
//
// DESCRIPTION: Gaming: Space sim in a tiny package.
//                      Work In Progress.
//
// CREATED:  2022.01.22 | AUTHOR: xixas | (v1) Original
// MODIFIED: 2022.01.23 | AUTHOR: xixas | (v2) Added colors and jumpgate travel
//           2022.01.30 | AUTHOR: xixas | (v3)
//                                      | Randomized jumpgate position
//                                      | Added factions and player-owner ships
//                                      | Added stations, docking, repair
//                                      | Added combat, killscreen, respawn
// EXPERIMENTS:
//           2022.02.01 | AUTHOR: xixas | Added basic XML serialization
//           2022.02.06 | AUTHOR: xixas | Added background serialization
//           2022.02.21 | AUTHOR: xixas | Split files for readability


#include <atomic>
#include <cstring>
#include <fstream>
#include <iostream>
#include <thread>
#include "actions.hpp"
#include "constants.hpp"
#include "init.hpp"
#include "types.hpp"
#include "ui.hpp"
#include "vector2.hpp"


using namespace tinyspace;
using std::atomic;
using std::chrono::duration;
using std::chrono::milliseconds;
using std::chrono::steady_clock;
using std::chrono::time_point;
using std::cout;
using std::endl;
using std::ofstream;
using std::ostream;
using std::thread;


// ---------------------------------------------------------------------------
// PROGRAM
// ---------------------------------------------------------------------------


int main( int argc, char** argv )
{
    std::srand( time( nullptr ));

    ofstream livefile, snapfile;

    bool useColor     = false;
    bool useJumpgates = true;

    for ( size_t i=0; i<argc; ++i )
    {
        if ( strcmp(argv[i], "--color") == 0 )        useColor     = true;
        if ( strcmp(argv[i], "--no-jumpgates" ) == 0) useJumpgates = false;
    }

    auto sectors   = initSectors( SECTOR_BOUNDS, SECTOR_SIZE );
    auto jumpgates = initJumpgates( sectors, useJumpgates );
    auto stations  = initStations( sectors );
    auto ships     = initShips( SHIP_COUNT, sectors, useJumpgates );

    auto& playerShip = ships[0];

    auto mainThreadFn = [ & ]()
    {
        duration<double>         d1, d2, delta;
        time_point<steady_clock> t, thisTick, nextTick = steady_clock::now(), lastTick = nextTick;

        while ( true )
        {
            std::this_thread::sleep_until( nextTick );

            thisTick   = steady_clock::now();
            delta      = thisTick - lastTick; // seconds
            nextTick   = thisTick + milliseconds(TICK_TIME);

            t = steady_clock::now();
            respawnShips( ships, &playerShip, stations, useJumpgates );
            moveShips( delta.count(), ships, &playerShip, useJumpgates );
            acquireTargets( sectors );
            fireWeapons( delta.count(), ships );
            d1 = steady_clock::now() - t;
            
            t = steady_clock::now();
            updateDisplay( cout, sectors, playerShip, useColor );
            d2 = steady_clock::now() - t;
            
            cout << "delta: "   << ( delta.count() * 1000 ) << "ms" << "  "
                 << "work: "    << ( d1.count() * 1000 )    << "ms" << "  "
                 << "display: " << ( d2.count() * 1000 )    << "ms" << endl;

            lastTick = thisTick;
        }
    };

    thread mainThread = thread( mainThreadFn );
    mainThread.join();

    return 0;
}
