// tiny-space.cpp (v1) (C++11)
// AUTHOR: xixas | DATE: 2022.01.22 | LICENSE: WTFPL/PDM/CC0... your choice
// DESCRIPTION: Gaming: Space sim in a tiny package.
//                      Demo base for testing real-time save mechanisms.
//                      Work In Progress.
// BUILD: g++ --std=c++11 -O3 -lpthread -o tiny-space tiny-space.cpp


#include <atomic>
#include <chrono>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <initializer_list>
#include <iomanip>
#include <iostream>
#include <set>
#include <sstream>
#include <string>
#include <thread>
#include <time.h>
#include <unordered_set>
#include <vector>


using std::chrono::duration;
using std::chrono::system_clock;
using std::chrono::time_point;
using std::cout;
using std::endl;
using std::ostringstream;
using std::set;
using std::string;
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
};
template <typename T> inline Vector2<T> operator +(Vector2<T> const& lhs, Vector2<T> const& rhs) { return {lhs.x + rhs.x, lhs.y + rhs.y}; }
template <typename T> inline Vector2<T> operator -(Vector2<T> const& lhs, Vector2<T> const& rhs) { return {lhs.x - rhs.x, lhs.y - rhs.y}; }


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


struct SectorNeighbors {
    Sector* north;
    Sector* east;
    Sector* south;
    Sector* west;

    SectorNeighbors() : north(nullptr), east(nullptr), south(nullptr), west(nullptr) {}
};


struct Ship;
struct Sector: public HasID, public HasName, public HasSize {
    SectorNeighbors neighbors;
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
        default:                 return "";
    }
}
struct Ship : public HasID, public HasCode, public HasName, public HasSector,
              public HasPosition, public HasDirection, public HasSpeed
{
    ShipType        type;
    vector<Weapon*> weapons;

    Ship(ShipType type, string const& code="", string const& name="",
        Sector* const sector=nullptr, position_t const& position={0, 0},
        direction_t const& direction={0, 0}, speed_t const& speed=0)
        :
        HasID(IDType_Ship), HasName(name), HasCode(code), HasSector(sector),
        HasPosition(position), HasDirection(direction), HasSpeed(speed),
        type(type), weapons() {}

    Ship(ID const& id, ShipType type, string const& code="", string const& name="",
        Sector* const sector=nullptr, position_t const& position={0, 0},
        direction_t const& direction={0, 0}, speed_t const& speed=0)
        :
        HasID(id, IDType_Ship), HasName(name), HasCode(code), HasSector(sector),
        HasPosition(position), HasDirection(direction), HasSpeed(speed),
        type(type), weapons() {}

    string str() const {
        ostringstream os;
        os << "(";
        if (name.size()) os << " name:" << name;
        if (code.size()) os << " code:" << code;
//        if (sector) os << " sec:" << sector->name;
        auto loc = position - (sector ? sector->size/2 : dimensions_t{0, 0});
        os << std::fixed << std::setprecision(1)
           << " loc:{"
           << (loc.x >= 0 ? " " : "") << loc.x << ","
           << (loc.y >= 0 ? " " : "") << loc.y << "}";
        os << std::fixed << std::setprecision(1)
           << " dir:{"
           << (direction.x >= 0 ? " " : "") << direction.x << ","
           << (direction.y >= 0 ? " " : "") << direction.y << "}";
        auto shipClass = paddedShipClass(type);
        if (shipClass.size()) os << " class:" << shipClass;
        os << " )";
        return os.str();
    }
};
inline bool operator <(const Ship& lhs, const Ship& rhs) { return lhs.id < rhs.id; }
inline bool operator ==(const Ship& lhs, const Ship& rhs) { return lhs.id == rhs.id; }
inline std::ostream& operator <<(std::ostream& os, const Ship& o) { os << o.str(); return os; }
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


// ---------------------------------------------------------------------------
// UI
// ---------------------------------------------------------------------------


vector<string> createSectorMap(Sector& sector, Ship* playerShip) {
    static string leftPadding(19, ' ');
    auto& sectorSize = sector.size;
    vector<string> sectorMap; sectorMap.reserve(static_cast<size_t>(sectorSize.x+3));
    { // header
        ostringstream os;
        os << leftPadding << "+-[ " << sector.name << " ]" << string(((sectorSize.x+1) * 3) - sector.name.size() - 5, '-') << '+';
        sectorMap.push_back(os.str());
    }
    for (size_t i=0; i<=sectorSize.y; ++i) {
        ostringstream os;
        os << leftPadding << '|' << string((sectorSize.x+1) * 3, ' ') << '|';
        sectorMap.push_back(os.str());
    }
    { // footer
        ostringstream os;
        os << leftPadding << '+' << string((sectorSize.x+1) * 3, '-') << '+';
        sectorMap.push_back(os.str());
    }
    // ships
    for (auto ship : sector.ships) {
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
        if (sectorMap[row][col] == ' ' || ship == playerShip) {
            char shipChar = '.';
            if (ship == playerShip) {
                auto& dir = ship->direction;
                auto dirMax = std::max(dir.y, 0.0f);
                shipChar = 'v';
                if (dir.x > 0 && dir.x > dirMax) { dirMax = dir.x; shipChar = '>'; }
                if (dir.y < 0 && std::abs(dir.y) > dirMax) { dirMax = std::abs(ship->direction.y); shipChar = '^'; }
                if (dir.x < 0 && std::abs(dir.x) > dirMax) { shipChar = '<'; }
            }
            sectorMap[row][col] = shipChar;
//cout << pos.y << '=' << row << ' ' << pos.x << '=' << col << endl;
        }
    }
    return std::move(sectorMap);
}


vector<string> createWorldMap(vector<vector<Sector>> const& sectors, Ship* playerShip) {
    vector<string> worldMap; worldMap.reserve(sectors.size()+1);
    { // column headers
        ostringstream os;
        os << "    ";
        for (size_t i=0; i<sectors[0].size(); ++i) {
            os << "    " << static_cast<char>('A' + i) << "  ";
        }
        worldMap.push_back(os.str());
    }
    { // header divider
        ostringstream os;
        os << "   ." << string(sectors[0].size() * 7, '-');
        worldMap.push_back(os.str());
    }
    for (size_t i=0; i<sectors.size(); ++i) {
        ostringstream os;
        char rowBuf[3];
        sprintf(rowBuf, "%02zu", i+1);
        os << rowBuf << " |";
        for (size_t j=0; j<sectors[i].size(); ++j) {
            Sector const& sector = sectors[i][j];
            bool isPlayerSector = playerShip && &sector == playerShip->sector;
            size_t shipCount = sector.ships.size();
            os << (isPlayerSector ? '[' : ' ');
            if (shipCount) {
                char shipCountBuf[5];
                sprintf(shipCountBuf, "%4zu", shipCount);
                os << shipCountBuf;
            } else {
                os << "    ";
            }
            os << (isPlayerSector ? " ]" : "  ");
        }
        worldMap.push_back(os.str());
    }
    return std::move(worldMap);
}

void updateDisplay(vector<vector<Sector>>& sectors, Ship& playerShip) {
    auto sectorMap = createSectorMap(*playerShip.sector, &playerShip);
    auto worldMap = createWorldMap(sectors, &playerShip);
    
    for (size_t i = 0; i<50; ++i) cout << endl;
//    cout << string(78, '-') << endl;
    cout << endl;
    for (auto ship : playerShip.sector->ships) {
        cout << (ship == &playerShip ? '>' : ' ') << ' ' << *ship << endl;
    }
    cout << endl;
    for (auto& line : sectorMap) cout << line << endl;
    cout << endl;
    for (auto& line : worldMap) cout << line << endl;
    cout << endl;
}


// ---------------------------------------------------------------------------
// PROGRAM
// ---------------------------------------------------------------------------


const speed_t
    SCOUT_SPEED     = 0.6f,
    CORVETTE_SPEED  = 0.4f,
    FRIGATE_SPEED   = 0.2f,
    TRANSPORT_SPEED = 0.3f;


speed_t maxShipSpeed(ShipType shipType) {
    switch (shipType) {
        case ShipType_Scout:     return SCOUT_SPEED;
        case ShipType_Corvette:  return CORVETTE_SPEED;
        case ShipType_Frigate:   return FRIGATE_SPEED;
        case ShipType_Transport: return TRANSPORT_SPEED;
        default:                 return 0;
    }
}


vector<vector<Sector>> initSectors(v2size_t const& bounds, dimensions_t const& size) {
    vector<vector<Sector>> sectors;
    {
        auto& rowCount = bounds.y;
        auto& colCount = bounds.x;
        char name[4];
        Sector* sector;
        sectors.reserve(rowCount);
        for (size_t row=0; row<rowCount; ++row) {
            sectors.emplace_back();
            sectors[row].reserve(colCount);
            for (size_t col=0; col<colCount; ++col) {
                sprintf(name, "%02zu%c", 1+row, 'A'+col);
                sectors[row].emplace_back(name, size);
            }
        }
        for (size_t row=0; row<rowCount; ++row) {
            for (size_t col=0; col<colCount; ++col) {
                auto& sector = sectors[row][col];
                sector.neighbors.north = (row == 0) ? nullptr : &sectors[row-1][col];
                sector.neighbors.south = (row == rowCount - 1) ? nullptr : &sectors[row+1][col];
                sector.neighbors.west = (col == 0) ? nullptr : &sectors[row][col-1];
                sector.neighbors.east = (col == colCount - 1) ? nullptr : &sectors[row][col+1];
            }
        }
    }
    return sectors;
}


vector<Ship> initShips(size_t const& shipCount, vector<vector<Sector>>& sectors, float const& wallBuffer=0.1f) {
    vector<Ship> ships;
    ships.reserve(shipCount);
    for (size_t i=0; i<shipCount; ++i) {
        Sector& sector = sectors[rand() % sectors.size()][rand() % sectors[0].size()];
        auto type  = randShipType();
        auto code  = randCode();
        auto name  = randName(type);
        auto pos   = randPosition(sector.size, wallBuffer);
        auto dir   = randDirection();
        auto speed = maxShipSpeed(type);
        auto it    = ships.emplace(ships.end(), type, code, name, &sector, pos, dir, speed);
        sector.ships.insert(&*it);
//cout << *it << endl;
    }
    return ships;
}


void moveShips(vector<Ship>& ships) {
    for (auto& ship : ships) {
        auto& pos   = ship.position;
        auto& dir   = ship.direction;
        auto& speed = ship.speed;
        Sector* sector = ship.sector;

        pos = pos + (dir * speed);
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
        if      (pos.x < 0)              pos.x = 0;
        else if (pos.x > sector->size.x) pos.x = sector->size.x;
        if      (pos.y < 0)              pos.y = 0;
        else if (pos.y > sector->size.y) pos.y = sector->size.y;
    }
}


int main() {
    std::srand(time(nullptr));

    v2size_t     sectorBounds = {10, 10};
    dimensions_t sectorSize   = {10, 10};
    size_t       shipCount    = 500;
    size_t       tick         = 300; // milliseconds

    auto sectors = initSectors(sectorBounds, sectorSize);
    auto ships = initShips(shipCount, sectors);

    auto& playerShip = ships[0];

    updateDisplay(sectors, playerShip);

    time_point<system_clock> t;
    duration<double>         d1, d2;
    while (true) {
        std::this_thread::sleep_for(std::chrono::milliseconds(tick));

        t = system_clock::now();
        moveShips(ships);
        d1 = system_clock::now() - t;
        
        t = system_clock::now();
        updateDisplay(sectors, playerShip);
        d2 = system_clock::now() - t;
        
        cout << "work: " << (d1.count() * 1000) << "ms" << "  "
             << "display: " << (d2.count() * 1000) << "ms"
             << endl;
    }

    return 0;
}
