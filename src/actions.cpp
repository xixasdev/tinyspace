// actions.cpp (tiny space v3 - experiment: snapshot serialization) (C++11)
// AUTHOR: xixas | DATE: 2022.02.21 | LICENSE: WTFPL/PDM/CC0... your choice


#include "actions.hpp"

#include <algorithm>
#include <map>
#include <vector>
#include "constants.hpp"
#include "models.hpp"
#include "rand.hpp"


namespace tinyspace {


void moveShips(
    double delta, // seconds
    ships_t& ships,
    Ship* playerShip,
    bool useJumpgates )
{
    std::map<Sector*, ship_ptrs_set_t> sectorShipRefs;
    
    for ( auto& ship : ships )
    {
        if ( ship.timeout )
        {
            ship.timeout = std::max( 0.0, ship.timeout - delta );
        }
        if ( ship.docked && !ship.timeout )
        {
            ship.docked = false;
        }
        if ( ship.docked || ship.currentHull <= 0 || ship.timeout )
        {
            continue;
        }

        bool    isPlayerShip = &ship == playerShip;
        Sector* sector       = ship.sector;
        auto&   pos          = ship.position;
        auto&   dir          = ship.direction;
        auto&   speed        = ship.speed;
        auto&   dest         = ship.destination;

        if ( dest && dest->sector == sector )
        {
            auto destPos  = dest->currentPosition();
            auto newDir   = ( destPos - pos ).normalized();
            auto newPos   = pos + ( newDir * speed * delta );
            auto checkVec = destPos - newPos;
            if ( newDir.dot( checkVec ) > 0 )
            {
                // Continue toward destination
                pos = newPos;
                dir = newDir;
            }
            else
            {
                // Reached the destination
                pos = destPos;

                location_ptrs_t excludes;

                if ( auto jumpgate = dynamic_cast<Jumpgate*>( dest->object ))
                {
                    sector = jumpgate->target->sector;
                    pos = jumpgate->target->position;
                    excludes.push_back(jumpgate->target);
                }
                else if ( dynamic_cast<Station*>( dest->object ))
                {
                    excludes.insert( excludes.end(), sector->stations.begin(), sector->stations.end() );
                    // reached a station -- dock and repair ship
                    ship.currentHull = ship.maxHull;
                    ship.docked      = true;
                    ship.timeout     = DOCK_TIME; // dock timer
                }

                float miscChance = isPlayerShip && sector->jumpgates.count() > 1 ? 0.f : MISC_DESTINATION_CHANCE;
                if ( dest->object )
                {
                    dest = randDestination( *sector, useJumpgates, miscChance, &excludes );
                    dir = ( dest->position - pos ).normalized();
                }
                else
                {
                    dest = randDestination( *sector, useJumpgates, miscChance );
                }
            }
        }
        else
        {
            // No destination -- just keep flying
            pos = pos + ( dir * speed * delta );
        }

        if ( useJumpgates )
        {
            // Jumpgates required between sectors -- bounce off sector walls
            if ( pos.x < 0 || pos.x >= sector->size.x || pos.y < 0 || pos.y >= sector->size.y )
            {
                dest = randDestination( *sector, useJumpgates );
                dir = ( dest->position - pos ).normalized();
            }
        }
        else
        {
            // No jumpgates -- fly directly between sectors
            if ( pos.x < 0 )
            {
                if ( sector->neighbors.west )
                {
                    sector = sector->neighbors.west;
                    pos = {pos.x + sector->size.x, pos.y};
                }
                else
                {
                    dir = direction_t{ -dir.x, randFloat( -1, 1 ) }.normalized();
                }
            }
            else if ( pos.x >= sector->size.x )
            {
                if ( sector->neighbors.east )
                {
                    pos = { pos.x - sector->size.x, pos.y };
                    sector = sector->neighbors.east;
                }
                else
                {
                    dir = direction_t{ -dir.x, randFloat( -1, 1 ) }.normalized();
                }
            }
            if ( pos.y < 0 )
            {
                if ( sector->neighbors.north )
                {
                    sector = sector->neighbors.north;
                    pos = { pos.x, pos.y + sector->size.y };
                }
                else
                {
                    dir = direction_t{ randFloat( -1, 1 ), -dir.y }.normalized();
                }
            }
            else if ( pos.y >= sector->size.y )
            {
                if ( sector->neighbors.south )
                {
                    pos = { pos.x, pos.y - sector->size.y };
                    sector = sector->neighbors.south;
                }
                else
                {
                    dir = direction_t{ randFloat( -1, 1 ), -dir.y }.normalized();
                }
            }
        }

        // Handle sector changes
        if ( sector != ship.sector )
        {
            // remove from old sector
            if ( ! sectorShipRefs.count( ship.sector ))
            {
                sectorShipRefs[ ship.sector ] = ship_ptrs_set_t( ship.sector->ships );
            }
            sectorShipRefs[ ship.sector ].erase( &ship );
            // add to new sector
            if ( ! sectorShipRefs.count( sector ))
            {
                sectorShipRefs[ sector ] = ship_ptrs_set_t( sector->ships );
            }
            sectorShipRefs[ sector ].insert( &ship );
            // update ship
            ship.sector = sector;
        }

        // Maintain sector boundary
        if      ( pos.x < 0 )              pos.x = 0;
        else if ( pos.x > sector->size.x ) pos.x = sector->size.x;
        if      ( pos.y < 0 )              pos.y = 0;
        else if ( pos.y > sector->size.y ) pos.y = sector->size.y;
    }
    
    // Update sector ships
    for ( auto& sectorShipRef : sectorShipRefs )
    {
        sectorShipRef.first->setShips( std::move( sectorShipRef.second ));
    }
}


void acquireTargets( Sector& sector )
{
    std::map<Ship*, ship_ptrs_t> potentialTargets;
    ship_ptrs_t sectorShips;
    sectorShips.reserve( sector.ships.size() );

    for ( Ship* ship : sector.ships )
    {
        // Clear dead ships' targets
        if ( ship->currentHull <= 0.f )
        {
            if ( ship->target )
            {
                ship->target = nullptr;
            }
            for ( auto weapon : ship->weaponsAndTurrets() )
            {
                if ( weapon->target )
                {
                    weapon->target = nullptr;
                }
            }
            continue;
        }
        // Exclude neutral ships
        if ( ! ship->faction )
        {
            continue;
        }
        sectorShips.push_back( ship );
    }

    // Map targets potentially in range
    for ( Ship* ship : sectorShips )
    {
        if ( ! ship->weapons.empty() || !ship->turrets.empty() )
        {
            for ( Ship* otherShip : sectorShips )
            {
                // Exclude friendly ships, docked ships, dead ships, and ones definitely out of range
                if ( ship == otherShip
                ||   otherShip->docked
                ||   otherShip->currentHull <= 0.f
                ||   ship->faction == otherShip->faction
                ||   ( ship->faction == ShipFaction_Player && otherShip->faction == ShipFaction_Friend )
                ||   ( ship->faction == ShipFaction_Friend && otherShip->faction == ShipFaction_Player )
                ||   ( otherShip->position - ship->position ).magnitude() > MAX_TO_HIT_RANGE )
                {
                    continue;
                }

                if ( ! potentialTargets.count( ship ))
                {
                    potentialTargets.emplace( ship, std::move( vector<Ship*>{ otherShip } ));
                }
                else
                {
                    potentialTargets[ ship ].push_back( otherShip );
                }
            }
        }
        // Untarget all if no potential targets are in range
        if ( ! potentialTargets.count( ship ))
        {
            if ( ship->target )
            {
                ship->target = nullptr;
            }
            for ( auto& weapon : ship->weapons ) if ( weapon->target ) weapon->target = nullptr;
            for ( auto& turret : ship->turrets ) if ( turret->target ) turret->target = nullptr;
        }
    }

    // Assign targets
    for ( auto it = potentialTargets.begin(); it != potentialTargets.end(); ++it )
    {
        Ship* ship = it->first;
        ship_ptrs_t& targets = it->second;
        ship_ptrs_t possibleMainTargets;
        std::map<Weapon*, pair<Ship*, float>> weaponToHit;

        // Determine potential main targets and chance to hit per weapon
        for ( Ship* target : targets )
        {
            // potential main targets
            if ( possibleMainTargets.empty() || target->type > possibleMainTargets[0]->type )
            {
                if ( ! possibleMainTargets.empty() )
                {
                    possibleMainTargets.clear();
                }
                possibleMainTargets.push_back( target );
            }

            // main weapons
            for ( size_t i=0; i < ship->weapons.size(); ++i )
            {
                Weapon& weapon = *( ship->weapons[ i ] );
                WeaponPosition weaponPosition = isShipSideFire( ship->type )
                                              ? ( i < ship->weapons.size()/2 )
                                                  ? WeaponPosition_Port
                                                  : WeaponPosition_Starboard
                                              : WeaponPosition_Bow;
                float toHit = chanceToHit( weapon, false, weaponPosition, target );
                if ( toHit > 0.f )
                {
                    if ( ! weaponToHit.count( &weapon ))
                    {
                        weaponToHit.emplace( &weapon, std::move( pair<Ship*, float>( target, toHit )));
                    }
                    else
                    {
                        if ( toHit > weaponToHit[ &weapon ].second )
                        {
                            weaponToHit[ &weapon ] = { target, toHit };
                        }
                    }
                }
            }
            // turrets
            for ( size_t i=0; i < ship->turrets.size(); ++i )
            {
                Weapon& turret = *( ship->turrets[ i ] );
                float toHit = chanceToHit( turret, true, WeaponPosition_Bow, target );
                if ( toHit > 0.f )
                {
                    if ( ! weaponToHit.count( &turret ))
                    {
                        weaponToHit.emplace( &turret, std::move( pair<Ship*, float>( target, toHit )));
                    }
                    else
                    {
                        if ( toHit > weaponToHit[&turret].second )
                        {
                            weaponToHit[ &turret ] = { target, toHit };
                        }
                    }
                }
            }
        }

        // Assign primary target
        if ( ship->type >= ShipType_Scout ) // only military ships have primary targets
        {
            Ship* bestTarget = nullptr;
            distance_t distanceToBestTarget = 0;
            distance_t distanceToTarget;
            for ( auto target : possibleMainTargets )
            {
                if ( ! bestTarget )
                {
                    bestTarget = target;
                    distanceToBestTarget = ( target->position - ship->position ).magnitude();
                }
                else if ( target->currentHull != bestTarget->currentHull )
                {
                    if ( target->currentHull < bestTarget->currentHull )
                    {
                        bestTarget = target;
                        distanceToBestTarget = ( target->position - ship->position ).magnitude();
                    }
                }
                else
                {
                    distanceToTarget = ( target->position - ship->position ).magnitude();
                    if ( distanceToTarget < distanceToBestTarget )
                    {
                        bestTarget = target;
                        distanceToBestTarget = distanceToTarget;
                    }
                }
            }
            if ( ship->target != bestTarget )
            {
                ship->target = bestTarget;
            }
        }

        // Assign weapon targets
        {
            Ship* bestTarget;
            Weapon* p;
            for ( auto& weapon : ship->weapons )
            {
                p = &*weapon;
                bestTarget = nullptr;
                if ( weaponToHit.count( p ))
                {
                    bestTarget = weaponToHit[p].first;
                }
                if ( p->target != bestTarget )
                {
                    p->target = bestTarget;
                }
            }
            for ( auto& turret : ship->turrets )
            {
                p = &*turret;
                bestTarget = nullptr;
                if ( weaponToHit.count( p ))
                {
                    bestTarget = weaponToHit[ p ].first;
                }
                if ( p->target != bestTarget )
                {
                    p->target = bestTarget;
                }
            }
        }
    }
}


void acquireTargets( sectors_t& sectors )
{
    for ( auto& sectorRow : sectors )
    {
        for ( auto& sector : sectorRow )
        {
            acquireTargets( sector );
        }
    }
}


void fireWeapons(
    double delta, //seconds
    ships_t& ships )
{
    std::map<Weapon*, float> weaponCooldowns; // weapon and interative cooldown
    vector<pair<Weapon*, float>> shots;  // weapon and snapshot cooldown 

    // queue shots
    {
        float minNextCooldown = 0.f;
        bool minNextCooldownSet = false;
        do
        {
            weapon_ptrs_t weaponsAndTurrets;
            for ( auto& ship : ships )
            {
                weaponsAndTurrets = ship.weaponsAndTurrets();
                for ( auto weaponsIt = weaponsAndTurrets.begin(); weaponsIt != weaponsAndTurrets.end(); ++weaponsIt )
                {
                    auto weapon = &**weaponsIt;
                    if ( weapon->target )
                    {
                        if ( weapon->target->sector != weapon->parent->sector )
                        {
                            return;
                        }
                        if ( ! weaponCooldowns.count(weapon))
                        {
                            weaponCooldowns[weapon] = weapon->cooldown;
                        }
                        auto& currentCooldown = weaponCooldowns[weapon];
                        if ( currentCooldown <= delta )
                        {
                            shots.push_back( { weapon, currentCooldown } );
                            currentCooldown += weaponCooldown( weapon->type );
                            if ( ! minNextCooldownSet || currentCooldown < minNextCooldown )
                            {
                                minNextCooldown = currentCooldown;
                                minNextCooldownSet = true;
                            }
                        }
                    }
                }
                minNextCooldownSet = false;
            }
        }
        while ( minNextCooldown < delta && minNextCooldown > 0.f );
    }

    // sort shot order
    sort( shots.begin(),
          shots.end(),
          []( pair<Weapon*, float> a, pair<Weapon*, float> b ) -> bool { return a.second < b.second; } );

    // apply shots
    if ( shots.size() )
    {
        float currentCooldown = 0.f;
        float appliedDelta    = 0.f;
        bool canFire          = true;
        for ( size_t i=0; i<shots.size(); ++i )
        {
            do
            {
                auto& shot     = shots[ i ];
                auto  weapon   = shot.first;
                auto& cooldown = shot.second;

                if ( cooldown != currentCooldown )
                {
                    appliedDelta    = currentCooldown;
                    currentCooldown = shot.second;
                    continue;
                }
                if ( auto target = dynamic_cast<Ship*>( weapon->target ))
                {
                    canFire = !target->docked && target->currentHull >= 0;
                    if ( canFire )
                    {
                        if ( Ship* ship = dynamic_cast<Ship*>( weapon->parent ))
                        {
                            if ( ship->currentHull <= 0.f )
                            {
                                // ship is dead
                                if ( cooldown > appliedDelta )
                                {
                                    // live fire rounds are expended
                                    canFire = false;
                                }
                            }
                        }

                        if ( canFire)
                        {
                            float toHit = chanceToHit( *weapon, weapon->isTurret, weapon->weaponPosition );
                            if ( toHit <= 0.f )
                            {
                                canFire = false;
                            }

                            if ( canFire )
                            {
                                float tryFire = randFloat();
                                if ( tryFire <= toHit )
                                {
                                    auto damage = weaponDamage( weapon->type, weapon->isTurret );
                                    if ( isWeaponDamageOverTime( weapon->type ))
                                    {
                                        // adjust beam weapon damage
                                        damage *= cooldown - appliedDelta;
                                    }
                                    if ( auto target = dynamic_cast<Ship*>( weapon->target ))
                                    {
                                        target->currentHull = std::max( 0.f, target->currentHull - damage );
                                        if ( target->currentHull <= 0 )
                                        {
                                            // respawn timer
                                            target->timeout = RESPAWN_TIME;
                                        }
                                    }
                                }
                            }
                        }
                    }
                }
                ++i;
            }
            while ( currentCooldown >= appliedDelta && i < shots.size() );
        }
    }

    // Update live weapon cooldowns
    for ( auto& ship : ships )
    {
        for ( auto& weapon : ship.weapons ) if (weapon->cooldown > 0.f) weapon->cooldown -= delta;
        for ( auto& turret : ship.turrets ) if (turret->cooldown > 0.f) turret->cooldown -= delta;
    }
}


void respawnShips(
    ships_t& ships,
    Ship* playerShip,
    stations_t& stations,
    bool useJumpgates )
{
    if ( stations.empty() )
    {
        return; // return if there are no valid respawn points
    }

    std::map<Sector*, ship_ptrs_set_t> sectorShipRefs;
    
    for ( auto& ship : ships )
    {
        bool isPlayerShip = &ship == playerShip;
        if ( ship.currentHull <= 0 && ship.timeout <= 0.f )
        {
            // don't respawn ships that are dead in the player's sector
            if ( playerShip && ! isPlayerShip && ship.sector == playerShip->sector )
            {
                continue;
            }

            // remove dead ship from the sector
            if ( ! sectorShipRefs.count( ship.sector )) 
            {
                sectorShipRefs[ ship.sector ] = ship_ptrs_set_t( ship.sector->ships );
            }
            sectorShipRefs[ ship.sector ].erase( &ship );

            // select a random station for respawn
            Station& station = stations[ rand() % stations.size() ];

            // redefine the ship from scratch
            Sector& sector = *station.sector;
            auto type      = randShipType();
            auto hull      = shipHull(type);
            auto code      = randCode();
            auto name      = randName(type);
            auto pos       = station.position;
            auto speed     = shipSpeed(type);
            auto weapons   = shipWeapons(type);
            auto turrets   = shipTurrets(type);

            // determine a travel destination for after undocking
            vector<HasSectorAndPosition*> excludes{ &station };
            float miscChance = isPlayerShip && sector.jumpgates.count() > 1 ? 0.f : MISC_DESTINATION_CHANCE;
            auto dest = randDestination( sector, useJumpgates, miscChance, &excludes );
            auto dir = ( dest->position - pos ).normalized();

            // replace dead ship, docked at selected station
            ship = Ship( type, hull, code, name, &sector, pos, dir, speed, dest );
            ship.docked = true;
            ship.timeout = 0.f;

            // weapons/turrets
            weapon_ptrs_t newWeapons;
            weapon_ptrs_t newTurrets;
            newWeapons.reserve( weapons.size() );
            newTurrets.reserve( turrets.size() );
            bool isSideFire = isShipSideFire(type);
            for (size_t i = 0; i < ( isSideFire ? 2 : 1 ); ++i)
            {
                for ( WeaponType weapon : weapons )
                {
                    WeaponPosition weaponPosition = isSideFire
                                                  ? i ? WeaponPosition_Port
                                                      : WeaponPosition_Starboard
                                                  : WeaponPosition_Bow;
                    newWeapons.emplace( newWeapons.end(), weapon_ptr_t( new Weapon(weapon, false, weaponPosition, ship )));
                }
            }
            for ( WeaponType turret : turrets )
            {
                newTurrets.emplace( newTurrets.end(), weapon_ptr_t( new Weapon( turret, true, WeaponPosition_Bow, ship )));
            }
            ship.setWeapons( std::move( newWeapons ));
            ship.setTurrets( std::move( newTurrets ));

            // friend/foe
            if ( isPlayerShip )
            {
                ship.faction = ShipFaction_Player;
            }
            else
            {
                // Neutral ships aren't presently part of the combat system, so
                // there's no need to respawn them -- so choose a combat-capable
                // faction
                float rnd = randFloat( PLAYER_FREQUENCY + FRIEND_FREQUENCY + ENEMY_FREQUENCY );
                if ( rnd < PLAYER_FREQUENCY )
                {
                    ship.faction = ShipFaction_Player;
                }
                else if ( rnd < PLAYER_FREQUENCY + FRIEND_FREQUENCY )
                {
                    ship.faction = ShipFaction_Friend;
                }
                else if ( rnd < PLAYER_FREQUENCY + FRIEND_FREQUENCY + ENEMY_FREQUENCY )
                {
                    ship.faction = ShipFaction_Foe;
                }
            }

            // add to respawn sector
            if ( ! sectorShipRefs.count( &sector ))
            {
                sectorShipRefs[ &sector ] = ship_ptrs_set_t( sector.ships );
            }
            sectorShipRefs[ &sector ].insert( &ship );
        }
    }
    
    // Update sector ships
    for ( auto& sectorShipRef : sectorShipRefs )
    {
        sectorShipRef.first->setShips( std::move( sectorShipRef.second ));
    }
}


} // tinyspace
