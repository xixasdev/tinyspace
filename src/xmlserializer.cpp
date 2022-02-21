// xmlserializer.cpp
// tiny space v3 - experiment: snapshot serialization (C++11)
// AUTHOR: xixas | DATE: 2022.02.12 | LICENSE: WTFPL/PDM/CC0... your choice


#include "xmlserializer.hpp"

#include <sstream>
#include "constants.hpp"
#include "saveable.hpp"


namespace tinyspace {
using std::endl;
using std::ostringstream;


// Constants
const string XML_INDENT = "  ";


XmlSerializer::XmlSerializer()
{}


XmlSerializer::~XmlSerializer()
{}


string XmlSerializer::open( string      const& tagname,
                            xml_attrs_t const& attrs,
                            bool               selfClose )
{
    ostringstream os;
    os << '<'
       << tagname;
    for ( auto attr : attrs )
    {
        os << ' '
           << attr.first
           << "=\""
           << attr.second
           << '"';
    }
    os << ( selfClose ? "/>" : ">" );
    return os.str();
}


string XmlSerializer::close( string const& tagname )
{
    ostringstream os;
    os << "</"
       << tagname
       << '>';
    return os.str();
}


string XmlSerializer::id( id_t v )
{
    char buf[13];
    snprintf( buf, 13, "[0x%04zx]", v );
    return string( buf );
}


string XmlSerializer::id( HasID* v )
{
    return v ? id( v->id ) : "";
}


string XmlSerializer::boolean( bool v )
{
    return v ? "true" : "false";
}


template<typename T, typename U> // U must be an iterable of T pointers
static inline string ptrs(
    XmlSerializer& x,
    string( XmlSerializer::*fn )( T const&, string const& ),
    U const& o,
    string const& tagname,
    string const& indent )
{
    ostringstream os;
    if ( o.size() )
    {
        string subindent = indent + XML_INDENT;
        os << indent << x.open( tagname, {{ "count", x.number( o.size() ) }} ) << endl;
        for ( auto v : o )
        {
            os << (x.*fn)(*v, subindent) << endl;
        }
        os << indent << x.close( tagname );
    }
    return os.str();
}
static inline string jumpgates( XmlSerializer& x, jumpgate_ptrs_t o, string const& indent )
{
    return ptrs( x, &XmlSerializer::jumpgate, o, "jumpgates", indent );
}
static inline string stations( XmlSerializer& x, station_ptrs_set_t o, string const& indent )
{
    return ptrs( x, &XmlSerializer::station, o, "stations", indent );
}
static inline string ships( XmlSerializer& x, ship_ptrs_set_t o, string const& indent )
{
    return ptrs( x, &XmlSerializer::ship, o, "ships", indent );
}
static inline string weapons( XmlSerializer& x, weapon_ptrs_t o, string const& indent )
{
    return ptrs( x, &XmlSerializer::weapon, o, "weapons", indent );
}
static inline string turrets( XmlSerializer& x, weapon_ptrs_t o, string const& indent )
{
    return ptrs( x, &XmlSerializer::weapon, o, "turrets", indent );
}


string XmlSerializer::sector( Sector const& o, string const& indent )
{
    static string const tagname = "sector";
    string const subindent = indent + XML_INDENT;

    ostringstream os;
    xml_attrs_t attrs = {
        { "id",     id( o.id ) },
        { "rowcol", pair( o.rowcol ) },
        { "name",   o.name },
        { "size",   vector2( o.size ) },
    };
    os << indent << open( tagname, attrs )                 << endl
       << jumpgates( *this, o.jumpgates.all(), subindent ) << endl
       << stations( *this, o.stations, subindent )         << endl
       << ships( *this, o.ships, subindent )               << endl
       << indent << close( tagname );
    return os.str();
}


string XmlSerializer::jumpgate( Jumpgate const& o, string const& indent )
{
    static string const tagname = "jumpgate";
    ostringstream os;
    string nesw;
    if ( o.sector )
    {
        if      ( &o == o.sector->jumpgates.north ) nesw="north";
        else if ( &o == o.sector->jumpgates.east )  nesw="east";
        else if ( &o == o.sector->jumpgates.south ) nesw="south";
        else if ( &o == o.sector->jumpgates.west )  nesw="west";
    }
    xml_attrs_t attrs = {
        { "id",       id( o.id ) },
//        { "sector",   id( o.sector ) },
        { "nesw",     nesw },
        { "position", vector2( o.position ) },
        { "target",   id( o.target ) },
    };
    os << indent << open( tagname, attrs, true );
    return os.str();
}


string XmlSerializer::station( Station const& o, string const& indent )
{
    static string const tagname = "station";
    ostringstream os;
    xml_attrs_t attrs = {
        { "id",       id( o.id ) },
//        { "sector",   id( o.sector ) },
        { "position", vector2( o.position ) },
    };
    os << indent << open( tagname, attrs, true );
    return os.str();
}


string XmlSerializer::ship( Ship const& o, string const& indent )
{
    static string const tagname = "ship";
    ostringstream os;
    string subindent = indent + XML_INDENT;
    string faction;
    switch ( o.faction )
    {
        case ShipFaction_Player  : faction = "Player";  break;
        case ShipFaction_Friend  : faction = "Friend";  break;
        case ShipFaction_Foe     : faction = "Foe";     break;
        default                  : faction = "Neutral"; break;
    }
    xml_attrs_t attrs = {
        { "id",           id( o.id ) },
        { "type",         shipClass( o.type ) },
        { "faction",      faction },
        { "code",         o.code },
        { "name",         o.name },
        { "max-hull",     number( o.maxHull ) },
        { "current-hull", number( o.currentHull ) },
//        { "sector",       id( o.sector ) },
        { "position",     vector2( o.position ) },
        { "direction",    vector2( o.direction ) },
        { "speed",        number( o.speed ) },
    };
    if ( o.destination )
    {
        if ( o.destination->object)
        {
            attrs.emplace_back( "destination-object", id( o.destination->object ));
        }
        attrs.emplace_back( "destination-sector",  id( o.destination->sector ));
        attrs.emplace_back( "destination-position", vector2( o.destination->position ));
    }
    if ( o.target )        attrs.emplace_back( "target",  id( o.target ));
    if ( o.docked )        attrs.emplace_back( "docked",  boolean( o.docked ));
    if ( o.timeout > 0.0 ) attrs.emplace_back( "timeout", number( o.timeout ));
    os << indent << open( tagname, attrs ) << endl;
    string s;
    if (( s = weapons( *this, o.weapons, subindent )).size() ) os << s << endl;
    if (( s = turrets( *this, o.turrets, subindent )).size() ) os << s << endl;
    os << indent << close( tagname );
    return os.str();
}


string XmlSerializer::weapon( Weapon const& o, string const& indent )
{
    static string const tagname = "weapon";
    ostringstream os;
    string weaponType;
    string weaponPosition;
    switch ( o.type )
    {
        case WeaponType_Pulse  : weaponType = "Pulse";  break;
        case WeaponType_Cannon : weaponType = "Cannon"; break;
        case WeaponType_Beam   : weaponType = "Beam";   break;
    }
    switch ( o.weaponPosition )
    {
        case WeaponPosition_Bow       : weaponPosition = "Bow";       break;
        case WeaponPosition_Port      : weaponPosition = "Port";      break;
        case WeaponPosition_Starboard : weaponPosition = "Starboard"; break;
    }
    xml_attrs_t attrs = {
        { "id",           id( o.id ) },
        { "type",         weaponType },
//        { "is-turret",    boolean( o.isTurret ) },
//        { "parent",       id( o.parent ) },
    };
    if ( o.target )         attrs.emplace_back( "target",          id( o.target ));
    if ( o.weaponPosition ) attrs.emplace_back( "weapon-position", weaponPosition );
    if ( o.cooldown > 0.f ) attrs.emplace_back( "cooldown",        number( o.cooldown ));
    os << indent << open( tagname, attrs, true );
    return os.str();
}


string XmlSerializer::savegame( sectors_t   const& sectors,
                                jumpgates_t const& jumpgates,
                                stations_t  const& stations,
                                ships_t     const& ships,
                                string      const& indent )
{
    static string const tagname = "savegame";
    ostringstream os;
    string subindent = indent + XML_INDENT;
    os << indent << open( tagname ) << endl
       << subindent
       << open( "sectors", {{ "count", number( sectors.size() * sectors[0].size() ) }}, true )
       << endl
       << subindent
       << open( "jumpgates", {{ "count", number( jumpgates.size() ) }}, true )
       << endl
       << subindent
       << open( "stations", {{ "count", number( stations.size() ) }}, true )
       << endl
       << subindent
       << open( "ships", {{ "count", number( ships.size() ) }}, true )
       << endl;
    for ( auto& sectorRow : sectors )
    {
        for ( auto& v : sectorRow )
        {
            os << sector( v, subindent ) << endl;
        }
    }
    os << indent << close( tagname );
    return os.str();
}


} // tinyspace
