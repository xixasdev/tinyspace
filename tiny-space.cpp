// tiny-space.cpp (v3 - experiment: snapshot serialization) (C++11)
// AUTHOR: xixas | DATE: 2022.02.06 | LICENSE: WTFPL/PDM/CC0... your choice
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
// EXPERIMENTS:
//           2022.02.01 | AUTHOR: xixas | Added basic XML serialization
//           2022.02.06 | AUTHOR: xixas | Added background serialization


#include <algorithm>
#include <atomic>
#include <chrono>
#include <cmath>
#include <cstdio>
#include <cstring>
#include <cstdlib>
#include <ctime>
#include <fstream>
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


using std::atomic;
using std::chrono::duration;
using std::chrono::milliseconds;
using std::chrono::steady_clock;
using std::chrono::time_point;
using std::cerr;
using std::cout;
using std::endl;
using std::map;
using std::make_shared;
using std::ofstream;
using std::ostream;
using std::ostringstream;
using std::pair;
using std::set;
using std::shared_ptr;
using std::string;
using std::thread;
using std::unordered_set;
using std::vector;


// ---------------------------------------------------------------------------
// SAVE STATE SYNC ATOMICS
// ---------------------------------------------------------------------------


std::atomic<bool> willSave(false);
std::atomic<bool> isSaving(false);
std::atomic<bool> saveComplete(false);
std::atomic<bool> endGame(false);

atomic<thread*> saveThread(nullptr);
inline bool isSavingThread() {
    if (!isSaving) return false;
    thread* saveThreadPtr = saveThread.load();
    if (!saveThreadPtr) return false;
    return saveThreadPtr->get_id() == std::this_thread::get_id();
}


// ---------------------------------------------------------------------------
// UPDATEABLE OBJECT REGISTRATION
// ---------------------------------------------------------------------------


struct Updateable;
unordered_set<Updateable*> updateables_aftersave;
struct Updateable {
    Updateable() { updateables_aftersave.insert(this); }
    virtual ~Updateable() { updateables_aftersave.erase(this); };
    virtual void update() = 0;
};
void update_aftersave() { for (auto u : updateables_aftersave) u->update(); }


// ---------------------------------------------------------------------------
// GENERIC SNAPSHOT TEMPLATE CLASS AND TYPE ERASURE
// ---------------------------------------------------------------------------


template <typename T>
class Saveable : public Updateable {
    T *_live, *_snap;

public:
    Saveable() : Updateable() { _snap = _live = new T; }
    Saveable(T const& t) : Updateable() { _snap = _live = new T(t); }
    Saveable(T&& t) : Updateable() { _snap = _live = new T(t); }
    Saveable(Saveable<T> const& o) : Updateable() { _snap = _live = new T(*o._live); }

    ~Saveable() { if (_snap != _live) delete _snap; delete _live; }

    void set(T const& t) { if (isSaving && _live == _snap) _live = new T(t); else *_live = t;}
    void set(T&& t) { if (isSaving && _live == _snap) _live = new T(t); else *_live = t;}
    void update() { if (_snap != _live) { delete _snap; _snap = _live; } }

    T const& live() { return *_live; }
    T const& snap() { return *_snap; }

    Saveable<T>& operator =(T const& t) { set(t); return *this; }
    Saveable<T>& operator =(T&& t) { set(t); return *this; }
    Saveable<T>& operator =(Saveable<T> const& o) { set(*o._live); return *this; }

    Saveable<T>& operator *=(T const& t) { set(*_live * t); return *this; }
    Saveable<T>& operator /=(T const& t) { set(*_live / t); return *this; }
    Saveable<T>& operator +=(T const& t) { set(*_live + t); return *this; }
    Saveable<T>& operator -=(T const& t) { set(*_live - t); return *this; }

    operator T const&() const { return isSavingThread() ? *_snap : *_live; }
    T const& operator ()() const { return isSavingThread() ? *_snap : *_live; }
    T const* operator ->() const { return isSavingThread() ? _snap : _live; }
};
template <typename T> inline ostream& operator <<(ostream& os, Saveable<T> const& o) { os << o(); return os; }


template <typename T>
class Saveable<T*> : public Updateable {
    T *_live, *_snap;

public:
    Saveable() : Updateable() { _snap = _live = nullptr; }
    Saveable(T* const& t) : Updateable() { _snap = _live = t; }
    Saveable(Saveable<T*> const& o) : Updateable() { _snap = _live = o._live; }

    ~Saveable() {}

    void set(T* const& t) { if (!isSaving) _snap = t; _live = t; }
    void update() { _snap = _live; }

    T* const& live() { return _live; }
    T* const& snap() { return _snap; }

    Saveable<T*>& operator =(T* const& t) { set(t) ; return *this; }
    Saveable<T*>& operator =(Saveable<T*> const& o) { set(o._live); return *this; }

    operator T* const&() const { return isSavingThread() ? _snap : _live; }
    T* const& operator ()() const { return isSavingThread() ? _snap : _live; }
    T* const operator ->() const { return isSavingThread() ? _snap : _live; }
};


template <typename T>
class Saveable<shared_ptr<T>> : public Updateable {
    shared_ptr<T> _live, _snap;

public:
    Saveable() : Updateable() { _snap = _live = nullptr; }
    Saveable(shared_ptr<T> const& t) : Updateable() { _snap = _live = t; }
    Saveable(Saveable<shared_ptr<T>> const& o) : Updateable() { _snap = _live = o._live; }

    ~Saveable() {}

    void set(shared_ptr<T> const& t) { if (!isSaving) _snap = t; _live = t; }
    void update() { _snap = _live; }

    shared_ptr<T> const& live() { return _live; }
    shared_ptr<T> const& snap() { return _snap; }

    Saveable<shared_ptr<T>>& operator =(shared_ptr<T> const& t) { set(t); return *this; }
    Saveable<shared_ptr<T>>& operator =(Saveable<shared_ptr<T>> const& o) { set(o._live); return *this; }

    operator bool() const { return isSavingThread() ? _snap.get() : _live.get(); }
    operator shared_ptr<T> const&() const { return isSavingThread() ? _snap : _live; }
    shared_ptr<T> const& operator ()() const { return isSavingThread() ? _snap : _live; }
    T const* operator ->() const { return isSavingThread() ? _snap.get() : _live.get(); }
};


template <typename T, typename U=float> struct Vector2;
template <typename T, typename U=float> struct SaveableVector2 : public Vector2<Saveable<T>, U> {
    SaveableVector2() : Vector2<Saveable<T>, U>() {}
    SaveableVector2(T const& x, T const& y) : Vector2<Saveable<T>, U>(x, y) {}
    SaveableVector2(Vector2<T,U> const& o) : Vector2<Saveable<T>, U>(o.x, o.y) {}
    SaveableVector2(Vector2<Saveable<T>,U> const& o) : Vector2<Saveable<T>, U>(o.x, o.y) {}
    ~SaveableVector2() {}

    operator Vector2<T,U>() const { return (*this)(); }
    Vector2<T,U> operator ()() const { return Vector2<T,U>{this->x, this->y}; }
};
template <typename T, typename U> inline Vector2<T,U> operator +(Vector2<Saveable<T>,U> const& lhs, Vector2<Saveable<T>,U> const& rhs) { return {lhs.x + rhs.x, lhs.y + rhs.y}; }
template <typename T, typename U> inline Vector2<T,U> operator +(Vector2<Saveable<T>,U> const& lhs, Vector2<T,U> const& rhs) { return {lhs.x + rhs.x, lhs.y + rhs.y}; }
template <typename T, typename U> inline Vector2<T,U> operator +(Vector2<T,U> const& lhs, Vector2<Saveable<T>,U> const& rhs) { return {lhs.x + rhs.x, lhs.y + rhs.y}; }
template <typename T, typename U> inline Vector2<T,U> operator -(Vector2<Saveable<T>,U> const& lhs, Vector2<Saveable<T>,U> const& rhs) { return {lhs.x - rhs.x, lhs.y - rhs.y}; }
template <typename T, typename U> inline Vector2<T,U> operator -(Vector2<Saveable<T>,U> const& lhs, Vector2<T,U> const& rhs) { return {lhs.x - rhs.x, lhs.y - rhs.y}; }
template <typename T, typename U> inline Vector2<T,U> operator -(Vector2<T,U> const& lhs, Vector2<Saveable<T>,U> const& rhs) { return {lhs.x - rhs.x, lhs.y - rhs.y}; }

template <typename T, typename U, typename V> inline Vector2<T,U> operator *(Vector2<Saveable<T>,U> const& lhs, Saveable<V> const& rhs) { return Vector2<T,U>{lhs.x, lhs.y} * rhs(); }
template <typename T, typename U, typename V> inline Vector2<T,U> operator *(Vector2<Saveable<T>,U> const& lhs, V const& rhs) { return Vector2<T,U>{lhs.x, lhs.y} * rhs; }
template <typename T, typename U, typename V> inline Vector2<T,U> operator *(Vector2<T,U> const& lhs, Saveable<V> const& rhs) { return lhs * rhs(); }
template <typename T, typename U, typename V> inline Vector2<T,U> operator /(Vector2<Saveable<T>,U> const& lhs, Saveable<V> const& rhs) { return Vector2<T,U>{lhs.x, lhs.y} / rhs(); }
template <typename T, typename U, typename V> inline Vector2<T,U> operator /(Vector2<Saveable<T>,U> const& lhs, V const& rhs) { return Vector2<T,U>{lhs.x, lhs.y} / rhs; }
template <typename T, typename U, typename V> inline Vector2<T,U> operator /(Vector2<T,U> const& lhs, Saveable<V> const& rhs) { return lhs / rhs(); }
template <typename T, typename U, typename V> inline Vector2<T,U> operator +(Vector2<Saveable<T>,U> const& lhs, Saveable<V> const& rhs) { return Vector2<T,U>{lhs.x, lhs.y} + rhs(); }
template <typename T, typename U, typename V> inline Vector2<T,U> operator +(Vector2<Saveable<T>,U> const& lhs, V const& rhs) { return Vector2<T,U>{lhs.x, lhs.y} + rhs; }
template <typename T, typename U, typename V> inline Vector2<T,U> operator +(Vector2<T,U> const& lhs, Saveable<V> const& rhs) { return lhs + rhs(); }
template <typename T, typename U, typename V> inline Vector2<T,U> operator -(Vector2<Saveable<T>,U> const& lhs, Saveable<V> const& rhs) { return Vector2<T,U>{lhs.x, lhs.y} - rhs(); }
template <typename T, typename U, typename V> inline Vector2<T,U> operator -(Vector2<Saveable<T>,U> const& lhs, V const& rhs) { return Vector2<T,U>{lhs.x, lhs.y} - rhs; }
template <typename T, typename U, typename V> inline Vector2<T,U> operator -(Vector2<T,U> const& lhs, Saveable<V> const& rhs) { return lhs.x - rhs(); }


// ---------------------------------------------------------------------------
// TYPES
// ---------------------------------------------------------------------------


// Forward declarations
enum IDType : unsigned int;
enum WeaponType : unsigned int;
enum WeaponPosition : int;
enum ShipType : unsigned int;
enum ShipFaction : unsigned int;
struct Ship;
struct Sector;
struct Destination;
struct Weapon;
struct HasIDAndSectorAndPosition;


// Standard types
typedef size_t          ID;
typedef Vector2<int>    v2int_t;
typedef Vector2<size_t> v2size_t;
typedef Vector2<float>  v2float_t;
typedef v2float_t       dimensions_t;
typedef v2float_t       position_t;
typedef v2float_t       direction_t;
typedef float           speed_t;
typedef float           distance_t;

typedef set<Ship*>      set_ship_ptr;
typedef shared_ptr<Destination> destination_ptr;
typedef shared_ptr<Weapon> weapon_ptr;
typedef vector<weapon_ptr> vector_weapon_ptr;
typedef HasIDAndSectorAndPosition* target_t;


// Saveable types
typedef Saveable<bool>          bool_s;
typedef Saveable<float>         float_s;
typedef Saveable<double>        double_s;
typedef Saveable<int>           int_s;
typedef Saveable<unsigned int>  unsigned_int_s;
typedef Saveable<size_t>        size_s;
typedef Saveable<string>        string_s;

typedef Saveable<ID>             ID_s;
typedef Saveable<IDType>         IDType_s;
typedef Saveable<WeaponType>     WeaponType_s;
typedef Saveable<WeaponPosition> WeaponPosition_s;
typedef Saveable<ShipType>       ShipType_s;
typedef Saveable<ShipFaction>    ShipFaction_s;

typedef SaveableVector2<float> v2float_s;
typedef v2float_s              dimensions_s;
typedef v2float_s              position_s;
typedef v2float_s              direction_s;
typedef float_s                speed_s;
typedef float_s                distance_s;

typedef Saveable<set_ship_ptr>      set_ship_ptr_s;
typedef Saveable<Sector*>           sector_ptr_s;
typedef Saveable<destination_ptr>   destination_ptr_s;
typedef Saveable<weapon_ptr>        weapon_ptr_s;
typedef Saveable<vector_weapon_ptr> vector_weapon_ptr_s;
typedef Saveable<target_t>          target_s;


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
template <typename T, typename U> inline std::ostream& operator <<(std::ostream& os, const Vector2<T,U>& o) { os << '{' << o.x << ',' << o.y << '}'; return os; }


struct XmlSerializable {
    virtual ~XmlSerializable() {}

    virtual string xml_tagname() = 0;// { return "NOT_IMPLEMENTED"; }
    virtual string xml_serialize(string const& indent="") = 0;// { return "<!-- serialize() method not implemented! -->"; }
};


enum IDType : unsigned int { IDType_NONE, IDType_Sector, IDType_Jumpgate, IDType_Station, IDType_Ship, IDType_Weapon, IDType_END };
struct HasID {
    ID_s id;
    IDType_s idType;

    HasID(IDType const& idType) : id(++curId), idType(idType) {}
    HasID(ID const& id, IDType const& idType) : id(id), idType(idType) {}

    void resetId() { id = ++curId; }
private:
    static ID curId;
};
ID HasID::curId = 0;


struct HasCode {
    string_s code;

    HasCode(string const& code="") : code(code) {}
};


struct HasName {
    string_s name;

    HasName(string const& name="") : name(name) {}
};


struct HasSize {
    dimensions_s size;

    HasSize(dimensions_t const& size={0, 0}) : size(size) {}
};


struct HasPosition {
    position_s position;

    HasPosition(position_t const& position={0, 0}) : position(position) {}
};


struct HasDirection {
    direction_s direction;

    HasDirection(direction_t const& direction={0, 0}) : direction(direction) {}
};


struct HasSpeed {
    speed_s speed;

    HasSpeed(speed_t const& speed=0) : speed(speed) {}
};


struct Sector;
struct HasSector {
    sector_ptr_s sector;

    HasSector(Sector* const sector=nullptr) : sector(sector) {}
};


struct HasSectorAndPosition : public HasSector, public HasPosition {

    HasSectorAndPosition(Sector* const sector, position_t const& position)
        : HasSector(sector), HasPosition(position) {}

    virtual ~HasSectorAndPosition() {}
};


struct HasIDAndSectorAndPosition : public HasID, public HasSectorAndPosition {

    HasIDAndSectorAndPosition(IDType const& idType, Sector* const sector, position_t const& position)
        : HasID(idType), HasSectorAndPosition(sector, position) {}

    HasIDAndSectorAndPosition(ID const& id, IDType const& idType, Sector* const sector, position_t const& position)
        : HasID(id, idType), HasSectorAndPosition(sector, position) {}

    virtual ~HasIDAndSectorAndPosition() {}
};


struct HasDestination {
    destination_ptr_s destination;

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

    vector<Jumpgate*> all(bool includeNull=false) const {
        vector<Jumpgate*> jumpgates;
        if (north || includeNull) jumpgates.push_back(north);
        if (east  || includeNull) jumpgates.push_back(east);
        if (south || includeNull) jumpgates.push_back(south);
        if (west  || includeNull) jumpgates.push_back(west);
        return jumpgates;
    };
};


struct Ship;
struct Station;
struct Sector: public HasID, public HasName, public HasSize, public XmlSerializable {
private:
    set_ship_ptr_s _ships;

public:
    pair<size_t, size_t> rowcol; // row and column in the universe (sectors)
    SectorNeighbors neighbors;
    SectorJumpgates jumpgates;
    set<Station*>   stations;
    set_ship_ptr_s const& ships = _ships;

    Sector(pair<size_t, size_t> rowcol, string const& name="", dimensions_t const& size={0, 0})
        : HasID(IDType_Sector), HasName(name), HasSize(size), rowcol(rowcol), neighbors(), _ships() {}

    Sector(ID const& id, pair<size_t, size_t> rowcol, string const& name="", dimensions_t const& size={0, 0})
        : HasID(id, IDType_Sector), HasName(name), HasSize(size), rowcol(rowcol), neighbors(), _ships() {}

    void setShips(set_ship_ptr&& ships) { _ships = ships; }

    string xml_tagname() override;
    string xml_serialize(string const& indent="") override;
};


struct Jumpgate : public HasIDAndSectorAndPosition, XmlSerializable {
    Jumpgate* target;

    Jumpgate(Sector* const sector=nullptr, position_t const& position={0,0}, Jumpgate* const target = nullptr)
        : HasIDAndSectorAndPosition(IDType_Jumpgate, sector, position), target(target) {};

    Jumpgate(ID const& id, Sector* const sector=nullptr, position_t const& position={0,0}, Jumpgate* const target = nullptr)
        : HasIDAndSectorAndPosition(id, IDType_Jumpgate, sector, position), target(target) {};

    string xml_tagname() override;
    string xml_serialize(string const& indent="") override;
};


struct Station : public HasIDAndSectorAndPosition, XmlSerializable {
    Station(Sector* const sector=nullptr, position_t const& position={0,0})
        : HasIDAndSectorAndPosition(IDType_Station, sector, position) {};

    Station(ID const& id, Sector* const sector=nullptr, position_t const& position={0,0})
        : HasIDAndSectorAndPosition(id, IDType_Station, sector, position) {};

    string xml_tagname() override;
    string xml_serialize(string const& indent="") override;
};


struct Destination : public HasSectorAndPosition {
    HasIDAndSectorAndPosition* object;  // sector and position are usually ignored if object is set

    Destination(HasIDAndSectorAndPosition& object)
        : HasSectorAndPosition(object.sector, object.position), object(&object) {}

    Destination(Sector& sector, position_t const& position)
        : HasSectorAndPosition(&sector, position), object(nullptr) {}

    Sector* currentSector() const { return object ? object->sector : sector; }
    position_t currentPosition() const { return object ? object->position : position; }
};


enum WeaponType : unsigned int { WeaponType_NONE, WeaponType_Pulse, WeaponType_Cannon, WeaponType_Beam, WeaponType_END };
enum WeaponPosition : int { WeaponPosition_Bow=0, WeaponPosition_Port=-1, WeaponPosition_Starboard=1, WeaponPosition_END };
struct Weapon : public HasID, XmlSerializable {
    WeaponType type;
    bool isTurret;
    target_t parent;
    target_s target;
    // weaponPosition designates forward mount (0), left (-1), or right (1) -- doesn't apply to turrets
    // these values can be considered 90 degree directional multipliers
    WeaponPosition weaponPosition;
    float_s cooldown;
    
    Weapon(WeaponType type, bool isTurret, WeaponPosition weaponPosition, HasIDAndSectorAndPosition& parent)
        :
        HasID(IDType_Weapon),
        type(type), isTurret(isTurret), weaponPosition(weaponPosition), parent(&parent),
        target(nullptr), cooldown(0) {};

    Weapon(ID const& id, WeaponType type, bool isTurret, WeaponPosition weaponPosition, HasIDAndSectorAndPosition& parent, HasIDAndSectorAndPosition* const target, float cooldown)
        :
        HasID(id, IDType_Weapon),
        type(type), isTurret(isTurret), weaponPosition(weaponPosition), parent(&parent),
        target(target), cooldown(cooldown) {};

    string xml_tagname() override;
    string xml_serialize(string const& indent="") override;
};


// ship type in order of priority of target importance, least to greatest, all civilian ships first
enum ShipType : unsigned int { ShipType_NONE, ShipType_Courier, ShipType_Transport, ShipType_Scout, ShipType_Corvette, ShipType_Frigate, ShipType_END };
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
enum ShipFaction : unsigned int { ShipFaction_Neutral, ShipFaction_Player, ShipFaction_Friend, ShipFaction_Foe, ShipFaction_END };
struct Ship : public HasIDAndSectorAndPosition,
              public HasCode, public HasName,
              public HasDirection, public HasSpeed,
              public HasDestination,
              public XmlSerializable
{
private:
    vector_weapon_ptr_s _weapons;
    vector_weapon_ptr_s _turrets;

public:
    ShipType_s                 type;
    ShipFaction_s              faction;
    unsigned_int_s             maxHull, currentHull;
    vector_weapon_ptr_s const& weapons = _weapons;
    vector_weapon_ptr_s const& turrets = _turrets;
    target_s                   target;
    bool_s                     docked;
    double_s                   timeout; // used any time the ship needs a delay (docked, dead, etc)

    Ship(Ship&& o)
        :
        HasIDAndSectorAndPosition(o.id, o.idType, o.sector, o.position),
        HasName(o.name), HasCode(o.code),
        HasDirection(o.direction), HasSpeed(o.speed),
        HasDestination(o.destination),
        type(o.type), maxHull(o.maxHull), currentHull(o.currentHull),
        faction(o.faction),
        _weapons(std::move(o._weapons)), _turrets(std::move(o._turrets)),
        target(o.target), docked(o.docked), timeout(o.timeout) {}

    Ship(ShipType type, const unsigned int hull,
        string const& code="", string const& name="",
        Sector* const sector=nullptr, position_t const& position={0, 0},
        direction_t const& direction={0, 0}, speed_t const& speed=0,
        destination_ptr destination=nullptr)
        :
        HasIDAndSectorAndPosition(IDType_Ship, sector, position),
        HasName(name), HasCode(code),
        HasDirection(direction), HasSpeed(speed),
        HasDestination(destination),
        type(type), maxHull(hull), currentHull(hull),
        faction(ShipFaction_Neutral), _weapons(), _turrets(), target(nullptr), docked(false), timeout(0.f) {}

    Ship(ID const& id, ShipType type, ShipFaction faction, const unsigned int maxHull, const unsigned int currentHull,
        string const& code, string const& name,
        Sector* const sector, position_t const& position,
        direction_t const& direction, speed_t const& speed,
        destination_ptr destination,
        HasIDAndSectorAndPosition* target, bool docked, float timeout)
        :
        HasIDAndSectorAndPosition(id, IDType_Ship, sector, position),
        HasName(name), HasCode(code),
        HasDirection(direction), HasSpeed(speed),
        HasDestination(destination),
        type(type), maxHull(maxHull), currentHull(currentHull),
        faction(faction), _weapons(), _turrets(), target(target), docked(docked), timeout(timeout) {}

    void setWeapons(vector_weapon_ptr&& weapons) {
        for (auto& weapon : weapons) weapon->isTurret = false;
        _weapons = weapons;
    }
    void setTurrets(vector_weapon_ptr&& turrets) {
        for (auto& turret : turrets) turret->isTurret = true;
        _turrets = turrets;
    }

    Ship& operator =(Ship&& o) {
        id = o.id;
        idType = o.idType;
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

        vector_weapon_ptr weapons;
        vector_weapon_ptr turrets;
        weapons.reserve(o._weapons->size());
        turrets.reserve(o._turrets->size());
        for (auto& weapon : o._weapons()) weapons.push_back(weapon);
        for (auto& turret : o._turrets()) turrets.push_back(turret);
        setWeapons(std::move(weapons));
        setTurrets(std::move(turrets));

        target = o.target;
        docked = o.docked;
        timeout = o.timeout;

        return *this;
    }

    vector_weapon_ptr weaponsAndTurrets() {
        vector_weapon_ptr r;
        r.reserve(weapons->size() + turrets->size());
        for (auto& weapon : _weapons()) r.push_back(weapon);
        for (auto& turret : _turrets()) r.push_back(turret);
        return r;
    }

    string xml_tagname() override;
    string xml_serialize(string const& indent="") override;
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
inline position_t randPosition(Vector2<position_t> minMax) {
    return randPosition(minMax.x, minMax.y);
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
        vector<HasIDAndSectorAndPosition*> potentialDestinations;
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
            auto r = destination_ptr(new Destination(*destinationObject));
            return r;
        }
    }
    return destination_ptr(new Destination(sector, randPosition({0,0}, sector.size)));
}


// ---------------------------------------------------------------------------
// BOUNDARY / RANGE DEFINITIONS
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

const Vector2<position_t>
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
float chanceToHit(Weapon const& weapon, bool isTurret, WeaponPosition weaponPosition=WeaponPosition_Bow, HasSectorAndPosition* potentialTarget=nullptr) {
    HasSectorAndPosition* parent = weapon.parent;
    HasSectorAndPosition* target = potentialTarget ? potentialTarget : weapon.target;
    if (!target || !parent || parent->sector != target->sector) return 0.f;
    direction_t targetVector = target->position - parent->position;
    if (targetVector.magnitude() > MAX_TO_HIT_RANGE) return 0.f;
    if (!isTurret) {
        if (Ship* ship = dynamic_cast<Ship*>(parent)) {
            direction_t const& dir = ship->direction;
            direction_t aim = weaponPosition == WeaponPosition_Port      ? dir.port()
                            : weaponPosition == WeaponPosition_Starboard ? dir.starboard()
                            :                                              dir;
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
    if (ship.code->size()) os << /*" code:"*/ " " << ship.code;
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
        if (auto target = dynamic_cast<Ship*>(ship.target())) {
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
    for (auto ship : sector.ships()) {
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
    vector<string> sectorMap; sectorMap.reserve(static_cast<size_t>(sector.size.x+3));
    { // header
        ostringstream os;
        os << leftPadding << '+' << string((sector.size.x+1) * 3, '-') << '+';
        sectorMap.push_back(os.str());
    }
    for (size_t i=0; i<=sector.size.y; ++i) {
        ostringstream os;
        os << leftPadding << '|' << string((sector.size.x+1) * 3, ' ') << '|';
        sectorMap.push_back(os.str());
    }
    { // footer
        ostringstream os;
        os << leftPadding << "+-[ "
           << colorString(PLAYER_COLOR, sector.name, useColor)
           << " ]" << string(((sector.size.x+1) * 3) - sector.name->size() - 5, '-')
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

    // Display saving message
    if (isSaving) {
        string saveString("[ SAVE IN PROGRESS... BUT GAME'S STILL RUNNING! ]");
        size_t col = sectorMap[0].size()/2 - saveString.size()/2 + 3;
        addReplacementString({0, col}, 0, saveString);
    }

    // ships
    for (auto ship : sector.ships()) {
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
                auto dirMax = std::max(dir.y(), 0.0f);
                shipStr = "v";
                if (dir.x > 0 && dir.x > dirMax) { dirMax = dir.x; shipStr = ">"; }
                if (dir.y < 0 && std::abs(dir.y) > dirMax) { dirMax = std::abs(dir.y); shipStr = "^"; }
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
            size_t shipCount = sector.ships->size();
            bool hasPlayerProperty = false;
            for (auto ship : sector.ships()) {
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
                sectors[row].emplace_back(pair<size_t, size_t>{row, col}, name, size);
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
    map<Sector*, set_ship_ptr> sectorShipRefs;

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
        vector_weapon_ptr newWeapons;
        vector_weapon_ptr newTurrets;
        newWeapons.reserve(weapons.size());
        newTurrets.reserve(turrets.size());
        bool isSideFire = isShipSideFire(type);
        for (size_t i = 0; i < (isSideFire ? 2 : 1); ++i) {
            for (WeaponType weapon : weapons) {
                WeaponPosition weaponPosition = isSideFire
                                              ? i ? WeaponPosition_Port
                                                  : WeaponPosition_Starboard
                                              : WeaponPosition_Bow;
                newWeapons.emplace(newWeapons.end(), weapon_ptr(new Weapon(weapon, false, weaponPosition, ship)));
            }
        }
        for (WeaponType turret : turrets) {
            newTurrets.emplace(newTurrets.end(), weapon_ptr(new Weapon(turret, true, WeaponPosition_Bow, ship)));
        }
        ship.setWeapons(std::move(newWeapons));
        ship.setTurrets(std::move(newTurrets));
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
        if (!sectorShipRefs.count(&sector)) {
            sectorShipRefs[&sector] = set_ship_ptr();
        }
        sectorShipRefs[&sector].insert(&ship);
    }

    // Store sector ships
    for (auto& sectorShipRef : sectorShipRefs) {
        sectorShipRef.first->setShips(std::move(sectorShipRef.second));
    }

    return ships;
}


// ---------------------------------------------------------------------------
// OBJECT ACTIONS
// ---------------------------------------------------------------------------


// delta = seconds
void moveShips(double delta, vector<Ship>& ships, Ship* playerShip, bool useJumpgates) {
    map<Sector*, set_ship_ptr> sectorShipRefs;
    
    for (auto& ship : ships) {
        if (ship.timeout) {
            ship.timeout = std::max(0.0, ship.timeout - delta);
        }
        if (ship.docked && !ship.timeout) {
            ship.docked = false;
        }
        if (ship.docked || ship.currentHull <= 0 || ship.timeout) continue;

        bool    isPlayerShip = &ship == playerShip;
        Sector* sector       = ship.sector;
        auto&   pos          = ship.position;
        auto&   dir          = ship.direction;
        auto&   speed        = ship.speed;
        auto&   dest         = ship.destination;

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
            if (!sectorShipRefs.count(ship.sector)) {
                sectorShipRefs[ship.sector] = set_ship_ptr(ship.sector->ships);
            }
            sectorShipRefs[ship.sector].erase(&ship);
            // add to new sector
            if (!sectorShipRefs.count(sector)) {
                sectorShipRefs[sector] = set_ship_ptr(sector->ships);
            }
            sectorShipRefs[sector].insert(&ship);
            // update ship
            ship.sector = sector;
        }

        // Maintain sector boundary
        if      (pos.x < 0)              pos.x = 0;
        else if (pos.x > sector->size.x) pos.x = sector->size.x;
        if      (pos.y < 0)              pos.y = 0;
        else if (pos.y > sector->size.y) pos.y = sector->size.y;
    }
    
    // Update sector ships
    for (auto& sectorShipRef : sectorShipRefs) {
        sectorShipRef.first->setShips(std::move(sectorShipRef.second));
    }
}


void acquireTargets(Sector& sector) {
    map<Ship*, vector<Ship*>> potentialTargets;
    vector<Ship*> sectorShips;
    sectorShips.reserve(sector.ships->size());

    for (Ship* ship : sector.ships()) {
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
        if (!ship->weapons->empty() || !ship->turrets->empty()) {
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
            for (auto& weapon : ship->weapons()) if (weapon->target) weapon->target = nullptr;
            for (auto& turret : ship->turrets()) if (turret->target) turret->target = nullptr;
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
            for (size_t i=0; i < ship->weapons->size(); ++i) {
                Weapon& weapon = *(ship->weapons()[i]);
                WeaponPosition weaponPosition = isShipSideFire(ship->type)
                                              ? (i < ship->weapons->size()/2)
                                                  ? WeaponPosition_Port
                                                  : WeaponPosition_Starboard
                                              : WeaponPosition_Bow;
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
            for (size_t i=0; i < ship->turrets->size(); ++i) {
                Weapon& turret = *(ship->turrets()[i]);
                float toHit = chanceToHit(turret, true, WeaponPosition_Bow, target);
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
            for (auto& weapon : ship->weapons()) {
                p = &*weapon;
                bestTarget = nullptr;
                if (weaponToHit.count(p)) bestTarget = weaponToHit[p].first;
                if (p->target != bestTarget) p->target = bestTarget;
            }
            for (auto& turret : ship->turrets()) {
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
            vector_weapon_ptr weaponsAndTurrets;
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
                if (auto target = dynamic_cast<Ship*>(weapon->target())) {
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
                                    if (auto target = dynamic_cast<Ship*>(weapon->target())) {
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
        for (auto& weapon : ship.weapons()) if (weapon->cooldown > 0.f) weapon->cooldown -= delta;
        for (auto& turret : ship.turrets()) if (turret->cooldown > 0.f) turret->cooldown -= delta;
    }
}


void respawnShips(vector<Ship>& ships, Ship* playerShip, vector<Station>& stations, bool useJumpgates) {
    if (stations.empty()) return; // return if there are no valid respawn points

    map<Sector*, set_ship_ptr> sectorShipRefs;
    
    for (auto& ship : ships) {
        bool isPlayerShip = &ship == playerShip;
        if (ship.currentHull <= 0 && ship.timeout <= 0.f) {
            // don't respawn ships that are dead in the player's sector
            if (playerShip && !isPlayerShip && ship.sector == playerShip->sector) {
                continue;
            }

            // remove dead ship from the sector
            if (!sectorShipRefs.count(ship.sector)) {
                sectorShipRefs[ship.sector] = set_ship_ptr(ship.sector->ships);
            }
            sectorShipRefs[ship.sector].erase(&ship);

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
            vector_weapon_ptr newWeapons;
            vector_weapon_ptr newTurrets;
            newWeapons.reserve(weapons.size());
            newTurrets.reserve(turrets.size());
            bool isSideFire = isShipSideFire(type);
            for (size_t i = 0; i < (isSideFire ? 2 : 1); ++i) {
                for (WeaponType weapon : weapons) {
                    WeaponPosition weaponPosition = isSideFire
                                                ? i ? WeaponPosition_Port
                                                    : WeaponPosition_Starboard
                                                : WeaponPosition_Bow;
                    newWeapons.emplace(newWeapons.end(), weapon_ptr(new Weapon(weapon, false, weaponPosition, ship)));
                }
            }
            for (WeaponType turret : turrets) {
                newTurrets.emplace(newTurrets.end(), weapon_ptr(new Weapon(turret, true, WeaponPosition_Bow, ship)));
            }
            ship.setWeapons(std::move(newWeapons));
            ship.setTurrets(std::move(newTurrets));

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

            // add to respawn sector
            if (!sectorShipRefs.count(&sector)) {
                sectorShipRefs[&sector] = set_ship_ptr(sector.ships);
            }
            sectorShipRefs[&sector].insert(&ship);
        }
    }
    
    // Update sector ships
    for (auto& sectorShipRef : sectorShipRefs) {
        sectorShipRef.first->setShips(std::move(sectorShipRef.second));
    }
}


// ---------------------------------------------------------------------------
// SERIALIZATION
// ---------------------------------------------------------------------------


const string XML_INDENT = "  ";


typedef vector<pair<string, string>> xml_attrs_t;
string xml_open(string tagname, xml_attrs_t const& attrs={}, bool selfClose=false) {
    ostringstream os;
    os << '<' << tagname;
    for (auto attr : attrs) os << ' ' << attr.first << "=\"" << attr.second << '"';
    os << (selfClose ? "/>" : ">");
    return os.str();
}


string xml_close(string tagname) {
    ostringstream os;
    os << "</" << tagname << '>';
    return os.str();
}


string xml_serializableId(ID id) {
    char buf[13];
    snprintf(buf, 13, "[0x%04zx]", id);
    return string(buf);
}


string xml_serializableId(HasID* hasId) {
    return hasId ? xml_serializableId(hasId->id) : "";
}


template <typename T>
string xml_serializableNumber(T const& t) {
    ostringstream os;
    os << t;
    return os.str();
}


string xml_serializableBool(bool v) {
    return v ? "true" : "false";
}


template <typename T, typename U>
string xml_serializableVector2(Vector2<T,U> const& v) {
    ostringstream os;
    os << '{' << v.x << ',' << v.y << '}';
    return os.str();
}


template <typename T>
string xml_serializablePair(pair<T,T> const& v) {
    ostringstream os;
    os << '{' << v.first << ',' << v.second << '}';
    return os.str();
}


string Sector::xml_tagname() { return "sector"; }
string Sector::xml_serialize(string const& indent) {
    ostringstream os;
    string subindent  = indent    + XML_INDENT;
    string subindent2 = subindent + XML_INDENT;
    os << indent << xml_open(xml_tagname(), {
        {"id",     xml_serializableId(id)},
        {"rowcol", xml_serializablePair(rowcol)},
        {"name",   name},
        {"size",   xml_serializableVector2(size)},
    }) << endl;
    auto jumpgates_ = jumpgates.all();
    if (!jumpgates_.empty()) {
        os << subindent << xml_open("jumpgates", {{"count", xml_serializableNumber(jumpgates_.size())}}) << endl;
        for (auto jumpgate : jumpgates_) os << jumpgate->xml_serialize(subindent2) << endl;
        os << subindent << xml_close("jumpgates") << endl;
    }
    if (!stations.empty()) {
        os << subindent << xml_open("stations", {{"count", xml_serializableNumber(stations.size())}}) << endl;
        for (auto station : stations) os << station->xml_serialize(subindent2) << endl;
        os << subindent << xml_close("stations") << endl;
    }
    if (!ships->empty()) {
        os << subindent << xml_open("ships", {{"count", xml_serializableNumber(ships->size())}}) << endl;
        for (auto ship : ships()) os << ship->xml_serialize(subindent2) << endl;
        os << subindent << xml_close("ships") << endl;
    }
    os << indent << xml_close(xml_tagname());
    return os.str();
}


string Jumpgate::xml_tagname() { return "jumpgate"; }
string Jumpgate::xml_serialize(string const& indent) {
    ostringstream os;
    string nesw;
    if (sector) {
        if      (this == sector->jumpgates.north) nesw="north";
        else if (this == sector->jumpgates.east)  nesw="east";
        else if (this == sector->jumpgates.south) nesw="south";
        else if (this == sector->jumpgates.west)  nesw="west";
    }
    os << indent << xml_open(xml_tagname(), {
        {"id",       xml_serializableId(id)},
//        {"sector",   xml_serializableId(sector)},
        {"nesw",     nesw},
        {"position", xml_serializableVector2(position)},
        {"target",   xml_serializableId(target)},
    }, true);
    return os.str();
}


string Station::xml_tagname() { return "station"; }
string Station::xml_serialize(string const& indent) {
    ostringstream os;
    os << indent << xml_open(xml_tagname(), {
        {"id",       xml_serializableId(id)},
//        {"sector",   xml_serializableId(sector)},
        {"position", xml_serializableVector2(position)},
    }, true);
    return os.str();
}


string Ship::xml_tagname() { return "ship"; }
string Ship::xml_serialize(string const& indent) {
    ostringstream os;
    string subindent  = indent    + XML_INDENT;
    string subindent2 = subindent + XML_INDENT;
    string faction_;
    switch (faction) {
        case ShipFaction_Player  : faction_ = "Player";  break;
        case ShipFaction_Friend  : faction_ = "Friend";  break;
        case ShipFaction_Foe     : faction_ = "Foe";     break;
        default                  : faction_ = "Neutral"; break;
    }
    xml_attrs_t attrs = {
        {"id",           xml_serializableId(id)},
        {"type",         shipClass(type)},
        {"faction",      faction_},
        {"code",         code},
        {"name",         name},
        {"max-hull",     xml_serializableNumber(maxHull)},
        {"current-hull", xml_serializableNumber(currentHull)},
//        {"sector",       xml_serializableId(sector)},
        {"position",     xml_serializableVector2(position)},
        {"direction",    xml_serializableVector2(direction)},
        {"speed",        xml_serializableNumber(speed)},
    };
    if (destination) {
        if (destination->object) attrs.emplace_back("destination-object", xml_serializableId(destination->object));
        attrs.emplace_back("destination-sector",  xml_serializableId(destination->sector));
        attrs.emplace_back("destination-position", xml_serializableVector2(destination->position));
    }
    if (target)        attrs.emplace_back("target",  xml_serializableId(target));
    if (docked)        attrs.emplace_back("docked",  xml_serializableBool(docked));
    if (timeout > 0.0) attrs.emplace_back("timeout", xml_serializableNumber(timeout));
    os << indent << xml_open(xml_tagname(), attrs) << endl;
    if (!weapons->empty()) {
        os << subindent << xml_open("weapons", {{"count", xml_serializableNumber(weapons->size())}}) << endl;
        for (auto& weapon : weapons()) os << weapon->xml_serialize(subindent2) << endl;
        os << subindent << xml_close("weapons") << endl;
    }
    if (!turrets->empty()) {
        os << subindent << xml_open("turrets", {{"count", xml_serializableNumber(turrets->size())}}) << endl;
        for (auto& turret : turrets()) os << turret->xml_serialize(subindent2) << endl;
        os << subindent << xml_close("turrets") << endl;
    }
    os << indent << xml_close(xml_tagname());
    return os.str();
}


string Weapon::xml_tagname() { return "weapon"; }
string Weapon::xml_serialize(string const& indent) {
    ostringstream os;
    string subindent  = indent    + XML_INDENT;
    string subindent2 = subindent + XML_INDENT;
    string type_;
    string weaponPosition_;
    switch (type) {
        case WeaponType_Pulse  : type_ = "Pulse";  break;
        case WeaponType_Cannon : type_ = "Cannon"; break;
        case WeaponType_Beam   : type_ = "Beam";   break;
    }
    switch (weaponPosition) {
        case WeaponPosition_Bow       : weaponPosition_ = "Bow";       break;
        case WeaponPosition_Port      : weaponPosition_ = "Port";      break;
        case WeaponPosition_Starboard : weaponPosition_ = "Starboard"; break;
    }
    xml_attrs_t attrs = {
        {"id",           xml_serializableId(id)},
        {"type",         type_},
//        {"is-turret",    xml_serializeableBool(isTurret)},
//        {"parent",       xml_serializableId(parent)},
    };
    if (target)         attrs.emplace_back("target",          xml_serializableId(target));
    if (weaponPosition) attrs.emplace_back("weapon-position", weaponPosition_);
    if (cooldown > 0.f) attrs.emplace_back("cooldown",        xml_serializableNumber(cooldown));
    os << indent << xml_open(xml_tagname(), attrs, true);
    return os.str();
}


void xml_serialize_savegame(std::ostream& os, vector<vector<Sector>> const& sectors, vector<Jumpgate> const& jumpgates, vector<Station> const& stations, vector<Ship>const& ships) {
    string subindent = XML_INDENT;
    os << xml_open("savegame") << endl
       << subindent << xml_open("sectors", {{"count", xml_serializableNumber(sectors.size() * sectors[0].size())}}, true) << endl
       << subindent << xml_open("jumpgates", {{"count", xml_serializableNumber(jumpgates.size())}}, true) << endl
       << subindent << xml_open("stations", {{"count", xml_serializableNumber(stations.size())}}, true) << endl
       << subindent << xml_open("ships", {{"count", xml_serializableNumber(ships.size())}}, true) << endl;
    for (auto sectorRow : sectors) {
        for (auto sector : sectorRow) {
            os << sector.xml_serialize(subindent) << endl;
        }
    }
    os << xml_close("savegame");
}


// ---------------------------------------------------------------------------
// PROGRAM
// ---------------------------------------------------------------------------


int main(int argc, char** argv) {
    std::srand(time(nullptr));

    ofstream livefile, snapfile;

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

    auto performSave = [&](ostream& os) {
        duration<double> d_save;
        time_point<steady_clock> t;
        
        t = steady_clock::now();
        xml_serialize_savegame(os, sectors, jumpgates, stations, ships);
//        update_aftersave(); // commented to allow main thread control for this example
        d_save = steady_clock::now() - t;
        os << endl << "save time: " << (d_save.count() * 1000) << "ms" << endl;
    };

    auto saveThreadFn = [&](ostream& os, int delay=0) {
//        isSaving = true; // commented to allow main thread control for this example
        willSave = false;

        // artificially make the save state take longer
        if (delay) std::this_thread::sleep_for(milliseconds(delay));

        os << "SNAP" << endl << endl;
        performSave(os);

//        isSaving = false; // commented to allow main thread control for this example
        saveComplete = true;
    };

    auto mainThreadFn = [&]() {
        duration<double>         d1, d2, delta;
        time_point<steady_clock> t, thisTick, nextTick = steady_clock::now(), lastTick = nextTick;

        while (!endGame) {
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
            
            cout << "delta: "   << (delta.count() * 1000) << "ms" << "  "
                 << "work: "    << (d1.count() * 1000)    << "ms" << "  "
                 << "display: " << (d2.count() * 1000)    << "ms" << endl;

            lastTick = thisTick;

            // ---------------------------------------------------------------
            // TRIGGER SERIALIZATION AND EXIT WHEN COMPLETE
            // ---------------------------------------------------------------

            static size_t count = 0;
            count++;
            // save on background thread around 10 and 20 seconds (at ~3 frames per second)
            if (count == 30 || count == 60) {

                // write out live state for comparison
                livefile.open(count == 30 ? "tiny-space_01-live.txt" : "tiny-space_03-live.txt");
                livefile << "LIVE" << endl << endl;
                performSave(livefile);
                livefile.close();

                willSave = true;

                // under main thread control for this example.
                // first snapshot should match live (as printed above).
                // isSaving stays true between the two save runs.
                // their outputs should be exactly the same!
                isSaving = true; 

                snapfile.open(count == 30 ? "tiny-space_02-snap.txt" : "tiny-space_04-snap.txt");
                saveThread.store(new thread(saveThreadFn, std::ref(snapfile), 5000));
            }

            // join save thread when complete
            if (saveComplete) {
                if (count > 60) { // hold save state open for about 10 seconds between background saves
                    isSaving = false; // under main thread control for this example
                    update_aftersave(); // under main thread control for this example
                }

                saveComplete = false;

                if (saveThread.load()) {
                    saveThread.load()->join();
                    saveThread = nullptr;
                    snapfile.close();
                }
            }

            // exit around 30 seconds (at ~3 frames per second)
            if (count >= 90 && !willSave && !isSaving && !saveComplete) {
                endGame = true;
            }
        }
    };

    thread mainThread = thread(mainThreadFn);
    mainThread.join();

    return 0;
}
