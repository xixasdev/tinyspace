// tiny-space.cpp (v2) (C++11)
// AUTHOR: xixas | DATE: 2022.01.30 | LICENSE: WTFPL/PDM/CC0... your choice
//
// DESCRIPTION: Gaming: Space sim in a tiny package.
//                      Work In Progress.
//
// BUILD: g++ --std=c++11 -O3 -lpthread -o tiny-space tiny-space.cpp
//
// CREATED:  2022.01.22 | AUTHOR: xixas | (v1) Original
// MODIFIED: 2022.01.23 | AUTHOR: xixas | (v2) Added colors and jumpgate travel
//           2022.01.30 | AUTHOR: xixas | (v3)
//                                      | Randomized jumpgate position
//                                      | Added factions and player-owner ships
//                                      | Added stations, docking, repair
//                                      | Added combat, killscreen, respawn


#include <algorithm>
#include <atomic>
#include <chrono>
#include <cmath>
#include <cstdio>
#include <cstring>
#include <cstdlib>
#include <ctime>
#include <initializer_list>
#include <iomanip>
#include <iostream>
#include <map>
#include <memory>
#include <set>
#include <sstream>
#include <string>
#include <thread>
#include <unordered_set>
#include <utility>
#include <vector>


using std::chrono::duration;
using std::chrono::milliseconds;
using std::chrono::steady_clock;
using std::chrono::time_point;
using std::cout;
using std::endl;
using std::map;
using std::ostringstream;
using std::pair;
using std::set;
using std::shared_ptr;
using std::string;
using std::thread;
using std::vector;


// ---------------------------------------------------------------------------
// TYPES
// ---------------------------------------------------------------------------


template <typename T, typename U=float> struct Vector2;
typedef size_t ID;
typedef Vector2<int> v2int_t;
typedef Vector2<size_t> v2size_t;
typedef Vector2<float> v2float_t;
typedef v2float_t dimensions_t;
typedef v2float_t position_t;
typedef v2float_t direction_t;
typedef float speed_t;
typedef float distance_t;
typedef pair<position_t, position_t> position_pair_t;


// ---------------------------------------------------------------------------
// GAME CLASSES
// ---------------------------------------------------------------------------


template <typename T>
inline T sign(T t) { return t > T(0) ? 1 : t < T(0) ? -1 : t; }


template <typename T, typename U> struct Vector2 {
    T x;
    T y;

    Vector2() : x(), y() {}
    Vector2(T const& x, T const& y) : x(x), y(y) {}
    Vector2(Vector2<T,U> const& o) : x(o.x), y(o.y) {}

    Vector2<T,U>& operator =(Vector2<T,U> const& o) { x = o.x, y = o.y; return *this; }

    Vector2<T,U> operator +(Vector2<T,U> const& o) const { return {x+o.x, y+o.y}; }
    Vector2<T,U> operator -(Vector2<T,U> const& o) const { return {x-o.x, y-o.y}; }

    Vector2<T,U> operator -() const { return {-x, -y}; }

    Vector2<T,U> operator *(U u) const { return {x*u, y*u}; }
    Vector2<T,U> operator /(U u) const { return {x/u, y/u}; }
    Vector2<T,U> operator +(U u) const { return *this + (this->normalized() * u); }
    Vector2<T,U> operator -(U u) const { return *this - (this->normalized() * u); }

    U magnitude() const { return sqrt(x*x + y*y); }
    Vector2<T,U> normalized() const { U m = magnitude(); if (!m) return *this; return {static_cast<T>(x/m), static_cast<T>(y/m)}; }

    Vector2<T,U> port() const { return {-y, x}; }       // relative left
    Vector2<T,U> starboard() const { return {y, -x}; }  // relative right

    T dot(Vector2<T,U> const& o) const { return x*o.x + y*o.y; }
    T cross(Vector2<T,U> const& o) const { return x*o.y - y*o.x; }

    // Faster, but less accurate for very small angles
    T angleRad(Vector2<T,U> const& o) const { return sign(starboard().dot(o)) * acos(dot(o) / (magnitude() * o.magnitude())); }
    T angleDeg(Vector2<T,U> const& o) const { return angleRad(o) * 180.f / M_PI; }

    // Slower, but more accurate for very small angles
    T angleRad2(Vector2<T,U> const& o) const { return -atan2(cross(o), dot(o)); }
    T angleDeg2(Vector2<T,U> const& o) const { return angleRad2(o) * 180.f / M_PI; }
};
template <typename T, typename U> inline Vector2<T,U> operator +(Vector2<T,U> const& lhs, Vector2<T,U> const& rhs) { return {lhs.x + rhs.x, lhs.y + rhs.y}; }
template <typename T, typename U> inline Vector2<T,U> operator -(Vector2<T,U> const& lhs, Vector2<T,U> const& rhs) { return {lhs.x - rhs.x, lhs.y - rhs.y}; }
template <typename T, typename U> inline std::ostream& operator <<(std::ostream& os, const Vector2<T,U>& o) { os << '{' << o.x << ',' << o.y << '}'; return os; }


enum IDType { IDType_NONE, IDType_Sector, IDType_Jumpgate, IDType_Station, IDType_Ship, IDType_Weapon, IDType_END };
struct HasID {
    ID id;
    IDType idType;

    HasID(IDType const& idType) : id(++curId), idType(idType) {}
    HasID(ID const& id, IDType const& idType) : id(id), idType(idType) {}

    void resetId() { id = ++curId; }
private:
    static ID curId;
};
ID HasID::curId = 0;


struct HasCode {
    string code;

    HasCode(string const& code="") : code(code) {}
};


struct HasName {
    string name;

    HasName(string const& name="") : name(name) {}
};


struct HasSize {
    dimensions_t size;

    HasSize(dimensions_t const& size={0, 0}) : size(size) {}
};


struct HasPosition {
    position_t position;

    HasPosition(position_t const& position={0, 0}) : position(position) {}
};


struct HasDirection {
    direction_t direction;

    HasDirection(direction_t const& direction={0, 0}) : direction(direction) {}
};


typedef float speed_t;
struct HasSpeed {
    speed_t speed;

    HasSpeed(speed_t const& speed=0) : speed(speed) {}
};


struct Sector;
struct HasSector {
    Sector* sector;

    HasSector(Sector* const sector=nullptr) : sector(sector) {}
};


struct HasSectorAndPosition : public HasSector, public HasPosition {

    HasSectorAndPosition(Sector* const sector, position_t const& position)
        : HasSector(sector), HasPosition(position) {}

    virtual ~HasSectorAndPosition() {}
};


struct Destination;
typedef shared_ptr<Destination> destination_ptr;
struct HasDestination {
    destination_ptr destination;

    HasDestination(destination_ptr destination=nullptr) : destination(destination) {}
};


struct SectorNeighbors {
    Sector* north;
    Sector* east;
    Sector* south;
    Sector* west;

    SectorNeighbors() : north(nullptr), east(nullptr), south(nullptr), west(nullptr) {}

    int count() { return (north ? 1 : 0) + (east  ? 1 : 0) + (south ? 1 : 0) + (west  ? 1 : 0); }

    vector<Sector*> all() const {
        vector<Sector*> sectors;
        if (north) sectors.push_back(north);
        if (east)  sectors.push_back(east);
        if (south) sectors.push_back(south);
        if (west)  sectors.push_back(west);
        return sectors;
    };
};


struct Jumpgate;
struct SectorJumpgates {
    Jumpgate* north;
    Jumpgate* east;
    Jumpgate* south;
    Jumpgate* west;

    SectorJumpgates() : north(nullptr), east(nullptr), south(nullptr), west(nullptr) {}

    int count() { return (north ? 1 : 0) + (east  ? 1 : 0) + (south ? 1 : 0) + (west  ? 1 : 0); }

    vector<Jumpgate*> all() const {
        vector<Jumpgate*> jumpgates;
        if (north) jumpgates.push_back(north);
        if (east)  jumpgates.push_back(east);
        if (south) jumpgates.push_back(south);
        if (west)  jumpgates.push_back(west);
        return jumpgates;
    };
};


struct Ship;
struct Station;
struct Sector: public HasID, public HasName, public HasSize {
    SectorNeighbors neighbors;
    SectorJumpgates jumpgates;
    set<Station*>   stations;
    set<Ship*>      ships;

    Sector(string const& name="", dimensions_t const& size={0, 0})
        : HasID(IDType_Sector), HasName(name), HasSize(size), neighbors(), ships() {}

    Sector(ID const& id, string const& name="", dimensions_t const& size={0, 0})
        : HasID(id, IDType_Sector), HasName(name), HasSize(size), neighbors(), ships() {}
};


struct Jumpgate : public HasID, public HasSectorAndPosition {
    Jumpgate* target;

    Jumpgate(Sector* const sector=nullptr, position_t const& position={0,0}, Jumpgate* const target = nullptr)
        : HasID(IDType_Jumpgate), HasSectorAndPosition(sector, position), target(target) {};

    Jumpgate(ID const& id, Sector* const sector=nullptr, position_t const& position={0,0}, Jumpgate* const target = nullptr)
        : HasID(id, IDType_Jumpgate), HasSectorAndPosition(sector, position), target(target) {};
};


struct Station : public HasID, public HasSectorAndPosition {
    Station(Sector* const sector=nullptr, position_t const& position={0,0})
        : HasID(IDType_Station), HasSectorAndPosition(sector, position) {};

    Station(ID const& id, Sector* const sector=nullptr, position_t const& position={0,0})
        : HasID(id, IDType_Station), HasSectorAndPosition(sector, position) {};
};


struct Destination : public HasSectorAndPosition {
    HasSectorAndPosition* object;  // sector and position are usually ignored if object is set

    Destination(HasSectorAndPosition& object)
        : HasSectorAndPosition(object.sector, object.position), object(&object) {}

    Destination(Sector& sector, position_t const& position)
        : HasSectorAndPosition(&sector, position), object(nullptr) {}

    Sector* currentSector() { return object ? object->sector : sector; }
    position_t currentPosition() { return object ? object->position : position; }
};


enum WeaponType { WeaponType_NONE, WeaponType_Pulse, WeaponType_Cannon, WeaponType_Beam, WeaponType_END };
struct Weapon : public HasID {
    WeaponType type;
    bool isTurret;
    HasSectorAndPosition* parent;
    HasSectorAndPosition* target;
    // weaponPosition designates forward mount (0), port (-1), or starboard (1) -- doesn't apply to turrets
    int weaponPosition;
    float cooldown;
    
    Weapon(WeaponType type, bool isTurret, int weaponPosition, HasSectorAndPosition& parent)
        :
        HasID(IDType_Weapon),
        type(type), isTurret(isTurret), weaponPosition(weaponPosition), parent(&parent),
        target(nullptr), cooldown(0) {};

    Weapon(ID const& id, WeaponType type, bool isTurret, int weaponPosition, HasSectorAndPosition& parent, HasSectorAndPosition* const target, float cooldown)
        :
        HasID(id, IDType_Weapon),
        type(type), isTurret(isTurret), weaponPosition(weaponPosition), parent(&parent),
        target(target), cooldown(cooldown) {};
};


typedef shared_ptr<Weapon> weapon_ptr;
// ship type in order of priority of target importance, least to greatest, all civilian ships first
enum ShipType { ShipType_NONE, ShipType_Courier, ShipType_Transport, ShipType_Scout, ShipType_Corvette, ShipType_Frigate, ShipType_END };
string shipClass(ShipType const& shipType) {
    switch (shipType) {
        case ShipType_Courier:   return "Courier";
        case ShipType_Transport: return "Transport";
        case ShipType_Scout:     return "Scout";
        case ShipType_Corvette:  return "Corvette";
        case ShipType_Frigate:   return "Frigate";
        default:                 return "";
    }
}
string paddedShipClass(ShipType const& shipType) {
    switch (shipType) {
        case ShipType_Courier:   return "Courier  ";
        case ShipType_Transport: return "Transport";
        case ShipType_Scout:     return "Scout    ";
        case ShipType_Corvette:  return "Corvette ";
        case ShipType_Frigate:   return "Frigate  ";
        default:                 return "         ";
    }
}
enum ShipFaction { ShipFaction_Neutral, ShipFaction_Player, ShipFaction_Friend, ShipFaction_Foe, ShipFaction_END };
struct Ship : public HasID, public HasCode, public HasName,
              public HasSectorAndPosition,
              public HasDirection, public HasSpeed,
              public HasDestination
{
    ShipType              type;
    ShipFaction           faction;
    unsigned int          maxHull, currentHull;
    vector<weapon_ptr>    weapons;
    vector<weapon_ptr>    turrets;
    HasSectorAndPosition* target;
    bool                  docked;
    double                timeout; // used any time the ship needs a delay (docked, dead, etc)

    Ship(ShipType type, const unsigned int hull,
        string const& code="", string const& name="",
        Sector* const sector=nullptr, position_t const& position={0, 0},
        direction_t const& direction={0, 0}, speed_t const& speed=0,
        destination_ptr destination=nullptr)
        :
        HasID(IDType_Ship), HasName(name), HasCode(code),
        HasSectorAndPosition(sector, position),
        HasDirection(direction), HasSpeed(speed),
        HasDestination(destination),
        type(type), maxHull(hull), currentHull(hull),
        faction(ShipFaction_Neutral), weapons(), turrets(), target(nullptr), docked(false), timeout(0.f) {}

    Ship(ID const& id, ShipType type, ShipFaction faction, const unsigned int maxHull, const unsigned int currentHull,
        string const& code, string const& name,
        Sector* const sector, position_t const& position,
        direction_t const& direction, speed_t const& speed,
        destination_ptr destination,
        HasSectorAndPosition* target, bool docked, float timeout)
        :
        HasID(id, IDType_Ship), HasName(name), HasCode(code),
        HasSectorAndPosition(sector, position),
        HasDirection(direction), HasSpeed(speed),
        HasDestination(destination),
        type(type), maxHull(maxHull), currentHull(currentHull),
        faction(faction), weapons(), turrets(), target(target), docked(docked), timeout(timeout) {}

    Ship& operator =(Ship const& o) {
        id = o.id;
        code = o.code;
        name = o.name;
        sector = o.sector;
        position = o.position;
        direction = o.direction;
        speed = o.speed;
        destination = o.destination;

        type = o.type;
        maxHull = o.maxHull;
        currentHull = o.currentHull;
        faction = o.faction;

        weapons.clear();
        weapons.reserve(o.weapons.size());
        for (auto weapon : o.weapons) {
            weapons.emplace(weapons.end(), weapon_ptr(new Weapon(weapon->type, false, weapon->weaponPosition, *this)));
        }

        turrets.clear();
        turrets.reserve(o.turrets.size());
        for (auto turret : o.turrets) {
            turrets.emplace(turrets.end(), weapon_ptr(new Weapon(turret->type, true, turret->weaponPosition, *this)));
        }

        target = o.target;
        docked = o.docked;
        timeout = o.timeout;

        return *this;
    }

    vector<Weapon*> weaponsAndTurrets() {
        vector<Weapon*> r;
        r.reserve(weapons.size() + turrets.size());
        for (auto weapon : weapons) r.push_back(&*weapon);
        for (auto turret : turrets) r.push_back(&*turret);
        return r;
    }
};
inline bool operator <(const Ship& lhs, const Ship& rhs) { return lhs.id < rhs.id; }
inline bool operator ==(const Ship& lhs, const Ship& rhs) { return lhs.id == rhs.id; }
namespace std { template<> struct hash<Ship> { ID operator ()(Ship const& o) const noexcept { return o.id; } }; }


enum TargetType {
    TargetType_NONE,
    // ships
    TargetType_Courier, TargetType_Transport, TargetType_Scout, TargetType_Corvette, TargetType_Frigate,
    TargetType_END
};


// ---------------------------------------------------------------------------
// UTILITY
// ---------------------------------------------------------------------------


// Returns a value between min and max (inclusive)
inline float randFloat(float min, float max) {
    return min + (static_cast<float>(rand()) / static_cast<float>(RAND_MAX/(max-min)));
}
inline float randFloat(float max=1.0f) {
    return randFloat(0.f, max);
}


inline position_t randPosition(position_t min, position_t max) {
    return {randFloat(min.x, max.x), randFloat(min.y, max.y)};
}
inline position_t randPosition(position_pair_t minMax) {
    return randPosition(minMax.first, minMax.second);
}
inline position_t randPosition(dimensions_t dimensions, float wallBuffer=0.0f) {
    return randPosition({0.0f+wallBuffer, 0.0f+wallBuffer}, {dimensions.x-wallBuffer, dimensions.y-wallBuffer});
}


inline direction_t randDirection() {
    direction_t direction;
    do {
        direction = {randFloat(-1.0f, 1.0f), randFloat(-1.0f, 1.0f)};
    } while (!direction.magnitude());
    return direction.normalized();
}


inline speed_t randSpeed(speed_t max) { return randFloat(0.0f, max); }


inline ShipType randShipType() { return static_cast<ShipType>(1 + (rand() % (ShipType_END - 1))); }


string randCode() {
    char buf[8];
    size_t i;
    for (i=0; i<3; ++i) buf[i] = 'A' + static_cast<char>(rand() % ('Z'-'A'+1));
    buf[3] = '-';
    for (i=4; i<7; ++i) buf[i] = '0' + static_cast<char>(rand() % ('9'-'0'+1));
    buf[7] = '\0';
    return buf;
}


string randName(ShipType const& shipType) {
    static size_t scoutCur     = 0;
    static size_t corvetteCur  = 0;
    static size_t frigateCur   = 0;
    static size_t transportCur = 0;
    ostringstream os;
    switch (shipType) {
        case ShipType_Courier:   os << "Z" << std::setfill('0') << std::setw(3) << ++transportCur; break;
        case ShipType_Transport: os << "T" << std::setfill('0') << std::setw(3) << ++transportCur; break;
        case ShipType_Scout:     os << "S" << std::setfill('0') << std::setw(3) << ++scoutCur;     break;
        case ShipType_Corvette:  os << "C" << std::setfill('0') << std::setw(3) << ++corvetteCur;  break;
        case ShipType_Frigate:   os << "F" << std::setfill('0') << std::setw(3) << ++frigateCur;   break;
        default:                                              break;
    }
    return os.str();
}


destination_ptr randDestination(Sector& sector, bool useJumpgates, float miscChance=0.f, vector<HasSectorAndPosition*>* excludes=nullptr) {
    bool isMisc = false;
    if (miscChance) isMisc = randFloat() <= miscChance;
    if (!isMisc) {
        vector<HasSectorAndPosition*> potentialDestinations;
        potentialDestinations.reserve(sector.stations.size() + sector.jumpgates.count());

        potentialDestinations.insert(potentialDestinations.end(), sector.stations.begin(), sector.stations.end());
        if (useJumpgates) {
            auto jumpgates = sector.jumpgates.all();
            potentialDestinations.insert(potentialDestinations.end(), jumpgates.begin(), jumpgates.end());
        }

        if (excludes) {
            for (auto exclude : *excludes) {
                auto it = std::find(potentialDestinations.begin(), potentialDestinations.end(), exclude);
                if (it != potentialDestinations.end()) {
                    potentialDestinations.erase(it);
                }
            }
        }

        if (!potentialDestinations.empty()) {
            auto destinationObject = potentialDestinations[rand() % potentialDestinations.size()];
            return destination_ptr(new Destination(*destinationObject));
        }
    }
    return destination_ptr(new Destination(sector, randPosition({0,0}, sector.size)));
}


// ---------------------------------------------------------------------------
// PROGRAM
// ---------------------------------------------------------------------------


const v2size_t     SECTOR_BOUNDS           = {10, 10};
const dimensions_t SECTOR_SIZE             = {20, 20};
const size_t       SECTOR_MAP_LEFT_PADDING = 6;
const size_t       SHIP_COUNT              = 500;
const float        JUMPGATE_EDGE_BUFFER    = 0.25f;
const size_t       MAX_STATIONS_PER_SECTOR = 5;
const float        NO_STATIONS_FREQUENCY   = 0.25;
const float        PLAYER_FREQUENCY        = 0.01f;
const float        FRIEND_FREQUENCY        = 0.2f;
const float        ENEMY_FREQUENCY         = 0.1f;
const float        MISC_DESTINATION_CHANCE = 0.1f;
const size_t       TICK_TIME               = 300;  // milliseconds
const float        DOCK_TIME               = 3.f;  // seconds
const float        RESPAWN_TIME            = 10.f; // seconds

const position_pair_t
    GATE_RANGE_NORTH {{SECTOR_SIZE.x/3.f + 0.1f, 0.25f},               {2*SECTOR_SIZE.x/3.f - 0.1f, SECTOR_SIZE.y/5.f}},
    GATE_RANGE_EAST  {{4*SECTOR_SIZE.x/5.f, SECTOR_SIZE.y/3.f + 0.1f}, {SECTOR_SIZE.x - 0.25f, 2*SECTOR_SIZE.y/3.f - 0.1f}},
    GATE_RANGE_SOUTH {{SECTOR_SIZE.x/3.f + 0.1f, 4*SECTOR_SIZE.y/5.f}, {2*SECTOR_SIZE.x/3.f - 0.1f, SECTOR_SIZE.y - 0.25f}},
    GATE_RANGE_WEST  {{0.25f, SECTOR_SIZE.y/3.f + 0.1f},               {SECTOR_SIZE.x/5.f, 2*SECTOR_SIZE.y/3.f - 0.1f}};

const speed_t DISTANCE_MULTIPLIER = 0.002f;

const speed_t
    COURIER_SPEED   = 600 * DISTANCE_MULTIPLIER,
    TRANSPORT_SPEED = 300 * DISTANCE_MULTIPLIER,
    SCOUT_SPEED     = 500 * DISTANCE_MULTIPLIER,
    CORVETTE_SPEED  = 400 * DISTANCE_MULTIPLIER,
    FRIGATE_SPEED   = 200 * DISTANCE_MULTIPLIER;

const unsigned int
    COURIER_HULL   = 300,
    TRANSPORT_HULL = 800,
    SCOUT_HULL     = 500,
    CORVETTE_HULL  = 1200,
    FRIGATE_HULL   = 1800;

const distance_t
    PULSE_RANGE    = 1000 * DISTANCE_MULTIPLIER,
    CANNON_RANGE   = 2000 * DISTANCE_MULTIPLIER,
    BEAM_RANGE     =  750 * DISTANCE_MULTIPLIER;

const distance_t MAX_TO_HIT_RANGE = 2000 * DISTANCE_MULTIPLIER; // match longest range weapon

// Cooldown in seconds
const float
    PULSE_COOLDOWN  = 1.f/3,
    CANNON_COOLDOWN = 1.f,
    BEAM_COOLDOWN   = 0.f;

const unsigned int
    PULSE_DAMAGE   = 20, // per shot
    CANNON_DAMAGE  = 60, // per shot
    BEAM_DAMAGE    = 20; // per second

const float
    PULSE_ACCURACY  = 0.8f,
    CANNON_ACCURACY = 0.5f,
    BEAM_ACCURACY   = 0.95f;

const float  // for targets
    COURIER_ACCURACY_MULTIPLIER   = 0.75f,
    TRANSPORT_ACCURACY_MULTIPLIER = 1.f,
    SCOUT_ACCURACY_MULTIPLIER     = 0.6f,
    CORVETTE_ACCURACY_MULTIPLIER  = 1.2f,
    FRIGATE_ACCURACY_MULTIPLIER   = 1.8f;

const float TURRET_RANGE_SCALE = 0.5;
const float TURRET_DAMAGE_SCALE = 0.7;


speed_t shipSpeed(ShipType shipType) {
    switch (shipType) {
        case ShipType_Courier:   return COURIER_SPEED;
        case ShipType_Transport: return TRANSPORT_SPEED;
        case ShipType_Scout:     return SCOUT_SPEED;
        case ShipType_Corvette:  return CORVETTE_SPEED;
        case ShipType_Frigate:   return FRIGATE_SPEED;
        default:                 return 0;
    }
}


unsigned int shipHull(ShipType shipType) {
    switch (shipType) {
        case ShipType_Courier:   return COURIER_HULL;
        case ShipType_Transport: return TRANSPORT_HULL;
        case ShipType_Scout:     return SCOUT_HULL;
        case ShipType_Corvette:  return CORVETTE_HULL;
        case ShipType_Frigate:   return FRIGATE_HULL;
        default:                 return 0;
    }
}


// Returns whether a ship has side-mounted weapons (as opposed to forward-mounted)
bool isShipSideFire(ShipType shipType) {
    switch (shipType) {
        case ShipType_Frigate: return true;
        default:               return false;
    }
}


vector<WeaponType> shipWeapons(ShipType shipType)  {
    switch (shipType) {
//        case ShipType_Courier:   return {};
//        case ShipType_Transport: return {};
        case ShipType_Scout:     return {WeaponType_Pulse, WeaponType_Pulse};
        case ShipType_Corvette:  return {WeaponType_Pulse, WeaponType_Pulse, WeaponType_Cannon};
        case ShipType_Frigate:   return {WeaponType_Cannon, WeaponType_Cannon}; // Frigates fire from the side, so these weapons are doubled
        default:                 return {};
    }
}


vector<WeaponType> shipTurrets(ShipType shipType) {
    switch (shipType) {
        case ShipType_Courier:   return {WeaponType_Pulse};
        case ShipType_Transport: return {WeaponType_Pulse, WeaponType_Pulse};
//        case ShipType_Scout:     return {};
        case ShipType_Corvette:  return {WeaponType_Pulse, WeaponType_Pulse};
        case ShipType_Frigate:   return {WeaponType_Pulse, WeaponType_Pulse, WeaponType_Beam, WeaponType_Beam};
        default:                 return {};
    }
}


TargetType shipTypeToTargetType(ShipType shipType) {
    switch (shipType) {
        case ShipType_Courier:   return TargetType_Courier;
        case ShipType_Transport: return TargetType_Transport;
        case ShipType_Scout:     return TargetType_Scout;
        case ShipType_Corvette:  return TargetType_Corvette;
        case ShipType_Frigate:   return TargetType_Frigate;
        default:                 return TargetType_NONE;
    }
}


float weaponDamage(WeaponType weaponType, bool isTurret) {
    float r;
    switch (weaponType) {
        case WeaponType_Pulse:  r = PULSE_DAMAGE;  break;
        case WeaponType_Cannon: r = CANNON_DAMAGE; break;
        case WeaponType_Beam:   r = BEAM_DAMAGE;   break;
        default:                return 0.f;
    }
    if (isTurret) r *= TURRET_DAMAGE_SCALE;
    return r;
}


float weaponCooldown(WeaponType weaponType) {
    switch (weaponType) {
        case WeaponType_Pulse:  return PULSE_COOLDOWN;
        case WeaponType_Cannon: return CANNON_COOLDOWN;
        case WeaponType_Beam:   return BEAM_COOLDOWN;
        default:                return 0.f;
    }
}

float isWeaponDamageOverTime(WeaponType weaponType) {
    switch (weaponType) {
        case WeaponType_Beam: return true;
        default:              return false;
    }
}


float chanceToHit(WeaponType weaponType, bool isTurret, TargetType targetType, distance_t distance) {
    distance_t range;
    switch (weaponType) {
        case WeaponType_Pulse:  range = PULSE_RANGE;  break;
        case WeaponType_Cannon: range = CANNON_RANGE; break;
        case WeaponType_Beam:   range = BEAM_RANGE;   break;
        default:                range = 0;            break;
    }
    if (isTurret) range *= TURRET_RANGE_SCALE;
    if (range < distance) return 0.f;

    float chance;
    switch (weaponType) {
        case WeaponType_Pulse:  chance = PULSE_ACCURACY;  break;
        case WeaponType_Cannon: chance = CANNON_ACCURACY; break;
        case WeaponType_Beam:   chance = BEAM_ACCURACY;   break;
        default:                chance = 0;               break;
    }
    switch (targetType) {
        case TargetType_Courier:    chance *= COURIER_ACCURACY_MULTIPLIER; break;
        case TargetType_Transport:  chance *= COURIER_ACCURACY_MULTIPLIER; break;
        case TargetType_Scout:      chance *= COURIER_ACCURACY_MULTIPLIER; break;
        case TargetType_Corvette:   chance *= COURIER_ACCURACY_MULTIPLIER; break;
        case TargetType_Frigate:    chance *= COURIER_ACCURACY_MULTIPLIER; break;
        default:                                                           break;
    }
    return chance;
}


// weaponPosition designates forward mount (0), port (-1), or starboard (1); 
float chanceToHit(Weapon const& weapon, bool isTurret, int weaponPosition=0, HasSectorAndPosition* potentialTarget=nullptr) {
    HasSectorAndPosition* parent = weapon.parent;
    HasSectorAndPosition* target = potentialTarget ? potentialTarget : weapon.target;
    if (!target || !parent || parent->sector != target->sector) return 0.f;
    direction_t targetVector = target->position - parent->position;
    if (targetVector.magnitude() > MAX_TO_HIT_RANGE) return 0.f;
    if (!isTurret) {
        if (Ship* ship = dynamic_cast<Ship*>(parent)) {
            direction_t const& dir = ship->direction;
            direction_t aim = !weaponPosition ? dir : weaponPosition < 0 ? dir.port() : dir.starboard();
            // +/-45 = 90 degree aim window
            if (abs(aim.angleDeg(targetVector)) > 45) return 0.f;
        } else {
            return 0.f;
        }
    }
    TargetType targetType = TargetType_NONE;
    if (Ship* targetShip = dynamic_cast<Ship*>(target)) {
        targetType = shipTypeToTargetType(targetShip->type);
    }
    return chanceToHit(weapon.type, isTurret, targetType, targetVector.magnitude());
}


// ---------------------------------------------------------------------------
// UI
// ---------------------------------------------------------------------------


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
string beginColorString(unsigned int color, bool useColor=true, bool bold=false) {
    if (!useColor) return "";
    ostringstream os;
    os << "\033[" << (bold ? '1' : '0') << ";" << color << 'm';
    return os.str();
}
string endColorString(bool useColor=true, unsigned int defaultColor=0) {
    if (!useColor) return "";
    if (defaultColor) return beginColorString(defaultColor);
    return "\033[0m";
}
string colorString(unsigned int color, string const& str, bool useColor=true, bool bold=false, unsigned int defaultColor=0) {
    if (!useColor) return str;
    ostringstream os;
    os << beginColorString(color, true, bold) << str << endColorString(true, defaultColor);
    return os.str();
}


string shipString(Ship const& ship, bool useColor=false, unsigned int color=0) {
    useColor = useColor && color != 0;
    ostringstream os;
    if (useColor) os << beginColorString(color);
    if (ship.code.size()) os << /*" code:"*/ " " << ship.code;
    float hull = ship.currentHull / static_cast<float>(ship.maxHull);
    if (useColor) {
        os << " "
        << colorString(hull > 0.0f ? COLOR_BRIGHT_CYAN : COLOR_BRIGHT_BLACK, "|", true, true, color)
        << colorString(hull > 0.2f ? COLOR_BRIGHT_CYAN : COLOR_BRIGHT_BLACK, "|", true, true, color)
        << colorString(hull > 0.4f ? COLOR_BRIGHT_CYAN : COLOR_BRIGHT_BLACK, "|", true, true, color)
        << colorString(hull > 0.6f ? COLOR_BRIGHT_CYAN : COLOR_BRIGHT_BLACK, "|", true, true, color)
        << colorString(hull > 0.8f ? COLOR_BRIGHT_CYAN : COLOR_BRIGHT_BLACK, "|", true, true, color);
    } else {
        os << " "
        << (hull > 0.0f ? "|" : " ")
        << (hull > 0.2f ? "|" : " ")
        << (hull > 0.4f ? "|" : " ")
        << (hull > 0.6f ? "|" : " ")
        << (hull > 0.8f ? "|" : " ");
    }
    auto loc = ship.position - (ship.sector ? ship.sector->size/2 : dimensions_t{0, 0});
    auto& dir = ship.direction;
    os << std::fixed << std::setprecision(0)
        << /*" loc:("*/ " ["
        << (loc.x >= 0 ? " " : "") << loc.x << ","
        << (-loc.y >= 0 ? " " : "") << -loc.y << "]";
    os << /*" dir:"*/ " "
       << (dir.y <= -0.3 ? "N" : dir.y >= 0.3 ? "S" : " ")
       << (dir.x <= -0.3 ? "W" : dir.x >= 0.3 ? "E" : " ");
    os << /*" class:"*/ " " << paddedShipClass(ship.type);
    if (ship.target && ship.sector == ship.target->sector) {
        if (auto target = dynamic_cast<Ship*>(ship.target)) {
            os << " -> "
               << beginColorString(target->faction == ShipFaction_Player ? PLAYER_COLOR
                                  :target->faction == ShipFaction_Friend ? FRIEND_COLOR
                                  :target->faction == ShipFaction_Foe    ? ENEMY_COLOR
                                  :                                        NEUTRAL_COLOR
                                  ,
                                  useColor)
               << paddedShipClass(target->type);
            os << /*" code:"*/ " " << target->code
               << endColorString(useColor, color);
            float targetHull = target->currentHull / static_cast<float>(target->maxHull);
            if (useColor) {
                os << " "
                << colorString(targetHull > 0.0f ? COLOR_BRIGHT_CYAN : COLOR_BRIGHT_BLACK, "|", true, true, color)
                << colorString(targetHull > 0.2f ? COLOR_BRIGHT_CYAN : COLOR_BRIGHT_BLACK, "|", true, true, color)
                << colorString(targetHull > 0.4f ? COLOR_BRIGHT_CYAN : COLOR_BRIGHT_BLACK, "|", true, true, color)
                << colorString(targetHull > 0.6f ? COLOR_BRIGHT_CYAN : COLOR_BRIGHT_BLACK, "|", true, true, color)
                << colorString(targetHull > 0.8f ? COLOR_BRIGHT_CYAN : COLOR_BRIGHT_BLACK, "|", true, true, color);
            } else {
                os << " "
                << (targetHull > 0.0f ? "|" : " ")
                << (targetHull > 0.2f ? "|" : " ")
                << (targetHull > 0.4f ? "|" : " ")
                << (targetHull > 0.6f ? "|" : " ")
                << (targetHull > 0.8f ? "|" : " ");
            }
        }
    }
    if (useColor) os << endColorString();
    return os.str();
}


vector<string> createSectorShipsList(Sector& sector, Ship* playerShip, bool const useColor) {
    vector<string> shipsList;
    for (auto ship : sector.ships) {
        if (ship->docked) continue;
        ostringstream os;
        bool isPlayerShip    = ship == playerShip;
        bool isPlayerFaction = ship->faction == ShipFaction_Player;
        bool isFriend        = ship->faction == ShipFaction_Friend;
        bool isEnemy         = ship->faction == ShipFaction_Foe;
        unsigned int color = 0;
        if (useColor) {
            color = ship->currentHull <= 0 ? COLOR_BRIGHT_BLACK
                  : isPlayerFaction        ? PLAYER_COLOR
                  : isFriend               ? FRIEND_COLOR
                  : isEnemy                ? ENEMY_COLOR
                  :                          NEUTRAL_COLOR
                  ;
        }
        string shipStr = shipString(*ship, useColor, color);
        os << ' '
           << ( isPlayerShip    ? '>'
              : isPlayerFaction ? '.'
              : isFriend        ? '+'
              : isEnemy         ? '-'
              :                   ' '
              )
           << shipStr;
        shipsList.push_back(os.str());
    }
    return shipsList;
}


vector<string> createSectorMap(Sector& sector, Ship* playerShip, bool useColor) {
    static string leftPadding(SECTOR_MAP_LEFT_PADDING, ' ');
    auto& sectorSize = sector.size;
    vector<string> sectorMap; sectorMap.reserve(static_cast<size_t>(sectorSize.x+3));
    { // header
        ostringstream os;
        os << leftPadding << '+' << string((sectorSize.x+1) * 3, '-') << '+';
        sectorMap.push_back(os.str());
    }
    for (size_t i=0; i<=sectorSize.y; ++i) {
        ostringstream os;
        os << leftPadding << '|' << string((sectorSize.x+1) * 3, ' ') << '|';
        sectorMap.push_back(os.str());
    }
    { // footer
        ostringstream os;
        os << leftPadding << "+-[ "
           << colorString(PLAYER_COLOR, sector.name, useColor)
           << " ]" << string(((sectorSize.x+1) * 3) - sector.name.size() - 5, '-')
           << '+';
        sectorMap.push_back(os.str());
    }

    // For character replacements
    // map<(row,col), replacement_str>
    map<pair<size_t, size_t>, string> replacements;

    auto addReplacementString = [&replacements, &useColor](pair<size_t, size_t> const& pos, unsigned int const color, string const& str, bool colorEach=false) {
        for (size_t i=0, j=str.size()-1; i<=j; ++i) {
            ostringstream os;
            if (i==0 || colorEach) os << beginColorString(color, useColor);
            os << str[i];
            if (i==j || colorEach) os << endColorString(useColor);
            replacements[{pos.first, pos.second+i}] = os.str();
        }
    };

    // ships
    for (auto ship : sector.ships) {
        bool isPlayerShip    = ship == playerShip;
        bool isPlayerFaction = ship->faction == ShipFaction_Player;
        bool isFriend        = ship->faction == ShipFaction_Friend;
        bool isEnemy         = ship->faction == ShipFaction_Foe;
        auto& pos = ship->position;
        size_t col=0, row=0;
        for (size_t i=0; i<sector.size.x+1; ++i) {
            if (pos.x >= -0.5f+i && pos.x < 0.5+i) {
                col = leftPadding.size() + 1 + i*3 + 1;
                break;
            }
        }
        for (size_t i=0; i<sector.size.y+1; ++i) {
            if (pos.y >= -0.5f+i && pos.y < 0.5+i) {
                row = 1 + i;
                break;
            }
        }
        if (!replacements.count({row,col}) || isPlayerShip) {
            string shipStr = ".";
            if (isPlayerShip) {
                auto& dir = ship->direction;
                auto dirMax = std::max(dir.y, 0.0f);
                shipStr = "v";
                if (dir.x > 0 && dir.x > dirMax) { dirMax = dir.x; shipStr = ">"; }
                if (dir.y < 0 && std::abs(dir.y) > dirMax) { dirMax = std::abs(ship->direction.y); shipStr = "^"; }
                if (dir.x < 0 && std::abs(dir.x) > dirMax) { shipStr = "<"; }
            }
            unsigned int color = 0;
            if (useColor) {
                color = ship->currentHull <= 0.f ? COLOR_BRIGHT_BLACK
                      : isPlayerFaction          ? PLAYER_COLOR
                      : isFriend                 ? FRIEND_COLOR
                      : isEnemy                  ? ENEMY_COLOR
                      :                            NEUTRAL_COLOR;
            }
            addReplacementString({row, col}, color, shipStr);
        }
    }

    // stations
    for (auto station : sector.stations) {
        auto const& pos = station->position;
        size_t col=0, row=0;
        for (size_t i=0; i<sector.size.x+1; ++i) {
            if (pos.x >= -0.5f+i && pos.x < 0.5+i) {
                col = leftPadding.size() + 1 + i*3 + 1;
                break;
            }
        }
        for (size_t i=0; i<sector.size.y+1; ++i) {
            if (pos.y >= -0.5f+i && pos.y < 0.5+i) {
                row = 1 + i;
                break;
            }
        }
        addReplacementString({row, col}, STATION_COLOR, "()", true);
    }

    // jumpgates
    for (auto jumpgate : sector.jumpgates.all()) {
        auto const& pos = jumpgate->position;
        size_t col=0, row=0;
        for (size_t i=0; i<sector.size.x+1; ++i) {
            if (pos.x >= -0.5f+i && pos.x < 0.5+i) {
                col = leftPadding.size() + 1 + i*3 + 1;
                break;
            }
        }
        for (size_t i=0; i<sector.size.y+1; ++i) {
            if (pos.y >= -0.5f+i && pos.y < 0.5+i) {
                row = 1 + i;
                break;
            }
        }
        addReplacementString({row, col}, JUMPGATE_COLOR, "0");
    }

    // kill screen
    if (playerShip && playerShip->currentHull <= 0) {
        char respawn[20];
        snprintf(respawn, 20, "|  respawn in %2d  |", static_cast<int>(playerShip->timeout));
        vector<string> killscreen = {
            "+-----------------+",
            "| you were killed |",
            respawn,
            "+-----------------+"
        };
        size_t row = sectorMap.size()/2 - killscreen.size()/2 + 1;
        size_t col = sectorMap[0].size()/2 - killscreen[0].size()/2 + 3;
        for (size_t i=0, j=killscreen.size(); i<j; ++i) {
            addReplacementString({row+i, col}, COLOR_BRIGHT_BLACK, killscreen[i]);
        }
    }

    // Character Replacements
    for (auto it = replacements.crbegin(); it != replacements.crend(); ++it) {
        sectorMap[it->first.first].replace(it->first.second, 1, it->second);
    }

    return sectorMap;
}


vector<string> createGlobalMap(vector<vector<Sector>> const& sectors, Ship* playerShip, bool const useColor) {
    vector<string> globalMap;
    globalMap.reserve(sectors.size()+1);
    { // column headers
        ostringstream os;
        os << "    ";
        for (size_t i=0; i<sectors[0].size(); ++i) {
            os << "    " << static_cast<char>('A' + i) << "  ";
        }
        globalMap.push_back(os.str());
    }
    { // header divider
        ostringstream os;
        os << "   ." << string(sectors[0].size() * 7, '-');
        globalMap.push_back(os.str());
    }
    v2size_t playerSectorIndex;
    for (size_t i=0; i<sectors.size(); ++i) {
        ostringstream os;
        char rowBuf[3];
        sprintf(rowBuf, "%02zu", i+1);
        os << rowBuf << " |";
        for (size_t j=0; j<sectors[i].size(); ++j) {
            Sector const& sector = sectors[i][j];
            bool isPlayerSector = playerShip && &sector == playerShip->sector;
            if (isPlayerSector) playerSectorIndex = {i, j};
            size_t shipCount = sector.ships.size();
            bool hasPlayerProperty = false;
            for (auto ship : sector.ships) {
                if (ship->currentHull <= 0.f) --shipCount;
                if (ship->faction == ShipFaction_Player && ship != playerShip) {
                    hasPlayerProperty = true;
                }
            }
            bool usePlayerColor = useColor && (isPlayerSector || hasPlayerProperty);
            if (usePlayerColor) os << beginColorString(PLAYER_COLOR) ;
            os << (isPlayerSector ? "[" : " ");
            if (shipCount) {
                char shipCountBuf[5];
                sprintf(shipCountBuf, "%4zu", shipCount);
                os << shipCountBuf;
            } else {
                os << "    ";
            }
            if (usePlayerColor) os << endColorString();
            os << (hasPlayerProperty ? "." : " ");
            if (usePlayerColor) os << beginColorString(PLAYER_COLOR) ;
            os << (isPlayerSector ? "]" : " ");
            if (usePlayerColor) os << endColorString();
        }
        globalMap.push_back(os.str());
    }
    if (useColor && playerShip && playerShip->sector) {
        string& rowStr = globalMap[2 + playerSectorIndex.x];
        string& colStr = globalMap[0];
        rowStr.replace(0, 2, colorString(PLAYER_COLOR, rowStr.substr(0, 2)));
        colStr.replace(4 + 7*playerSectorIndex.y, 7, colorString(PLAYER_COLOR, colStr.substr(4 + 7*playerSectorIndex.y, 7)));
    }
    return globalMap;
}


void updateDisplay(vector<vector<Sector>>& sectors, Ship& playerShip, bool const useColor) {
    auto shipsList = createSectorShipsList(*playerShip.sector, &playerShip, useColor);
    auto sectorMap = createSectorMap(*playerShip.sector, &playerShip, useColor);
    auto globalMap = createGlobalMap(sectors, &playerShip, useColor);
    
    for (size_t i = 0; i<50; ++i) cout << endl;
    cout << endl;
    for (auto& line : shipsList) cout << line << endl;
    cout << endl;
    for (auto& line : sectorMap) cout << line << endl;
    cout << endl;
    for (auto& line : globalMap) cout << line << endl;
    cout << endl;
}


// ---------------------------------------------------------------------------
// OBJECT INITIALIZATION
// ---------------------------------------------------------------------------


vector<vector<Sector>> initSectors(v2size_t const& bounds, dimensions_t const& size) {
    vector<vector<Sector>> sectors;

    auto& rowCount = bounds.y;
    auto& colCount = bounds.x;

    // Create sectors
    sectors.reserve(rowCount);
    {
        char name[4];
        for (size_t row=0; row<rowCount; ++row) {
            sectors.emplace_back();
            sectors[row].reserve(colCount);
            for (size_t col=0; col<colCount; ++col) {
                sprintf(name, "%c%02zu", 'A'+col, 1+row);
                sectors[row].emplace_back(name, size);
            }
        }
    }

    // Determine neighbors
    for (size_t row=0; row<rowCount; ++row) {
        for (size_t col=0; col<colCount; ++col) {
            auto& sector = sectors[row][col];
            auto& neighbors = sector.neighbors;

            // determine adjacent sectors
            neighbors.north = (row == 0)            ? nullptr : &sectors[row-1][col];
            neighbors.south = (row == rowCount - 1) ? nullptr : &sectors[row+1][col];
            neighbors.west  = (col == 0)            ? nullptr : &sectors[row][col-1];
            neighbors.east  = (col == colCount - 1) ? nullptr : &sectors[row][col+1];
        }
    }

    return sectors;
}


vector<Jumpgate> initJumpgates(vector<vector<Sector>>& sectors, bool useJumpgates) {
    vector<Jumpgate> jumpgatesBuf;
    if (!useJumpgates) return jumpgatesBuf;

    jumpgatesBuf.reserve(sectors.size() * sectors[0].size() * 4);

    auto addJumpgateNorth = [&jumpgatesBuf](Sector& sector) -> bool {
        if (sector.jumpgates.north || !sector.neighbors.north || sector.neighbors.north->jumpgates.south) {
            return false;
        }
        Sector& neighbor = *sector.neighbors.north;
        auto localPos  = randPosition(GATE_RANGE_NORTH);
        auto remotePos = randPosition(GATE_RANGE_SOUTH);
        Jumpgate& localJumpgate  = *(jumpgatesBuf.emplace(jumpgatesBuf.end(), &sector, localPos, nullptr));
        Jumpgate& remoteJumpgate = *(jumpgatesBuf.emplace(jumpgatesBuf.end(), &neighbor, remotePos, &localJumpgate));
        localJumpgate.target = &remoteJumpgate;
        sector.jumpgates.north = &localJumpgate;
        neighbor.jumpgates.south = &remoteJumpgate;
        return true;
    };

    auto addJumpgateEast = [&jumpgatesBuf](Sector& sector) -> bool {
        if (sector.jumpgates.east || !sector.neighbors.east || sector.neighbors.east->jumpgates.west) {
            return false;
        }
        Sector& neighbor = *sector.neighbors.east;
        auto localPos  = randPosition(GATE_RANGE_EAST);
        auto remotePos = randPosition(GATE_RANGE_WEST);
        Jumpgate& localJumpgate  = *(jumpgatesBuf.emplace(jumpgatesBuf.end(), &sector, localPos, nullptr));
        Jumpgate& remoteJumpgate = *(jumpgatesBuf.emplace(jumpgatesBuf.end(), &neighbor, remotePos, &localJumpgate));
        localJumpgate.target = &remoteJumpgate;
        sector.jumpgates.east = &localJumpgate;
        neighbor.jumpgates.west = &remoteJumpgate;
        return true;
    };

    auto addJumpgateSouth = [&jumpgatesBuf](Sector& sector) -> bool {
        if (sector.jumpgates.south || !sector.neighbors.south || sector.neighbors.south->jumpgates.north) {
            return false;
        }
        Sector& neighbor = *sector.neighbors.south;
        auto localPos  = randPosition(GATE_RANGE_SOUTH);
        auto remotePos = randPosition(GATE_RANGE_NORTH);
        Jumpgate& localJumpgate  = *(jumpgatesBuf.emplace(jumpgatesBuf.end(), &sector, localPos, nullptr));
        Jumpgate& remoteJumpgate = *(jumpgatesBuf.emplace(jumpgatesBuf.end(), &neighbor, remotePos, &localJumpgate));
        localJumpgate.target = &remoteJumpgate;
        sector.jumpgates.south = &localJumpgate;
        neighbor.jumpgates.north = &remoteJumpgate;
        return true;
    };

    auto addJumpgateWest = [&jumpgatesBuf](Sector& sector) -> bool {
        if (sector.jumpgates.west || !sector.neighbors.west || sector.neighbors.west->jumpgates.east) {
            return false;
        }
        Sector& neighbor = *sector.neighbors.west;
        auto localPos  = randPosition(GATE_RANGE_WEST);
        auto remotePos = randPosition(GATE_RANGE_EAST);
        Jumpgate& localJumpgate  = *(jumpgatesBuf.emplace(jumpgatesBuf.end(), &sector, localPos, nullptr));
        Jumpgate& remoteJumpgate = *(jumpgatesBuf.emplace(jumpgatesBuf.end(), &neighbor, remotePos, &localJumpgate));
        localJumpgate.target = &remoteJumpgate;
        sector.jumpgates.west = &localJumpgate;
        neighbor.jumpgates.east = &remoteJumpgate;
        return true;
    };

    auto rowCount = sectors.size();
    auto colCount = sectors[0].size();

    // determine jumpgates
    for (size_t row=0; row<rowCount; ++row) {
        for (size_t col=0; col<colCount; ++col) {
            auto& sector = sectors[row][col];
            auto& neighbors = sector.neighbors;
            auto& jumpgates = sector.jumpgates;
            int jumpgatesCount = 1 + (rand() % sector.neighbors.count()) - sector.jumpgates.count();

            // Populate jumpgates in an XY-forward direction
            while (jumpgatesCount > 0 && (
                (neighbors.south && !jumpgates.south) ||
                (neighbors.west  && !jumpgates.west)
            )) {
                if (jumpgatesCount > 0 && neighbors.south && !jumpgates.south && rand() % 2 && addJumpgateSouth(sector)) --jumpgatesCount;
                if (jumpgatesCount > 0 && neighbors.west  && !jumpgates.west  && rand() % 2 && addJumpgateWest(sector))  --jumpgatesCount;
            }

            // Sometimes the last sector is unreachable due to neither the
            // north or west neighbors having given it a jumpgate.
            //
            // This block catches any time no jumpgates are defined.
            // (just in case)
            //
            // If there are no jumpgates, place a joining one in whichever
            // neighboring sector has the fewest jumpgates.
            if (!jumpgates.count()) {
                auto allNeighbors = neighbors.all();
                auto it = allNeighbors.begin();
                if (it != allNeighbors.end()) {

                    Sector* selectedNeighbor = *it;
                    int minJumpgates = selectedNeighbor->jumpgates.count();
                    while (it != allNeighbors.end()) {
                        Sector* neighbor = *it;
                        int count = neighbor->jumpgates.count();
                        if (count < minJumpgates) {
                            selectedNeighbor = neighbor;
                            minJumpgates = count;
                        }
                        ++it;
                    }

                    if      (selectedNeighbor == neighbors.north) addJumpgateNorth(sector);
                    else if (selectedNeighbor == neighbors.east)  addJumpgateEast(sector);
                    else if (selectedNeighbor == neighbors.south) addJumpgateSouth(sector);
                    else if (selectedNeighbor == neighbors.west)  addJumpgateWest(sector);
                }
            }
        }
    }

    return jumpgatesBuf;
}


vector<Station> initStations(vector<vector<Sector>>& sectors) {
    vector<Station> stations;
    size_t rowCount = sectors.size();
    size_t colCount = sectors[0].size();
    for (size_t row=0; row<rowCount; ++row) {
        for (size_t col=0; col<colCount; ++col) {
            Sector& sector = sectors[row][col];
            float t = randFloat(1.f + NO_STATIONS_FREQUENCY);
            if (t > NO_STATIONS_FREQUENCY) {
                int stationCount = 1;
                for (size_t i=MAX_STATIONS_PER_SECTOR; i>1; --i) {
                    if (t > NO_STATIONS_FREQUENCY + 1.f - 1.f/i) {
                        stationCount = i;
                        break;
                    }
                }
                int stationsPlaced = 0;
                for (size_t i=0; i<stationCount; ++i) {
                    position_t pos;
                    float objectDistance = 0.f;
                    for (size_t tries=0; tries<10 && objectDistance<2.f; ++tries) { 
                        pos = randPosition(SECTOR_SIZE, 2.f);
                        objectDistance = 2.f;
                        // maintain distance from jumpgates
                        for (auto jumpgate : sector.jumpgates.all()) {
                            objectDistance = std::min(objectDistance, (jumpgate->position - pos).magnitude());
                            if (objectDistance < 2.f) break;
                        }
                        // maintain distance from other stations
                        if (stationsPlaced && objectDistance >= 2.f) {
                            auto it = stations.rbegin();
                            for (size_t j=0; j<stationsPlaced; ++j) {
                                auto& station = *it;
                                objectDistance = std::min(objectDistance, (station.position - pos).magnitude());
                                if (objectDistance < 2.f) break;
                                ++it;
                            }
                        }
                    }
                    if (objectDistance >= 2.f) {
                        stations.emplace(stations.end(), &sector, pos);
                        ++stationsPlaced;
                    }
                }
            }
        }
    }
    // Reference now that the stations vector size is set (no reallocations)
    for (auto& station : stations) {
        station.sector->stations.insert(&station);
    }
    return stations;
}


vector<Ship> initShips(size_t const& shipCount, vector<vector<Sector>>& sectors, bool useJumpgates, float const& wallBuffer=0.1f) {
    vector<Ship> ships;
    ships.reserve(shipCount);

    for (size_t i=0; i<shipCount; ++i) {
        bool isPlayerShip = i == 0;
        auto& sector = sectors[rand() % sectors.size()][rand() % sectors[0].size()];
        auto type    = randShipType();
        auto hull    = shipHull(type);
        auto code    = randCode();
        auto name    = randName(type);
        auto pos     = randPosition(sector.size, wallBuffer);
        auto dest    = randDestination(sector, useJumpgates, (isPlayerShip ? 0.f : MISC_DESTINATION_CHANCE));
        auto dir     = dest ? (dest->position - pos).normalized() : randDirection();
        auto speed   = shipSpeed(type);
        auto weapons = shipWeapons(type);
        auto turrets = shipTurrets(type);
        auto it      = ships.emplace(ships.end(), type, hull, code, name, &sector, pos, dir, speed, dest);
        auto& ship   = *it;
        // weapons/turrets
        ship.weapons.reserve(weapons.size());
        ship.turrets.reserve(turrets.size());
        bool isSideFire = isShipSideFire(type);
        for (size_t i = 0; i < (isSideFire ? 2 : 1); ++i) {
            for (WeaponType weapon : weapons) {
                ship.weapons.emplace(ship.weapons.end(), weapon_ptr(new Weapon(weapon, false, isSideFire ? i ? -1 : 1 : 0, ship)));
            }
        }
        for (WeaponType turret : turrets) {
            ship.turrets.emplace(ship.turrets.end(), weapon_ptr(new Weapon(turret, true, 0, ship)));
        }
        // friend/foe
        if (isPlayerShip) {
            ship.faction = ShipFaction_Player;
        } else {
            float rnd = randFloat();
            if (rnd < PLAYER_FREQUENCY) {
                ship.faction = ShipFaction_Player;
            } else if (rnd < PLAYER_FREQUENCY + FRIEND_FREQUENCY) {
                ship.faction = ShipFaction_Friend;
            } else if (rnd < PLAYER_FREQUENCY + FRIEND_FREQUENCY + ENEMY_FREQUENCY) {
                ship.faction = ShipFaction_Foe;
            }
        }
        // add ship to sector
        sector.ships.insert(&ship);
    }

    return ships;
}


// ---------------------------------------------------------------------------
// OBJECT ACTIONS
// ---------------------------------------------------------------------------


// delta = seconds
void moveShips(double delta, vector<Ship>& ships, Ship* playerShip, bool useJumpgates) {
    for (auto& ship : ships) {
        if (ship.timeout) {
            ship.timeout = std::max(0.0, ship.timeout - delta);
        }
        if (ship.docked && !ship.timeout) {
            ship.docked = false;
        }
        if (ship.docked || ship.currentHull <= 0 || ship.timeout) continue;

        bool  isPlayerShip = &ship == playerShip;
        auto  sector       = ship.sector;
        auto& pos          = ship.position;
        auto& dir          = ship.direction;
        auto& speed        = ship.speed;
        auto& dest         = ship.destination;

        if (dest && dest->sector == sector) {
            auto destPos  = dest->currentPosition();
            auto newDir   = (destPos - pos).normalized();
            auto newPos   = pos + (newDir * speed * delta);
            auto checkVec = destPos - newPos;
            if (newDir.dot(checkVec) > 0) {
                // Continue toward destination
                pos = newPos;
                dir = newDir;
            } else {
                // Reached the destination
                pos = destPos;

                vector<HasSectorAndPosition*> excludes;

                if (auto jumpgate = dynamic_cast<Jumpgate*>(dest->object)) {
                    sector = jumpgate->target->sector;
                    pos = jumpgate->target->position;
                    excludes.push_back(jumpgate->target);
                } else if (dynamic_cast<Station*>(dest->object)) {
                    excludes.insert(excludes.end(), sector->stations.begin(), sector->stations.end());
                    // reached a station -- dock and repair ship
                    ship.currentHull = ship.maxHull;
                    ship.docked = true;
                    ship.timeout = DOCK_TIME; // dock timer
                }


                float miscChance = isPlayerShip && sector->jumpgates.count() > 1 ? 0.f : MISC_DESTINATION_CHANCE;
                if (dest->object) {
                    dest = randDestination(*sector, useJumpgates, miscChance, &excludes);
                    dir = (dest->position - pos).normalized();
                } else {
                    dest = randDestination(*sector, useJumpgates, miscChance);
                }
            }
        } else {
            // No destination -- just keep flying
            pos = pos + (dir * speed * delta);
        }

        if (useJumpgates) {
            // Jumpgates required between sectors -- bounce off sector walls
            if (pos.x < 0 || pos.x >= sector->size.x || pos.y < 0 || pos.y >= sector->size.y) {
                dest = randDestination(*sector, useJumpgates);
                dir = (dest->position - pos).normalized();
            }
        } else {
            // No jumpgates -- fly directly between sectors
            if (pos.x < 0) {
                if (sector->neighbors.west) {
                    sector = sector->neighbors.west;
                    pos = {pos.x + sector->size.x, pos.y};
                } else {
                    dir = direction_t{-dir.x, randFloat(-1, 1)}.normalized();
                }
            } else if (pos.x >= sector->size.x) {
                if (sector->neighbors.east) {
                    pos = {pos.x - sector->size.x, pos.y};
                    sector = sector->neighbors.east;
                } else {
                    dir = direction_t{-dir.x, randFloat(-1, 1)}.normalized();
                }
            }
            if (pos.y < 0) {
                if (sector->neighbors.north) {
                    sector = sector->neighbors.north;
                    pos = {pos.x, pos.y + sector->size.y};
                } else {
                    dir = direction_t{randFloat(-1, 1), -dir.y}.normalized();
                }
            } else if (pos.y >= sector->size.y) {
                if (sector->neighbors.south) {
                    pos = {pos.x, pos.y - sector->size.y};
                    sector = sector->neighbors.south;
                } else {
                    dir = direction_t{randFloat(-1, 1), -dir.y}.normalized();
                }
            }
        }

        // Handle sector changes
        if (sector != ship.sector) {
            // remove from old sector
            set<Ship*> sectorShips(ship.sector->ships);
            sectorShips.erase(&ship);
            ship.sector->ships = sectorShips;
            // add to new sector
            sectorShips = sector->ships;
            sectorShips.insert(&ship);
            sector->ships = sectorShips;
            // update ship
            ship.sector = sector;
        }

        // Maintain sector boundary
        if      (pos.x < 0)              pos.x = 0;
        else if (pos.x > sector->size.x) pos.x = sector->size.x;
        if      (pos.y < 0)              pos.y = 0;
        else if (pos.y > sector->size.y) pos.y = sector->size.y;
    }
}


void acquireTargets(Sector& sector) {
    map<Ship*, vector<Ship*>> potentialTargets;
    vector<Ship*> sectorShips;
    sectorShips.reserve(sector.ships.size());

    for (Ship* ship : sector.ships) {
        // Clear dead ships' targets
        if (ship->currentHull <= 0.f) {
            if (ship->target)
                ship->target = nullptr;
            for (auto weapon : ship->weaponsAndTurrets()) {
                if (weapon->target) weapon->target = nullptr;
            }
            continue;
        }
        // Exclude neutral ships
        if (!ship->faction) continue;
        sectorShips.push_back(ship);
    }

    // Map targets potentially in range
    for (Ship* ship : sectorShips) {
        if (!ship->weapons.empty() || !ship->turrets.empty()) {
            for (Ship* otherShip : sectorShips) {
                // Exclude friendly ships, docked ships, dead ships, and ones definitely out of range
                if (ship == otherShip
                || otherShip->docked
                || otherShip->currentHull <= 0.f
                || ship->faction == otherShip->faction
                || (ship->faction == ShipFaction_Player && otherShip->faction == ShipFaction_Friend)
                || (ship->faction == ShipFaction_Friend && otherShip->faction == ShipFaction_Player)
                || (otherShip->position - ship->position).magnitude() > MAX_TO_HIT_RANGE
                ) continue;

                if (!potentialTargets.count(ship)) {
                    potentialTargets.emplace(ship, std::move(vector<Ship*>{otherShip}));
                } else {
                    potentialTargets[ship].push_back(otherShip);
                }
            }
        }
        // Untarget all if no potential targets are in range
        if (!potentialTargets.count(ship)) {
            if (ship->target) ship->target = nullptr;
            for (auto weapon : ship->weapons) if (weapon->target) weapon->target = nullptr;
            for (auto turret : ship->turrets) if (turret->target) turret->target = nullptr;
        }
    }

    // Assign targets
    for (auto it = potentialTargets.begin(); it != potentialTargets.end(); ++it) {
        Ship* ship = it->first;
        vector<Ship*>& targets = it->second;
        vector<Ship*> possibleMainTargets;
        map<Weapon*, pair<Ship*, float>> weaponToHit;

        // Determine potential main targets and chance to hit per weapon
        for (Ship* target : targets) {

            // potential main targets
            if (possibleMainTargets.empty() || target->type > possibleMainTargets[0]->type) {
                if (!possibleMainTargets.empty()) possibleMainTargets.clear();
                possibleMainTargets.push_back(target);
            }

            // main weapons
            for (size_t i=0; i < ship->weapons.size(); ++i) {
                Weapon& weapon = *(ship->weapons[i]);
                int weaponPosition = isShipSideFire(ship->type) ? (i < ship->weapons.size()/2) ? -1 : 1 : 0;
                float toHit = chanceToHit(weapon, false, weaponPosition, target);
                if (toHit > 0.f) {
                    if (!weaponToHit.count(&weapon)) {
                        weaponToHit.emplace(&weapon, std::move(pair<Ship*, float>(target, toHit)));
                    } else {
                        if (toHit > weaponToHit[&weapon].second) {
                            weaponToHit[&weapon] = {target, toHit};
                        }
                    }
                }
            }
            // turrets
            for (size_t i=0; i < ship->turrets.size(); ++i) {
                Weapon& turret = *(ship->turrets[i]);
                float toHit = chanceToHit(turret, true, 0, target);
                if (toHit > 0.f) {
                    if (!weaponToHit.count(&turret)) {
                        weaponToHit.emplace(&turret, std::move(pair<Ship*, float>(target, toHit)));
                    } else {
                        if (toHit > weaponToHit[&turret].second) {
                            weaponToHit[&turret] = {target, toHit};
                        }
                    }
                }
            }
        }

        // Assign primary target
        if (ship->type >= ShipType_Scout)  // only military ships have primary targets
        {
            Ship* bestTarget = nullptr;
            distance_t distanceToBestTarget = 0;
            distance_t distanceToTarget;
            for (auto target : possibleMainTargets) {
                if (!bestTarget) {
                    bestTarget = target;
                    distanceToBestTarget = (target->position - ship->position).magnitude();
                } else if (target->currentHull != bestTarget->currentHull) {
                    if (target->currentHull < bestTarget->currentHull) {
                        bestTarget = target;
                        distanceToBestTarget = (target->position - ship->position).magnitude();
                    }
                } else {
                    distanceToTarget = (target->position - ship->position).magnitude();
                    if (distanceToTarget < distanceToBestTarget) {
                        bestTarget = target;
                        distanceToBestTarget = distanceToTarget;
                    }
                }
            }
            if (ship->target != bestTarget) ship->target = bestTarget;
        }

        // Assign weapon targets
        {
            Ship* bestTarget;
            Weapon* p;
            for (auto weapon : ship->weapons) {
                p = &*weapon;
                bestTarget = nullptr;
                if (weaponToHit.count(p)) bestTarget = weaponToHit[p].first;
                if (p->target != bestTarget) p->target = bestTarget;
            }
            for (auto turret : ship->turrets) {
                p = &*turret;
                bestTarget = nullptr;
                if (weaponToHit.count(p)) bestTarget = weaponToHit[p].first;
                if (p->target != bestTarget) p->target = bestTarget;
            }
        }
    }
}


void acquireTargets(vector<vector<Sector>>& sectors) {
    for (auto& sectorRow : sectors) {
        for (auto& sector : sectorRow) {
            acquireTargets(sector);
        }
    }
}


void fireWeapons(double delta, vector<Ship>& ships) {
    map<Weapon*, float> weaponCooldowns; // weapon and interative cooldown
    vector<pair<Weapon*, float>> shots;  // weapon and snapshot cooldown 

    // queue shots
    {
        float minNextCooldown = 0.f;
        bool minNextCooldownSet = false;
        do {
            vector<Weapon*> weaponsAndTurrets;
            for (auto& ship : ships) {
                weaponsAndTurrets = ship.weaponsAndTurrets();
                for (auto weaponsIt = weaponsAndTurrets.begin(); weaponsIt != weaponsAndTurrets.end(); ++weaponsIt) {
                    auto weapon = &**weaponsIt;
                    if (weapon->target) {
                        if (weapon->target->sector != weapon->parent->sector) return;
                        if (!weaponCooldowns.count(weapon)) weaponCooldowns[weapon] = weapon->cooldown;
                        auto& currentCooldown = weaponCooldowns[weapon];
                        if (currentCooldown <= delta) {
                            shots.push_back({weapon, currentCooldown});
                            currentCooldown += weaponCooldown(weapon->type);
                            if (!minNextCooldownSet || currentCooldown < minNextCooldown) {
                                minNextCooldown = currentCooldown;
                                minNextCooldownSet = true;
                            }
                        }
                    }
                }
                minNextCooldownSet = false;
            }
        }  while (minNextCooldown < delta && minNextCooldown > 0.f);
    }

    // sort shot order
    sort(shots.begin(), shots.end(), [](pair<Weapon*, float> a, pair<Weapon*, float> b) -> bool { return a.second < b.second; });

    // apply shots
    if (shots.size())
    {
        float currentCooldown = 0.f;
        float appliedDelta    = 0.f;
        bool canFire          = true;
        for (size_t i=0; i<shots.size(); ++i) {
            do {
                auto& shot = shots[i];
                auto  weapon = shot.first;
                auto& cooldown = shot.second;
                if (cooldown != currentCooldown) {
                    appliedDelta = currentCooldown;
                    currentCooldown = shot.second;
                    continue;
                }
                if (auto target = dynamic_cast<Ship*>(weapon->target)) {
                    canFire = !target->docked && target->currentHull >= 0;
                    if (canFire) {
                        if (Ship* ship = dynamic_cast<Ship*>(weapon->parent)) {
                            if (ship->currentHull <= 0.f) {     // ship is dead
                                if (cooldown > appliedDelta) {  // live fire rounds are expended
                                    canFire = false;
                                }
                            }
                        }
                        if (canFire) {
                            float toHit = chanceToHit(*weapon, weapon->isTurret, weapon->weaponPosition);
                            if (toHit <= 0.f) canFire = false;
                            if (canFire) {
                                float tryFire = randFloat();
                                if (tryFire <= toHit) {
                                    auto damage = weaponDamage(weapon->type, weapon->isTurret);
                                    if (isWeaponDamageOverTime(weapon->type)) damage *= cooldown - appliedDelta;  // adjust beam weapon damage
                                    if (auto target = dynamic_cast<Ship*>(weapon->target)) {
                                        target->currentHull = std::max(0.f, target->currentHull - damage);
                                        if (target->currentHull <= 0) target->timeout = RESPAWN_TIME; // respawn timer
                                    }
                                }
                            }
                        }
                    }
                }
                ++i;
            } while (currentCooldown >= appliedDelta && i < shots.size());
        }
    }

    // Update live weapon cooldowns
    for (auto& ship : ships) {
        for (auto weapon : ship.weapons) if (weapon->cooldown > 0.f) weapon->cooldown -= delta;
        for (auto turret : ship.turrets) if (turret->cooldown > 0.f) turret->cooldown -= delta;
    }
}


void respawnShips(vector<Ship>& ships, Ship* playerShip, vector<Station>& stations, bool useJumpgates) {
    if (stations.empty()) return; // return if there are no valid respawn points
    for (auto& ship : ships) {
        bool isPlayerShip = &ship == playerShip;
        if (ship.currentHull <= 0 && ship.timeout <= 0.f) {
            // don't respawn ships that are dead in the player's sector
            if (playerShip && !isPlayerShip && ship.sector == playerShip->sector) {
                continue;
            }

            // remove dead ship from the sector
            set<Ship*> sectorShips(ship.sector->ships);
            sectorShips.erase(&ship);
            ship.sector->ships = sectorShips;

            // select a random station for respawn
            Station& station = stations[rand() % stations.size()];

            // redefine the ship from scratch
            Sector& sector = *station.sector;
            auto type    = randShipType();
            auto hull    = shipHull(type);
            auto code    = randCode();
            auto name    = randName(type);
            auto pos     = station.position;
            auto speed   = shipSpeed(type);
            auto weapons = shipWeapons(type);
            auto turrets = shipTurrets(type);

            // determine a travel destination for after undocking
            vector<HasSectorAndPosition*> excludes{&station};
            float miscChance = isPlayerShip && sector.jumpgates.count() > 1 ? 0.f : MISC_DESTINATION_CHANCE;
            auto dest = randDestination(sector, useJumpgates, miscChance, &excludes);
            auto dir = (dest->position - pos).normalized();

            // replace dead ship, docked at selected station
            ship = Ship(type, hull, code, name, &sector, pos, dir, speed, dest);
            ship.docked = true;
            ship.timeout = 0.f;

            // weapons/turrets
            ship.weapons.reserve(weapons.size());
            ship.turrets.reserve(turrets.size());
            bool isSideFire = isShipSideFire(type);
            for (size_t i = 0; i < (isSideFire ? 2 : 1); ++i) {
                for (WeaponType weapon : weapons) {
                    ship.weapons.emplace(ship.weapons.end(), weapon_ptr(new Weapon(weapon, false, isSideFire ? i ? -1 : 1 : 0, ship)));
                }
            }
            for (WeaponType turret : turrets) {
                ship.turrets.emplace(ship.turrets.end(), weapon_ptr(new Weapon(turret, true, 0, ship)));
            }

            // friend/foe
            if (isPlayerShip) {
                ship.faction = ShipFaction_Player;
            } else {
                // Neutral ships aren't presently part of the combat system, so
                // there's no need to respawn them -- so choose a combat-capable
                // faction
                float rnd = randFloat(PLAYER_FREQUENCY + FRIEND_FREQUENCY + ENEMY_FREQUENCY);
                if (rnd < PLAYER_FREQUENCY) {
                    ship.faction = ShipFaction_Player;
                } else if (rnd < PLAYER_FREQUENCY + FRIEND_FREQUENCY) {
                    ship.faction = ShipFaction_Friend;
                } else if (rnd < PLAYER_FREQUENCY + FRIEND_FREQUENCY + ENEMY_FREQUENCY) {
                    ship.faction = ShipFaction_Foe;
                }
            }

            // add to sector
            sectorShips = sector.ships;
            sectorShips.insert(&ship);
            sector.ships = sectorShips;
        }
    }
}


// ---------------------------------------------------------------------------
// PROGRAM
// ---------------------------------------------------------------------------


int main(int argc, char** argv) {
    std::srand(time(nullptr));

    bool useColor     = false;
    bool useJumpgates = true;

    for (size_t i=0; i<argc; ++i) {
        if (strcmp(argv[i], "--color") == 0)        useColor     = true;
        if (strcmp(argv[i], "--no-jumpgates") == 0) useJumpgates = false;
    }

    auto sectors   = initSectors(SECTOR_BOUNDS, SECTOR_SIZE);
    auto jumpgates = initJumpgates(sectors, useJumpgates);
    auto stations  = initStations(sectors);
    auto ships     = initShips(SHIP_COUNT, sectors, useJumpgates);

    auto& playerShip = ships[0];

    auto mainThreadFn = [&]() {
        duration<double>         d1, d2, delta;
        time_point<steady_clock> t, thisTick, nextTick = steady_clock::now(), lastTick = nextTick;
        while (true) {
            std::this_thread::sleep_until(nextTick);

            thisTick   = steady_clock::now();
            delta      = thisTick - lastTick; // seconds
            nextTick   = thisTick + milliseconds(TICK_TIME);

            t = steady_clock::now();
            respawnShips(ships, &playerShip, stations, useJumpgates);
            moveShips(delta.count(), ships, &playerShip, useJumpgates);
            acquireTargets(sectors);
            fireWeapons(delta.count(), ships);
            d1 = steady_clock::now() - t;
            
            t = steady_clock::now();
            updateDisplay(sectors, playerShip, useColor);
            d2 = steady_clock::now() - t;
            
            cout << "delta: " << (delta.count() * 1000) << "ms" << "  "
                 << "work: " << (d1.count() * 1000) << "ms" << "  "
                 << "display: " << (d2.count() * 1000) << "ms"
                 << endl;

            lastTick = thisTick;
        }
    };

    thread mainThread = thread(mainThreadFn);
    mainThread.join();

    return 0;
}
