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
#include <algorithm>
#include <vector>

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
public:
    enum Action { Init, Drawn, Discard };
private:
    TileType type; // is red | type (lower 6 bits)
    int lastActionTurn = 0;
    Action lastAction = Init;
public:
    Tile(TileType type = NONE, bool red = false) : type((((TileType)red) << 6) | type) {}
    TileType operator*() const; // get tile type (without red flag)
    bool isRed() const; // get whether tile is red
    void setLastAction(Action action, int turn); // set last action
    Action getLastAction() const; // get last action
    int getLastActionTurn() const; // get last action turn
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

// wait
struct Wait {
    TileType tileType = NONE;
    uint8_t fu = 0;
};

// hand state
struct Hand {
    Tile tiles[MAX_HAND_SIZE] = {}; // last tile is drawn tile, all initialized to none
    Group callMelds[MAX_GROUPS]; // melds from calls
    int8_t callMeldCount = 0; // number of call melds (number of elements in callMelds active)
    int8_t callTiles = 0; // number of tiles locked in calls, call tiles are at the start of the tiles array
    Wait waits[TILE_COUNT]; // waits in hand
    int waitCount = 0; // number of waits in hand
    TileType operator[](size_t index) const; // get ith tile's type
    void swapDrawn(size_t index); // swap drawn tile with ith tile
    Tile discardDrawn(); // return drawn tile and discard it from hand
    bool operator==(const Hand& other) const; // check if hand equals another hand
    void clear(); // clears hand
    void updateWaits(const Player& player); // update wait tiles
};

// player state
struct Player {
    Hand hand;
    Tile discards[TILE_COUNT];
    int discardCount;
    int riichiTurn = 0; // turn riichi was called, 0 if not called
    int score = 25000;
    int lastTurn = 0;
    int firstTurn = 0;
    bool ronActive = false;
    void initRound();
};

// board state
// wind values: 00 = east, 01 = south, 10 = west, 11 = north, matches last 2 bits on wind tile types
struct Board {
    enum DrawAction { natural, pon, chi, kan };
    Player players[PLAYER_COUNT]; // array of players (indexed by wind)
    Tile wall[TILE_COUNT]; // wall
    int drawIndex; // next tile in wall to draw from
    int revealedDora; // number of revealed dora
    int turn; // current turn starting at 1
    int lastCallTurn; // turn of last call 0 if none
    int8_t lastDiscardPlayer; // player who discarded last
    DrawAction lastDrawAction; // last draw action
    int8_t roundWind; // round/prevalent wind
    int8_t seatWind; // seat wind
    Board() { initGame(); }
    int valueOfHand(int8_t playerIndex) const; // gets basic point value of a player's hand
    void initGame(); // reset to start of game
    void nextRound(); // sets up game to start of next round
    TileType getDora(int index, bool ura) const; // get dora/uradora at specified index
    void drawTile(int8_t playerIndex, DrawAction drawActionType); // draw tile from wall
};

// sorted permutation of hand (empty tiles at the end)
// used for translation between sorted hand and original hand (similar to virtual addressing)
class SortedHand {
public:
    // element of sorted hand
    struct SortedTile {
        TileType type; // tile type
        int originalIndex; // original unsorted index
        int nextTypeIndice; // index of next element in sortedHand with different type (used for finding runs)
        SortedTile() {}
        SortedTile(TileType type, int originalIndex) : type(type), originalIndex(originalIndex) {}
        inline int operator*() const { return originalIndex; }
    };
private:
    int8_t _size;
    SortedTile tiles[MAX_HAND_SIZE];
public:
    SortedHand(const Hand& hand) {
        // copy non-call and non-empty tiles into hand
        _size = MAX_HAND_SIZE - hand.callTiles;
        int j = 0;
        for (int i = hand.callTiles; i < MAX_HAND_SIZE; ++i) {
            TileType type = *hand.tiles[i];
            tiles[type == NONE ? --_size : j++] = SortedTile(type, i);
        }

        // sort tiles
        std::sort(tiles, tiles + _size, [](SortedTile& a, SortedTile& b) {
            return a.type < b.type;
        });

        // for non-call tiles define nextTypeIndice
        tiles[_size-1].nextTypeIndice = _size;
        for (int i = _size-2; i >= 0; --i)
            tiles[i].nextTypeIndice = tiles[i].type == tiles[i+1].type ? tiles[i+1].nextTypeIndice : i+1;
    }

    inline SortedTile& operator[](int8_t index) {
        return tiles[index];
    }

    inline int8_t size() const {
        return _size;
    }
};

enum Yaku {
    Riichi,
    DoubleRiichi,
    AllSimples,
    SevenPairs,
    NagashiMangan,
    Tsumo,
    Ippatsu,
    UnderTheSea,
    UnderTheRiver,
    DeadWallDraw,
    RobbingAKan,
    Pinfu,
    TwinSequences,
    MixedSequences,
    FullStraight,
    DoubleTwinSequences,
    AllTriplets,
    ThreeConcealedTriplets,
    FourConcealedTriplets,
    ThreeMixedTriplets,
    ThreeKan,
    FourKan,
    HonorTiles,
    CommonEnds,
    PerfectEnds,
    CommonTerminals,
    LittleThreeDragons,
    BigThreeDragons,
    LittleFourWinds,
    BigFourWinds,
    HalfFlush,
    FullFlush,
    ThirteenOrphans,
    AllHonors,
    AllTerminals,
    AllGreen,
    NineGates,
    BlessingOfHeaven,
    BlessingOfEarth,
    BlessingOfMan,
};

struct ScoreInfo {
    std::vector<std::pair<Yaku, int>> yakuHan; // pair of yaku and its han value (closed variants may have different han)
    int doraCount = 0;
    int uradoraCount = 0;
    int redDoraCount = 0;
    int han = 0;
    int fu = 0;
    inline void clear(); // clears score
    inline void addYaku(Yaku yaku, int han); // adds yaku
    inline void addDora(); // adds 1 dora
    inline void addUradora(); // adds 1 uradora
    inline void addRedDora(); // adds 1 red dora
    inline int totalDora(); // gets total dora
};