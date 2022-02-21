// models.cpp (tiny space v3 - experiment: snapshot serialization) (C++11)
// AUTHOR: xixas | DATE: 2022.02.12 | LICENSE: WTFPL/PDM/CC0... your choice


#include "models.hpp"


namespace tinyspace {


// ---------------------------------------------------------------------------
// HasID
// ---------------------------------------------------------------------------


id_t HasID::curId = 0;


HasID::HasID( IdType const& idType )
    : id( ++curId )
    , idType( idType )
{}


HasID::HasID( id_t const& id, IdType const& idType )
    : id( id )
    , idType( idType )
{}


HasID::~HasID()
{}


void HasID::resetId()
{
    id = ++curId;
}


// ---------------------------------------------------------------------------
// HasCode
// ---------------------------------------------------------------------------


HasCode::HasCode( string const& code )
    : code( code )
{}


HasCode::~HasCode()
{}


// ---------------------------------------------------------------------------
// HasName
// ---------------------------------------------------------------------------


HasName::HasName( string const& name )
    : name( name )
{}


HasName::~HasName()
{}


// ---------------------------------------------------------------------------
// HasSize
// ---------------------------------------------------------------------------


HasSize::HasSize( dimensions_t const& size )
    : size( size )
{}


HasSize::~HasSize()
{}


// ---------------------------------------------------------------------------
// HasPosition
// ---------------------------------------------------------------------------


HasPosition::HasPosition( position_t const& position)
    : position( position )
{}


HasPosition::~HasPosition()
{}


// ---------------------------------------------------------------------------
// HasDirection
// ---------------------------------------------------------------------------


HasDirection::HasDirection( direction_t const& direction)
    : direction( direction )
{}


HasDirection::~HasDirection()
{}


// ---------------------------------------------------------------------------
// HasSpeed
// ---------------------------------------------------------------------------


HasSpeed::HasSpeed( speed_t const& speed )
    : speed( speed )
{}


HasSpeed::~HasSpeed()
{}


// ---------------------------------------------------------------------------
// HasSector
// ---------------------------------------------------------------------------


HasSector::HasSector( Sector* const sector )
    : sector( sector )
{}


HasSector::~HasSector()
{}


// ---------------------------------------------------------------------------
// HasSectorAndPosition
// ---------------------------------------------------------------------------


HasSectorAndPosition::HasSectorAndPosition( Sector* const sector, position_t const& position )
    : HasSector( sector ), HasPosition( position )
{}

HasSectorAndPosition::~HasSectorAndPosition()
{}


// ---------------------------------------------------------------------------
// HasIDAndSectorAndPosition
// ---------------------------------------------------------------------------


HasIDAndSectorAndPosition::HasIDAndSectorAndPosition(
    IdType const& idType,
    Sector* const sector,
    position_t const& position )
    :
    HasID( idType ), HasSectorAndPosition( sector, position )
{}


HasIDAndSectorAndPosition::HasIDAndSectorAndPosition(
    id_t const& id,
    IdType const& idType,
    Sector* const sector,
    position_t const& position )
    :
    HasID( id, idType ), HasSectorAndPosition( sector, position )
{}


HasIDAndSectorAndPosition::~HasIDAndSectorAndPosition()
{}


// ---------------------------------------------------------------------------
// HasDestination
// ---------------------------------------------------------------------------


HasDestination::HasDestination( destination_ptr_t destination )
    : destination( destination )
{}


HasDestination::~HasDestination()
{}


// ---------------------------------------------------------------------------
// SectorNeighbors
// ---------------------------------------------------------------------------


SectorNeighbors::SectorNeighbors()
    : north( nullptr ), east( nullptr ), south( nullptr ), west( nullptr )
{}


SectorNeighbors::~SectorNeighbors()
{}


int SectorNeighbors::count()
{
    return ( north ? 1 : 0 ) +
           ( east  ? 1 : 0 ) +
           ( south ? 1 : 0 ) +
           ( west  ? 1 : 0 );
}


sector_ptrs_t SectorNeighbors::all() const
{
    sector_ptrs_t sectors;
    if ( north ) sectors.push_back(north);
    if ( east )  sectors.push_back(east);
    if ( south ) sectors.push_back(south);
    if ( west )  sectors.push_back(west);
    return sectors;
}


// ---------------------------------------------------------------------------
// SectorJumpgates
// ---------------------------------------------------------------------------


SectorJumpgates::SectorJumpgates()
    : north( nullptr ), east( nullptr ), south( nullptr ), west( nullptr )
{}


SectorJumpgates::~SectorJumpgates()
{}


int SectorJumpgates::count()
{
    return ( north ? 1 : 0 ) +
           ( east  ? 1 : 0 ) +
           ( south ? 1 : 0 ) +
           ( west  ? 1 : 0 );
}


jumpgate_ptrs_t SectorJumpgates::all( bool includeNull ) const
{
    jumpgate_ptrs_t jumpgates;
    if ( north || includeNull ) jumpgates.push_back( north );
    if ( east  || includeNull ) jumpgates.push_back( east );
    if ( south || includeNull ) jumpgates.push_back( south );
    if ( west  || includeNull ) jumpgates.push_back( west );
    return jumpgates;
};


// ---------------------------------------------------------------------------
// Sector
// ---------------------------------------------------------------------------


Sector::Sector(
    pair<size_t, size_t> rowcol,
    string const& name,
    dimensions_t const& size )
    :
    HasID( IdType_Sector ),
    HasName( name ),
    HasSize( size ),
    rowcol( rowcol ),
    neighbors(),
    _ships()
{}


Sector::Sector(
    id_t const& id,
    pair<size_t, size_t> rowcol,
    string const& name,
    dimensions_t const& size )
    :
    HasID( id, IdType_Sector ),
    HasName( name ),
    HasSize( size ),
    rowcol( rowcol ),
    neighbors(),
    _ships()
{}


Sector::~Sector()
{}


void Sector::setShips( ship_ptrs_set_t&& ships )
{
    _ships = ships;
}


// ---------------------------------------------------------------------------
// Jumpgate
// ---------------------------------------------------------------------------


Jumpgate::Jumpgate(
    Sector* const sector,
    position_t const& position,
    Jumpgate* const target )
    :
    HasIDAndSectorAndPosition( IdType_Jumpgate, sector, position ),
    target(target)
{}


Jumpgate::Jumpgate(
    id_t const& id,
    Sector* const sector,
    position_t const& position,
    Jumpgate* const target )
    :
    HasIDAndSectorAndPosition( id, IdType_Jumpgate, sector, position ),
    target( target )
{}


Jumpgate::~Jumpgate()
{}


// ---------------------------------------------------------------------------
// Station
// ---------------------------------------------------------------------------


Station::Station( Sector* const sector, position_t const& position )
    : HasIDAndSectorAndPosition( IdType_Station, sector, position )
{};


Station::Station( id_t const& id, Sector* const sector, position_t const& position )
    : HasIDAndSectorAndPosition(id, IdType_Station, sector, position)
{};


Station::~Station()
{}


// ---------------------------------------------------------------------------
// Destination
// ---------------------------------------------------------------------------


Destination::Destination( HasIDAndSectorAndPosition& object )
    : HasSectorAndPosition(object.sector, object.position), object(&object)
{}


Destination::Destination( Sector& sector, position_t const& position )
    : HasSectorAndPosition(&sector, position), object(nullptr)
{}


Destination::~Destination()
{}


Sector* Destination::currentSector() const
{
    return object ? object->sector : sector;
}


position_t Destination::currentPosition() const
{
    return object ? object->position : position;
}


// ---------------------------------------------------------------------------
// Weapon
// ---------------------------------------------------------------------------


Weapon::Weapon(
    WeaponType type,
    bool isTurret,
    WeaponPosition weaponPosition,
    HasIDAndSectorAndPosition& parent )
    :
    HasID( IdType_Weapon ),
    type( type ),
    isTurret( isTurret ),
    weaponPosition( weaponPosition ),
    parent( &parent ),
    target( nullptr ), cooldown( 0 )
{}


Weapon::Weapon(
    id_t const& id,
    WeaponType type,
    bool isTurret,
    WeaponPosition weaponPosition,
    HasIDAndSectorAndPosition& parent,
    HasIDAndSectorAndPosition* const target, float cooldown )
    :
    HasID( id, IdType_Weapon ),
    type( type ),
    isTurret( isTurret ),
    weaponPosition( weaponPosition ),
    parent( &parent ),
    target( target ),
    cooldown( cooldown )
{}


Weapon::~Weapon()
{}


// ---------------------------------------------------------------------------
// Ship
// ---------------------------------------------------------------------------


Ship::Ship( Ship&& o )
    :
    HasIDAndSectorAndPosition( o.id, o.idType, o.sector, o.position ),
    HasName( o.name ),
    HasCode( o.code ),
    HasDirection( o.direction ),
    HasSpeed( o.speed ),
    HasDestination( o.destination ),
    type( o.type ),
    maxHull( o.maxHull ),
    currentHull( o.currentHull ),
    faction( o.faction ),
    _weapons( std::move( o._weapons )),
    _turrets( std::move( o._turrets )),
    target( o.target ),
    docked( o.docked ),
    timeout( o.timeout )
{}


Ship::Ship( 
    ShipType type, const unsigned int hull,
    string const& code, string const& name,
    Sector* const sector, position_t const& position,
    direction_t const& direction, speed_t const& speed,
    destination_ptr_t destination )
    :
    HasIDAndSectorAndPosition( IdType_Ship, sector, position ),
    HasName( name ),
    HasCode( code ),
    HasDirection( direction ),
    HasSpeed( speed ),
    HasDestination( destination ),
    type( type ),
    maxHull( hull ),
    currentHull( hull ),
    faction( ShipFaction_Neutral ),
    _weapons(),
    _turrets(),
    target( nullptr ),
    docked( false ),
    timeout( 0.f )
{}


Ship::Ship(
    id_t const& id, ShipType type, ShipFaction faction,
    const unsigned int maxHull, const unsigned int currentHull,
    string const& code, string const& name,
    Sector* const sector, position_t const& position,
    direction_t const& direction, speed_t const& speed,
    destination_ptr_t destination,
    HasIDAndSectorAndPosition* target, bool docked, float timeout )
    :
    HasIDAndSectorAndPosition( id, IdType_Ship, sector, position ),
    HasName( name ),
    HasCode( code ),
    HasDirection( direction ),
    HasSpeed( speed ),
    HasDestination( destination ),
    type( type ),
    maxHull( maxHull ),
    currentHull( currentHull ),
    faction( faction ),
    _weapons(),
    _turrets(),
    target( target ),
    docked( docked ),
    timeout( timeout )
{}


Ship::~Ship()
{}


void Ship::setWeapons( weapon_ptrs_t&& weapons )
{
    for ( auto& weapon : weapons )
    {
        weapon->isTurret = false;
    }
    _weapons = weapons;
}


void Ship::setTurrets( weapon_ptrs_t&& turrets )
{
    for ( auto& turret : turrets )
    {
        turret->isTurret = true;
    }
    _turrets = turrets;
}


Ship& Ship::operator =( Ship&& o )
{
    id          = o.id;
    idType      = o.idType;
    code        = o.code;
    name        = o.name;
    sector      = o.sector;
    position    = o.position;
    direction   = o.direction;
    speed       = o.speed;
    destination = o.destination;

    type        = o.type;
    maxHull     = o.maxHull;
    currentHull = o.currentHull;
    faction     = o.faction;

    weapon_ptrs_t weapons;
    weapon_ptrs_t turrets;
    weapons.reserve( o._weapons->size() );
    turrets.reserve( o._turrets->size() );
    for ( auto& weapon : o._weapons() ) weapons.push_back( weapon );
    for ( auto& turret : o._turrets() ) turrets.push_back( turret );
    setWeapons( std::move( weapons ));
    setTurrets( std::move( turrets ));

    target  = o.target;
    docked  = o.docked;
    timeout = o.timeout;

    return *this;
}


weapon_ptrs_t Ship::weaponsAndTurrets()
{
    weapon_ptrs_t r;
    r.reserve( weapons->size() + turrets->size() );
    for ( auto& weapon : _weapons() ) r.push_back( weapon );
    for ( auto& turret : _turrets() ) r.push_back( turret );
    return r;
}


bool operator <( const Ship& lhs, const Ship& rhs )
{
    return lhs.id < rhs.id;
}


bool operator ==( const Ship& lhs, const Ship& rhs )
{
    return lhs.id == rhs.id;
}


} // tinyspace
