// types.hpp (tiny space v3 - experiment: snapshot serialization) (C++11)
// AUTHOR: xixas | DATE: 2022.02.12 | LICENSE: WTFPL/PDM/CC0... your choice


#ifndef _TINYSPACE_TYPES_HPP_
#define _TINYSPACE_TYPES_HPP_


#include <memory>
#include <set>
#include <vector>
#include "vector2.hpp"


namespace tinyspace {
using std::pair;
using std::set;
using std::shared_ptr;
using std::string;
using std::vector;

// Forward declarations
struct Destination;
struct HasIDAndSectorAndPosition;
struct HasSectorAndPosition;
struct Jumpgate;
struct Sector;
struct Ship;
struct Station;
struct Weapon;

enum IdType : unsigned int;
enum ShipFaction : unsigned int;
enum ShipType : unsigned int;
enum WeaponPosition : int;
enum WeaponType : unsigned int;


// Standard types
typedef size_t          id_t;
typedef Vector2<int>    v2int_t;
typedef Vector2<size_t> v2size_t;
typedef Vector2<float>  v2float_t;
typedef v2float_t       dimensions_t;
typedef v2float_t       position_t;
typedef v2float_t       direction_t;
typedef float           speed_t;
typedef float           distance_t;


// Class-specific types
typedef shared_ptr<Destination>    destination_ptr_t;
typedef shared_ptr<Weapon>         weapon_ptr_t;
typedef HasSectorAndPosition*      location_ptr_t;
typedef HasIDAndSectorAndPosition* target_ptr_t;

typedef vector<vector<Sector>>     sectors_t;
typedef vector<Jumpgate>           jumpgates_t;
typedef vector<Station>            stations_t;
typedef vector<Ship>               ships_t;
typedef vector<WeaponType>         weapontypes_t;

typedef vector<Sector*>            sector_ptrs_t;
typedef vector<Jumpgate*>          jumpgate_ptrs_t;
typedef vector<Station*>           station_ptrs_t;
typedef vector<Ship*>              ship_ptrs_t;
typedef vector<weapon_ptr_t>       weapon_ptrs_t;
typedef vector<location_ptr_t>     location_ptrs_t;

typedef set<Station*>              station_ptrs_set_t;
typedef set<Ship*>                 ship_ptrs_set_t;


} // tinyspace


#endif // _TINYSPACE_TYPES_HPP_
