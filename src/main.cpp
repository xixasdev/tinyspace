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
#include "saveable.hpp"
#include "types.hpp"
#include "ui.hpp"
#include "vector2.hpp"
#include "xmlserializer.hpp"


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
// SAVE STATE SYNC ATOMICS
// ---------------------------------------------------------------------------


atomic<bool> willSave(false);
atomic<bool> saveComplete(false);
atomic<bool> endGame(false);


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

    auto performSave = [ & ]( ostream& os )
    {
        duration<double> d_save;
        time_point<steady_clock> t;
        
        t = steady_clock::now();
        XmlSerializer xml;
        os << xml.savegame( sectors, jumpgates, stations, ships ) << endl;
//        update_aftersave(); // commented to allow main thread control for this example
        d_save = steady_clock::now() - t;
        os << endl << "save time: " << ( d_save.count() * 1000 ) << "ms" << endl;
    };

    auto saveThreadFn = [ & ]( ostream& os, int delay=0 )
    {
//        isSaving = true; // commented to allow main thread control for this example
        willSave = false;

        // artificially make the save state take longer
        if ( delay ) std::this_thread::sleep_for( milliseconds( delay ));

        os << "SNAP" << endl << endl;
        performSave( os );

//        isSaving = false; // commented to allow main thread control for this example
        saveComplete = true;
    };

    auto mainThreadFn = [ & ]()
    {
        duration<double>         d1, d2, delta;
        time_point<steady_clock> t, thisTick, nextTick = steady_clock::now(), lastTick = nextTick;

        while ( ! endGame )
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

            // ---------------------------------------------------------------
            // TRIGGER SERIALIZATION AND EXIT WHEN COMPLETE
            // ---------------------------------------------------------------

            static size_t count = 0;
            count++;
            // save on background thread around 10 and 20 seconds (at ~3 frames per second)
            if ( count == 30 || count == 60 )
            {

                // write out live state for comparison
                livefile.open( count == 30 ? "tinyspace_01-live.txt" : "tinyspace_03-live.txt" );
                livefile << "LIVE" << endl << endl;
                performSave( livefile );
                livefile.close();

                willSave = true;

                // under main thread control for this example.
                // first snapshot should match live (as printed above).
                // isSaving stays true between the two save runs.
                // their outputs should be exactly the same!
                isSaving = true; 

                snapfile.open( count == 30 ? "tinyspace_02-snap.txt" : "tinyspace_04-snap.txt" );
                saveThread.store( new thread( saveThreadFn, std::ref( snapfile ), 5000 ));
            }

            // join save thread when complete
            if ( saveComplete )
            {
                if ( count > 60 )
                { // hold save state open for about 10 seconds between background saves
                    isSaving = false; // under main thread control for this example
                    update_aftersave(); // under main thread control for this example
                }

                saveComplete = false;

                if ( saveThread.load() )
                {
                    saveThread.load()->join();
                    saveThread = nullptr;
                    snapfile.close();
                }
            }

            // exit around 30 seconds (at ~3 frames per second)
            if ( count >= 90 && !willSave && !isSaving && !saveComplete )
            {
                endGame = true;
            }
        }
    };

    thread mainThread = thread( mainThreadFn );
    mainThread.join();

    return 0;
}
