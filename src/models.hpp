// models.hpp (tiny space v3 - experiment: snapshot serialization) (C++11)
// AUTHOR: xixas | DATE: 2022.02.12 | LICENSE: WTFPL/PDM/CC0... your choice


#ifndef _TINYSPACE_MODELS_HPP_
#define _TINYSPACE_MODELS_HPP_


#include "types.hpp"


namespace tinyspace {


// Forward declarations
struct Sector;
struct Jumpgate;
struct Station;
struct Ship;


enum IdType : unsigned int
{
    IdType_NONE,
    IdType_Sector,
    IdType_Jumpgate,
    IdType_Station,
    IdType_Ship,
    IdType_Weapon,
    IdType_END
};


struct HasID
{
    id_s id;
    IdType_s idType;

    HasID( IdType const& idType );
    HasID( id_t const& id, IdType const& idType );
    ~HasID();

    void resetId();

private:
    static id_t curId;
};


struct HasCode
{
    string_s code;

    HasCode( string const& code="" );
    ~HasCode();
};


struct HasName
{
    string_s name;

    HasName( string const& name="" );
    ~HasName();
};


struct HasSize
{
    dimensions_s size;

    HasSize( dimensions_t const& size={ 0, 0 } );
    ~HasSize();
};


struct HasPosition
{
    position_s position;

    HasPosition( position_t const& position={ 0, 0 } );
    ~HasPosition();
};


struct HasDirection
{
    direction_s direction;

    HasDirection( direction_t const& direction={ 0, 0 } );
    ~HasDirection();
};


struct HasSpeed
{
    speed_s speed;

    HasSpeed( speed_t const& speed=0 );
    ~HasSpeed();
};


struct HasSector
{
    sector_ptr_s sector;

    HasSector( Sector* const sector=nullptr );
    ~HasSector();
};


struct HasSectorAndPosition : public HasSector, public HasPosition
{
    HasSectorAndPosition( Sector* const sector, position_t const& position );
    virtual ~HasSectorAndPosition();
};


struct HasIDAndSectorAndPosition : public HasID, public HasSectorAndPosition
{
    HasIDAndSectorAndPosition( IdType const& idType, Sector* const sector, position_t const& position );
    HasIDAndSectorAndPosition( id_t const& id, IdType const& idType, Sector* const sector, position_t const& position );
    virtual ~HasIDAndSectorAndPosition();
};


struct HasDestination
{
    destination_ptr_s destination;

    HasDestination( destination_ptr_t destination=nullptr );
    ~HasDestination();
};


struct SectorNeighbors
{
    Sector* north;
    Sector* east;
    Sector* south;
    Sector* west;

    SectorNeighbors();
    ~SectorNeighbors();

    int count();
    sector_ptrs_t all() const;
};


struct SectorJumpgates {
    Jumpgate* north;
    Jumpgate* east;
    Jumpgate* south;
    Jumpgate* west;

    SectorJumpgates();
    ~SectorJumpgates();

    int count();
    jumpgate_ptrs_t all( bool includeNull=false ) const;
};


struct Sector : public HasID, public HasName, public HasSize
{
    pair<size_t, size_t>   rowcol; // row and column in the universe (sectors)
    SectorNeighbors        neighbors;
    SectorJumpgates        jumpgates;
    station_ptrs_set_t     stations;
    ship_ptrs_set_s const& ships = _ships;

    Sector( pair<size_t, size_t> rowcol, string const& name="", dimensions_t const& size={ 0, 0 } );
    Sector( id_t const& id, pair<size_t, size_t> rowcol, string const& name="", dimensions_t const& size={ 0, 0 } );
    ~Sector();

    void setShips( ship_ptrs_set_t&& ships );

private:
    ship_ptrs_set_s _ships;
};


struct Jumpgate : public HasIDAndSectorAndPosition
{
    Jumpgate* target;

    Jumpgate( Sector* const sector=nullptr, position_t const& position={ 0, 0 }, Jumpgate* const target = nullptr );
    Jumpgate( id_t const& id, Sector* const sector=nullptr, position_t const& position={ 0, 0 }, Jumpgate* const target = nullptr );
    ~Jumpgate();
};


struct Station : public HasIDAndSectorAndPosition
{
    Station( Sector* const sector=nullptr, position_t const& position={ 0, 0 } );
    Station( id_t const& id, Sector* const sector=nullptr, position_t const& position={ 0, 0 } );
    ~Station();
};


struct Destination : public HasSectorAndPosition
{
    HasIDAndSectorAndPosition* object;  // sector and position are usually ignored if object is set

    Destination( HasIDAndSectorAndPosition& object );
    Destination( Sector& sector, position_t const& position );
    ~Destination();

    Sector* currentSector() const;
    position_t currentPosition() const;
};


enum WeaponType : unsigned int
{
    WeaponType_NONE,
    WeaponType_Pulse,
    WeaponType_Cannon,
    WeaponType_Beam,
    WeaponType_END
};


enum WeaponPosition : int
{
    WeaponPosition_Bow=0,
    WeaponPosition_Port=-1,
    WeaponPosition_Starboard=1,
    WeaponPosition_END
};


struct Weapon : public HasID
{
    WeaponType type;
    bool isTurret;
    target_ptr_t parent;
    target_ptr_s target;
    // weaponPosition designates forward mount (0), left (-1), or right (1) -- doesn't apply to turrets
    // these values can be considered 90 degree directional multipliers
    WeaponPosition weaponPosition;
    float_s cooldown;
    
    Weapon( WeaponType type, bool isTurret, WeaponPosition weaponPosition, HasIDAndSectorAndPosition& parent );
    Weapon( id_t const& id, WeaponType type, bool isTurret, WeaponPosition weaponPosition, HasIDAndSectorAndPosition& parent, HasIDAndSectorAndPosition* const target, float cooldown );
    ~Weapon();
};


enum ShipType : unsigned int
{
    // ship type in order of priority of target importance, least to greatest,
    // all civilian ships first
    ShipType_NONE,
    ShipType_Courier,
    ShipType_Transport,
    ShipType_Scout,
    ShipType_Corvette,
    ShipType_Frigate,
    ShipType_END
};


enum ShipFaction : unsigned int
{
    ShipFaction_Neutral,
    ShipFaction_Player,
    ShipFaction_Friend,
    ShipFaction_Foe,
    ShipFaction_END
};


struct Ship : public HasIDAndSectorAndPosition,
              public HasCode, public HasName,
              public HasDirection, public HasSpeed,
              public HasDestination
{
    ShipType_s           type;
    ShipFaction_s        faction;
    unsigned_int_s       maxHull, currentHull;
    weapon_ptrs_s const& weapons = _weapons;
    weapon_ptrs_s const& turrets = _turrets;
    target_ptr_s         target;
    bool_s               docked;
    double_s             timeout; // used any time the ship needs a delay (docked, dead, etc)

    Ship( Ship&& o );
    Ship( ShipType type, const unsigned int hull,
        string const& code="", string const& name="",
        Sector* const sector=nullptr, position_t const& position={ 0, 0 },
        direction_t const& direction={ 0, 0 }, speed_t const& speed=0,
        destination_ptr_t destination=nullptr );
    Ship( id_t const& id, ShipType type, ShipFaction faction,
        const unsigned int maxHull, const unsigned int currentHull,
        string const& code, string const& name,
        Sector* const sector, position_t const& position,
        direction_t const& direction, speed_t const& speed,
        destination_ptr_t destination,
        HasIDAndSectorAndPosition* target, bool docked, float timeout );
    ~Ship();

    void setWeapons( weapon_ptrs_t&& weapons );
    void setTurrets( weapon_ptrs_t&& turrets );

    Ship& operator =( Ship&& o );

    weapon_ptrs_t weaponsAndTurrets();

private:
    weapon_ptrs_s _weapons;
    weapon_ptrs_s _turrets;
};
bool operator <(const Ship& lhs, const Ship& rhs);
bool operator ==(const Ship& lhs, const Ship& rhs);


enum TargetType
{
    TargetType_NONE,
    // ships
    TargetType_Courier,
    TargetType_Transport,
    TargetType_Scout,
    TargetType_Corvette,
    TargetType_Frigate,
    TargetType_END
};


} //tinyspace
namespace std {
    template<> struct hash<tinyspace::Ship> {
        tinyspace::id_t operator ()( tinyspace::Ship const& o ) const noexcept {
            return o.id;
        }
    };
} // std


#endif // _TINYSPACE_MODELS_HPP_
