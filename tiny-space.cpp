// tiny-space.cpp (v2) (C++11)
// AUTHOR: xixas | DATE: 2022.01.23 | LICENSE: WTFPL/PDM/CC0... your choice
//
// DESCRIPTION: Gaming: Space sim in a tiny package.
//                      Work In Progress.
//
// BUILD: g++ --std=c++11 -O3 -lpthread -o tiny-space tiny-space.cpp
//
// CREATED:  2022.01.22 | AUTHOR: xixas | (v1) Original
// MODIFIED: 2022.01.23 | AUTHOR: xixas | (v2) Added colors and jumpgate travel


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


// ---------------------------------------------------------------------------
// GAME CLASSES
// ---------------------------------------------------------------------------


template <typename T, typename U> struct Vector2 {
    T x;
    T y;

    Vector2() : x(), y() {}
    Vector2(T const& x, T const& y) : x(x), y(y) {}
    Vector2(Vector2<T,U> const& o) : x(o.x), y(o.y) {}

    Vector2<T,U>& operator=(const Vector2<T,U>& o) { x = o.x, y = o.y; return *this; }

    Vector2<T,U> operator +(Vector2<T,U> const& o) const { return {x+o.x, y+o.y}; }
    Vector2<T,U> operator -(Vector2<T,U> const& o) const { return {x-o.x, y-o.y}; }

    Vector2<T,U> operator -() const { return {-x, -y}; }

    Vector2<T,U> operator *(U u) const { return {x*u, y*u}; }
    Vector2<T,U> operator /(U u) const { return {x/u, y/u}; }
    Vector2<T,U> operator +(U u) const { return *this + (this->normalized() * u); }
    Vector2<T,U> operator -(U u) const { return *this - (this->normalized() * u); }

    U magnitude() const { return sqrt(x*x + y*y); }
    Vector2<T,U> normalized() const { U m = magnitude(); if (!m) return *this; return {static_cast<T>(x/m), static_cast<T>(y/m)}; }

    T dot(Vector2<T,U> const& o) const { return x*o.x + y*o.y; }
};
template <typename T, typename U> inline Vector2<T,U> operator +(Vector2<T,U> const& lhs, Vector2<T,U> const& rhs) { return {lhs.x + rhs.x, lhs.y + rhs.y}; }
template <typename T, typename U> inline Vector2<T,U> operator -(Vector2<T,U> const& lhs, Vector2<T,U> const& rhs) { return {lhs.x - rhs.x, lhs.y - rhs.y}; }
template <typename T, typename U> inline std::ostream& operator <<(std::ostream& os, const Vector2<T,U>& o) { os << '{' << o.x << ',' << o.y << '}'; return os; }


enum IDType { IDType_Sector, IDType_Ship, IDType_ShipWeapon, IDType_END };
struct HasID {
    ID id;
    IDType idType;

    HasID(IDType const& idType) : id(++curId), idType(idType) {}
    HasID(ID const& id, IDType const& idType) : id(id), idType(idType) {}
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


struct Destination : public HasSectorAndPosition {
    HasSectorAndPosition* object;  // sector and position are usually ignore if object is set

    Destination(HasSectorAndPosition& object)
        : HasSectorAndPosition(object.sector, object.position), object(&object) {}

    Destination(Sector& sector, position_t const& position)
        : HasSectorAndPosition(&sector, position), object(nullptr) {}

    Sector* currentSector() { return object ? object->sector : sector; }
    position_t currentPosition() { return object ? object->position : position; }
};


struct HasDestination {
    shared_ptr<Destination> destination;

    HasDestination(shared_ptr<Destination> destination=nullptr) : destination(destination) {}
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


struct Jumpgate : public HasSectorAndPosition {
    Jumpgate* target;

    Jumpgate(Sector* const sector=nullptr, position_t const& position={0,0}, Jumpgate* const target = nullptr)
        : HasSectorAndPosition(sector, position), target(target) {};
};


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
struct Sector: public HasID, public HasName, public HasSize {
    SectorNeighbors neighbors;
    SectorJumpgates jumpgates;
    set<Ship*>      ships;

    Sector(string const& name="", dimensions_t const& size={0, 0})
        : HasID(IDType_Sector), HasName(name), HasSize(size), neighbors(), ships() {}

    Sector(ID const& id, string const& name="", dimensions_t const& size={0, 0})
        : HasID(id, IDType_Sector), HasName(name), HasSize(size), neighbors(), ships() {}
};


struct Weapon;
enum ShipType { ShipType_Scout, ShipType_Corvette, ShipType_Frigate, ShipType_Transport, ShipType_END };
string shipClass(ShipType const& shipType) {
    switch (shipType) {
        case ShipType_Scout:     return "Scout";
        case ShipType_Corvette:  return "Corvette";
        case ShipType_Frigate:   return "Frigate";
        case ShipType_Transport: return "Transport";
        default:                 return "";
    }
}
string paddedShipClass(ShipType const& shipType) {
    switch (shipType) {
        case ShipType_Scout:     return "Scout    ";
        case ShipType_Corvette:  return "Corvette ";
        case ShipType_Frigate:   return "Frigate  ";
        case ShipType_Transport: return "Transport";
        default:                 return "         ";
    }
}
enum ShipFaction { ShipFaction_Neutral, ShipFaction_Player, ShipFaction_Friend, ShipFaction_Foe, ShipFaction_END };
struct Ship : public HasID, public HasCode, public HasName,
              public HasSectorAndPosition,
              public HasDirection, public HasSpeed,
              public HasDestination
{
    ShipType        type;
    ShipFaction     faction;
    vector<Weapon*> weapons;

    Ship(ShipType type, string const& code="", string const& name="",
        Sector* const sector=nullptr, position_t const& position={0, 0},
        direction_t const& direction={0, 0}, speed_t const& speed=0,
        shared_ptr<Destination> destination=nullptr)
        :
        HasID(IDType_Ship), HasName(name), HasCode(code),
        HasSectorAndPosition(sector, position),
        HasDirection(direction), HasSpeed(speed),
        HasDestination(destination),
        type(type), faction(ShipFaction_Neutral), weapons() {}

    Ship(ID const& id, ShipType type, string const& code="", string const& name="",
        Sector* const sector=nullptr, position_t const& position={0, 0},
        direction_t const& direction={0, 0}, speed_t const& speed=0,
        shared_ptr<Destination> destination=nullptr)
        :
        HasID(id, IDType_Ship), HasName(name), HasCode(code),
        HasSectorAndPosition(sector, position),
        HasDirection(direction), HasSpeed(speed),
        HasDestination(destination),
        type(type), faction(ShipFaction_Neutral), weapons() {}
};
inline bool operator <(const Ship& lhs, const Ship& rhs) { return lhs.id < rhs.id; }
inline bool operator ==(const Ship& lhs, const Ship& rhs) { return lhs.id == rhs.id; }
namespace std { template<> struct hash<Ship> { ID operator ()(Ship const& o) const noexcept { return o.id; } }; }


enum WeaponType { WeaponType_Laser, WeaponType_Pulse, WeaponType_Missile, WeaponType_END };
struct Weapon : public HasID {
    WeaponType type;

    ID* ship;
    ID* target;
};


// ---------------------------------------------------------------------------
// UTILITY
// ---------------------------------------------------------------------------


// Returns a value between min and max (inclusive)
inline float randFloat(float min=0.0f, float max=1.0f) {
    return min + (static_cast<float>(rand()) / static_cast<float>(RAND_MAX/(max-min)));
}


inline position_t randPosition(position_t min, position_t max) {
    return {randFloat(min.x, max.x), randFloat(min.y, max.y)};
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


inline ShipType randShipType() { return static_cast<ShipType>(rand() % ShipType_END); }


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
        case ShipType_Scout:     os << "S" << std::setfill('0') << std::setw(3) << ++scoutCur;     break;
        case ShipType_Corvette:  os << "C" << std::setfill('0') << std::setw(3) << ++corvetteCur;  break;
        case ShipType_Frigate:   os << "F" << std::setfill('0') << std::setw(3) << ++frigateCur;   break;
        case ShipType_Transport: os << "T" << std::setfill('0') << std::setw(3) << ++transportCur; break;
        default:                                              break;
    }
    return os.str();
}


shared_ptr<Destination> randDestination(Sector& sector, bool useJumpgates, vector<HasSectorAndPosition*>* excludes=nullptr) {
    if (useJumpgates) {
        auto jumpgates = sector.jumpgates.all();
        if (excludes) {
            for (auto exclude : *excludes) {
                if (auto jumpgate = dynamic_cast<Jumpgate*>(exclude)) {
                    auto it = std::find(jumpgates.begin(), jumpgates.end(), jumpgate);
                    if (it != jumpgates.end()) {
                        jumpgates.erase(it);
                    }
                }
            }
        }
        if (!jumpgates.empty()) {
            auto jumpgate = jumpgates[rand() % jumpgates.size()];
            return shared_ptr<Destination>(new Destination(*jumpgate));
        }
    }
    return shared_ptr<Destination>(new Destination(sector, randPosition({0,0}, sector.size)));
}


// ---------------------------------------------------------------------------
// PROGRAM
// ---------------------------------------------------------------------------


const v2size_t     SECTOR_BOUNDS           = {10, 10};
const dimensions_t SECTOR_SIZE             = {20, 20};
const size_t       SECTOR_MAP_LEFT_PADDING = 6;
const size_t       SHIP_COUNT              = 500;
const float        ENEMY_FREQUENCY         = 0.1f;
const float        FRIEND_FREQUENCY        = 0.2f;
const size_t       TICK_TIME               = 300; // milliseconds

const speed_t
    SCOUT_SPEED     = 600,
    CORVETTE_SPEED  = 400,
    FRIGATE_SPEED   = 200,
    TRANSPORT_SPEED = 300;

const speed_t SPEED_MULTIPLIER = 0.002f;


speed_t maxShipSpeed(ShipType shipType) {
    switch (shipType) {
        case ShipType_Scout:     return SPEED_MULTIPLIER * SCOUT_SPEED;
        case ShipType_Corvette:  return SPEED_MULTIPLIER * CORVETTE_SPEED;
        case ShipType_Frigate:   return SPEED_MULTIPLIER * FRIGATE_SPEED;
        case ShipType_Transport: return SPEED_MULTIPLIER * TRANSPORT_SPEED;
        default:                 return 0;
    }
}


// ---------------------------------------------------------------------------
// UI
// ---------------------------------------------------------------------------


const unsigned int
    COLOR_RED   = 31,
    COLOR_GREEN = 32,
    COLOR_BLUE  = 34,
    COLOR_CYAN  = 36, //34,

    PLAYER_COLOR  = COLOR_GREEN,
    NEUTRAL_COLOR = COLOR_BLUE,
    FRIEND_COLOR  = COLOR_CYAN,
    ENEMY_COLOR   = COLOR_RED;
string beginColorString(unsigned int color, bool useColor=true) {
    if (!useColor) return "";
    ostringstream os;
    os << "\033[1;" << color << 'm';
    return os.str();
}
string endColorString(bool useColor=true) {
    if (!useColor) return "";
    return "\033[0m";
}
string colorString(unsigned int color, string const& str, bool useColor=true) {
    if (!useColor) return str;
    ostringstream os;
    os << beginColorString(color) << str << endColorString();
    return os.str();
}


string shipString(Ship const& ship) {
    ostringstream os;
    os << "(";
    if (ship.name.size()) os << " name:" << ship.name;
    if (ship.code.size()) os << " code:" << ship.code;
//    if (ship.sector) os << " sec:" << ship.sector->name;
    auto loc = ship.position - (ship.sector ? ship.sector->size/2 : dimensions_t{0, 0});
    auto& dir = ship.direction;
    os << std::fixed << std::setprecision(1)
        << " loc:{"
        << (loc.x >= 0 ? " " : "") << loc.x << ","
        << (-loc.y >= 0 ? " " : "") << -loc.y << "}";
    os << std::fixed << std::setprecision(1)
        << " dir:{"
        << (dir.x >= 0 ? " " : "") << dir.x << ","
        << (-dir.y >= 0 ? " " : "") << -dir.y << "}";
    auto shipClass = paddedShipClass(ship.type);
    if (shipClass.size()) os << " class:" << shipClass;
    os << " )";
    return os.str();
}


vector<string> createSectorShipsList(Sector& sector, Ship* playerShip, bool const useColor) {
    vector<string> shipsList;
    for (auto ship : sector.ships) {
        ostringstream os;
        bool isPlayerShip = ship == playerShip;
        bool isFriend = ship->faction == ShipFaction_Friend;
        bool isEnemy = ship->faction == ShipFaction_Foe;
        string shipStr = shipString(*ship);
        if (useColor) {
            auto color = isPlayerShip ? PLAYER_COLOR
                       : isFriend     ? FRIEND_COLOR
                       : isEnemy      ? ENEMY_COLOR
                       :                NEUTRAL_COLOR
                       ;
            shipStr = colorString(color, shipStr);
        }
        os << ' '
           << ( isPlayerShip ? '>'
              : isFriend     ? '+'
              : isEnemy      ? '-'
              :                ' '
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
    // map<(row,col), (replace_length, replace_str)>
    map<pair<size_t, size_t>, pair<size_t, string>> replacements;

    // ships
    for (auto ship : sector.ships) {
        bool isPlayerShip = ship == playerShip;
        bool isFriend = ship->faction == ShipFaction_Friend;
        bool isEnemy = ship->faction == ShipFaction_Foe;
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
            if (useColor) {
                auto color = isPlayerShip ? PLAYER_COLOR
                           : isFriend     ? FRIEND_COLOR
                           : isEnemy      ? ENEMY_COLOR
                           :                NEUTRAL_COLOR;
                shipStr = colorString(color, shipStr);
            }
            replacements[{row,col}] = {1, shipStr};
        }
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
        string jumpgateStr = colorString(FRIEND_COLOR, "0", useColor);
        replacements[{row,col}] = {1, jumpgateStr};
    }

    // Charatcer Replacements
    for (auto it = replacements.crbegin(); it != replacements.crend(); ++it) {
        sectorMap[it->first.first].replace(it->first.second, it->second.first, it->second.second);
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
            if (useColor && isPlayerSector) os << beginColorString(PLAYER_COLOR) ;
            os << (isPlayerSector ? "[" : " ");
            if (shipCount) {
                char shipCountBuf[5];
                sprintf(shipCountBuf, "%4zu", shipCount);
                os << shipCountBuf;
            } else {
                os << "    ";
            }
            os << (isPlayerSector ? " ]" : "  ");
            if (useColor && isPlayerSector) os << endColorString();
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
//    cout << string(78, '-') << endl;
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
        Jumpgate& localJumpgate = *(jumpgatesBuf.emplace(jumpgatesBuf.end(), &sector, position_t{sector.size.x/2, 0.25f}, nullptr));
        Jumpgate& remoteJumpgate = *(jumpgatesBuf.emplace(jumpgatesBuf.end(), &neighbor, position_t{neighbor.size.x/2, neighbor.size.y-0.25f}, &localJumpgate));
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
        Jumpgate& localJumpgate = *(jumpgatesBuf.emplace(jumpgatesBuf.end(), &sector, position_t{sector.size.x-0.25f, sector.size.y/2}, nullptr));
        Jumpgate& remoteJumpgate = *(jumpgatesBuf.emplace(jumpgatesBuf.end(), &neighbor, position_t{0.25f, neighbor.size.y/2}, &localJumpgate));
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
        Jumpgate& localJumpgate = *(jumpgatesBuf.emplace(jumpgatesBuf.end(), &sector, position_t{sector.size.x/2, sector.size.y-0.25f}, nullptr));
        Jumpgate& remoteJumpgate = *(jumpgatesBuf.emplace(jumpgatesBuf.end(), &neighbor, position_t{neighbor.size.x/2, 0.25f}, &localJumpgate));
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
        Jumpgate& localJumpgate = *(jumpgatesBuf.emplace(jumpgatesBuf.end(), &sector, position_t{0.25f, sector.size.y/2}, nullptr));
        Jumpgate& remoteJumpgate = *(jumpgatesBuf.emplace(jumpgatesBuf.end(), &neighbor, position_t{neighbor.size.x-0.25f, neighbor.size.y/2}, &localJumpgate));
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


vector<Ship> initShips(size_t const& shipCount, vector<vector<Sector>>& sectors, bool useJumpgates, float const& wallBuffer=0.1f) {
    vector<Ship> ships;
    ships.reserve(shipCount);

    for (size_t i=0; i<shipCount; ++i) {
        auto& sector = sectors[rand() % sectors.size()][rand() % sectors[0].size()];
        auto type    = randShipType();
        auto code    = randCode();
        auto name    = randName(type);
        auto pos     = randPosition(sector.size, wallBuffer);
        auto dest    = randDestination(sector, useJumpgates);
        auto dir     = dest ? (dest->position - pos).normalized() : randDirection();
        auto speed   = maxShipSpeed(type);
        auto it      = ships.emplace(ships.end(), type, code, name, &sector, pos, dir, speed, dest);
        auto& ship   = *it;
        // friend/foe
        if (i == 0) {
            ship.faction = ShipFaction_Player;
        } else {
            float rnd = randFloat();
            if (rnd < ENEMY_FREQUENCY) {
                ship.faction = ShipFaction_Foe;
            } else if (rnd < ENEMY_FREQUENCY + FRIEND_FREQUENCY) {
                ship.faction = ShipFaction_Friend;
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
void moveShips(double delta, vector<Ship>& ships, bool useJumpgates) {
    for (auto& ship : ships) {
        auto  sector = ship.sector;
        auto& pos    = ship.position;
        auto& dir    = ship.direction;
        auto& speed  = ship.speed;
        auto& dest   = ship.destination;

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

                if (useJumpgates) {
                    if (auto jumpgate = dynamic_cast<Jumpgate*>(dest->object)) {
                        sector = jumpgate->target->sector;
                        pos = jumpgate->target->position;
                        excludes.push_back(jumpgate->target);
                    }
                }

                if (dest->object) {
                    dest = randDestination(*sector, useJumpgates, &excludes);
                    dir = (dest->position - pos).normalized();
                } else {
                    dest = nullptr;
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
    auto ships     = initShips(SHIP_COUNT, sectors, useJumpgates);

    auto& playerShip = ships[0];

    auto mainThreadFn = [&]() {
        time_point<steady_clock> t, thisTick, nextTick;
        time_point<steady_clock> lastTick = steady_clock::now();
        duration<double>         d1, d2, delta;
        while (true) {
            thisTick   = steady_clock::now();
            delta      = thisTick - lastTick; // seconds
            nextTick   = thisTick + milliseconds(TICK_TIME);

            t = steady_clock::now();
            moveShips(delta.count(), ships, useJumpgates);
            d1 = steady_clock::now() - t;
            
            t = steady_clock::now();
            updateDisplay(sectors, playerShip, useColor);
            d2 = steady_clock::now() - t;
            
            cout << "delta: " << (delta.count() * 1000) << "ms" << "  "
                 << "work: " << (d1.count() * 1000) << "ms" << "  "
                 << "display: " << (d2.count() * 1000) << "ms"
                 << endl;

            lastTick = thisTick;
            std::this_thread::sleep_until(nextTick);
        }
    };

    thread mainThread = thread(mainThreadFn);
    mainThread.join();

    return 0;
}
