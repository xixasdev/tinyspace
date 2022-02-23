// xmlserializer.hpp
// tiny space v3 - experiment: snapshot serialization (C++11)
// AUTHOR: xixas | DATE: 2022.02.12 | LICENSE: WTFPL/PDM/CC0... your choice


#ifndef _TINYSPACE_XMLSERIALIZER_HPP_
#define _TINYSPACE_XMLSERIALIZER_HPP_


#include <ostream>
#include <string>
#include <utility>
#include <vector>
#include "models.hpp"
#include "types.hpp"


namespace tinyspace {


// Types
typedef vector<pair<string, string>> xml_attrs_t;


// Constants
extern const string XML_INDENT;


// XmlSerializer
class XmlSerializer
{
public:
    XmlSerializer();
    ~XmlSerializer();

    // General XML
    string open( string      const& tagname,
                 xml_attrs_t const& attrs={},
                 bool               selfClose=false );
    string close( string const& tagname );

    // Values
    string id( id_t v );
    string id( HasID* v );
    template<typename T>
    string number( T const& v );
    string boolean( bool v );
    template <typename T, typename U>
    string vector2( Vector2<T,U> const& v );
    template <typename T>
    string pair( std::pair<T,T> const& v );

    // Tags
    string sector( Sector const& o, string const& indent="" );
    string jumpgate( Jumpgate const& o, string const& indent="" );
    string station( Station const& o, string const& indent="" );
    string ship( Ship const& o, string const& indent="" );
    string weapon( Weapon const& o, string const& indent="" );
    string savegame( sectors_t   const& sectors,
                     jumpgates_t const& jumpgates,
                     stations_t  const& stations,
                     ships_t     const& ships,
                     string      const& indent="" );
}; // XmlSerializer


} // tinyspace


#include "xmlserializer.tpp"


#endif // _TINYSPACE_XMLSERIALIZER_HPP_
