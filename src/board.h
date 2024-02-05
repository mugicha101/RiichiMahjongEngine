#pragma once

/* tile type bit mappings (stored in 8 bit char)
suits
    00XXXX - honors
    01XXXX - pin suit (circles)
    10XXXX - sou suit (bamboo)
    11XXXX - wan suit (characters)
honors tiles
    0000XX - dragons
    0001XX - winds
dragons
    000000 - no tile
    000001 - white dragon
    000010 - green dragon
    000011 - red dragon
winds
    000100 - east wind
    000101 - south wind
    000110 - west wind
    000111 - north wind
suited tiles
    XX0001 - 1
    XX0010 - 2
    XX0011 - 3
    XX0100 - 4
    XX0101 - 5
    XX0110 - 6
    XX0111 - 7
    XX1000 - 8
    XX1001 - 9
*/
// Red Tile: mark 7th bit (ignore when doing checks)

#include <stdint.h>
#include <array>

typedef int8_t TileType;

const TileType NONE = 0b000000;

const TileType DGNW = 0b000001;
const TileType DGNG = 0b000010;
const TileType DGNR = 0b000011;

const TileType WNDE = 0b000100;
const TileType WNDS = 0b000101;
const TileType WNDW = 0b000110;
const TileType WNDN = 0b000111;

const TileType PIN1 = 0b010001;
const TileType PIN2 = 0b010010;
const TileType PIN3 = 0b010011;
const TileType PIN4 = 0b010100;
const TileType PIN5 = 0b010101;
const TileType PIN6 = 0b010110;
const TileType PIN7 = 0b010111;
const TileType PIN8 = 0b011000;
const TileType PIN9 = 0b011001;

const TileType SOU1 = 0b100001;
const TileType SOU2 = 0b100010;
const TileType SOU3 = 0b100011;
const TileType SOU4 = 0b100100;
const TileType SOU5 = 0b100101;
const TileType SOU6 = 0b100110;
const TileType SOU7 = 0b100111;
const TileType SOU8 = 0b101000;
const TileType SOU9 = 0b101001;

const TileType MAN1 = 0b110001;
const TileType MAN2 = 0b110010;
const TileType MAN3 = 0b110011;
const TileType MAN4 = 0b110100;
const TileType MAN5 = 0b110101;
const TileType MAN6 = 0b110110;
const TileType MAN7 = 0b110111;
const TileType MAN8 = 0b111000;
const TileType MAN9 = 0b111001;

const size_t MAX_HAND_SIZE = 20; // includes drawn tile (drawn tile is index 0)
const size_t DRAWN_I = MAX_HAND_SIZE-1; // drawn tile index
const size_t MAX_GROUPS = 4; // max groups of 3 or more
const size_t TILE_COUNT = 136; // number of tiles, also size of untouched wall
const size_t PLAYER_COUNT = 4; // number of players
const size_t DEAD_WALL_SIZE = 14; // size of dead wall
const size_t DORA_OFFSET = 4; // index of first uradora
const size_t MAX_DORA_INDICATORS = 4; // max number of top dora indicators
const int MANGAN_HAN = 5; // number of han constituting a mangan
const int YAKUMAN_HAN = 13; // number of han constituting a yakuman
const int DOUBLE_YAKUMAN_HAN = 999; // identifier for double yakuman (must be large enough to never appear naturally)

// regarding wall indexing, since wall is made of stacked pairs of tiles, bottom tile is the lower index
// so all bottom tiles are even indexes, alll top tiles are odd indexes

const std::array<TileType, 1 << 7> initDoraMap(); // initializes doraMap
const std::array<TileType, 1 << 7> DORA_MAP = initDoraMap(); // maps dora indicator to dora

struct Tile; struct Group; struct Hand; struct Player; struct Board;

extern const Tile GAME_TILES[TILE_COUNT]; // all game tiles (starting wall)

// singular tile
class Tile {
    TileType type; // is red | type (lower 6 bits)
public:
    Tile(TileType type = NONE, bool red = false) : type((((TileType)red) << 6) | type) {}
    TileType operator*() const; // get tile type (without red flag)
    bool isRed() const; // get whether tile is red
};

// groups of up to 4 tiles, essentially treated as an interval
class Group {
    int8_t tileIndices[4] = {-1,-1,-1,-1}; // indices of group tiles in hand
    int8_t _size;
    bool _open;
    bool _locked;
public:
    Group(int size = 3, bool open = false, bool locked = false) : _size(size), _open(open), _locked(locked) {}
    bool valid(const Hand& hand) const; // determines if meld is valid
    uint16_t mask() const; // return a bitmask with tile indices marked
    int8_t& operator[](int8_t index); // returns tile index at specified index in tileIndices
    int8_t size() const; // returns size of group
    bool open() const; // returns whether group is open
    bool locked() const; //returns whether group is locked
};

// hand state
struct Hand {
    Tile tiles[MAX_HAND_SIZE] = {}; // last tile is drawn tile, all initialized to none
    Group callMelds[MAX_GROUPS]; // melds from calls
    int8_t callMeldCount = 0; // number of call melds (number of elements in callMelds active)
    int8_t callTiles = 0; // number of tiles locked in calls, call tiles are at the start of the tiles array
    TileType operator[](size_t index) const; // get ith tile's type
    void swapDrawn(size_t index); // swap drawn tile with ith tile
    Tile discardDrawn(); // return drawn tile and discard it from hand
    bool operator==(const Hand& other) const; // check if hand equals another hand
    void clear(); // clears hand
};

// player state
struct Player {
    Hand hand;
    bool riichiActive = false; // in riichi state
    int score = 25000;
};

// board state
// wind values: 00 = east, 01 = south, 10 = west, 11 = north, matches last 2 bits on wind tile types
struct Board {
    Player players[PLAYER_COUNT]; // array of players (indexed by wind)
    Tile wall[TILE_COUNT]; // wall
    int drawIndex; // next tile in wall to draw from
    int revealedDora; // number of revealed dora
    int8_t roundWind; // round/prevalent wind
    int8_t seatWind; // seat wind
    int valueOfHand(int8_t playerIndex) const; // gets basic point value of a player's hand
    void initGame(); // reset to start of game
    void nextRound(); // sets up game to start of next round
    TileType getDora(int index, bool ura) const; // get dora/uradora at specified index
    Board() { initGame(); }
};