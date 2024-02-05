#include "board.h"
#include <utility>
#include <vector>
#include <algorithm>
#include <iostream>
#include <deque>
#include <cstring>

// set of valid groups (can have multiple per hand, points is max value meld set)
class GroupSet {
    Group groups[MAX_GROUPS + 1];
    int8_t _size = 0;
public:
    inline Group& operator[](int8_t index) {
        return groups[index];
    }

    inline int8_t size() const {
        return _size;
    }

    inline void push(Group group) {
        groups[_size++] = group;
    }

    inline void pop() {
        --_size;
    }
};

inline TileType Tile::operator*() const { return type & (TileType)0b0111111; }
inline bool Tile::isRed() const { return (bool)(type & 0b1000000); }

inline TileType Hand::operator[](size_t index) const { return *tiles[index]; }
inline void Hand::swapDrawn(size_t index) { std::swap(tiles[index], tiles[DRAWN_I]); }
inline Tile Hand::discardDrawn() {
    Tile drawnTile;
    std::swap(drawnTile, tiles[DRAWN_I]);
    return drawnTile;
}
inline void Hand::clear() {
    for (int i = 0; i < MAX_HAND_SIZE; ++i)
        tiles[i] = NONE;
}

inline bool Group::valid(const Hand& hand) const {
    if (hand[tileIndices[0]] == NONE) return false;

    // handle sets
    bool valid = true;
    for (int i = 1; i < _size; ++i) valid &= hand[tileIndices[0]] == hand[tileIndices[i]];
    if (valid || _size != 3) return valid;

    // handle runs
    return (hand[tileIndices[0]] & 0b110000) && hand[tileIndices[1]] == hand[tileIndices[0]] + 1 && hand[tileIndices[2]] == hand[tileIndices[0]] + 2;
}

inline uint16_t Group::mask() const {
    uint16_t mask = 0;
    for (int i = 0; i < _size; ++i)
        mask |= 1 << tileIndices[i];
    return mask;
}

inline int8_t& Group::operator[](int8_t index) { return tileIndices[index]; }
inline int8_t Group::size() const { return _size; }
inline bool Group::open() const { return _open; }
inline bool Group::locked() const { return _locked; }

const std::array<TileType, 1 << 7> initDoraMap() {
    std::array<TileType, 1 << 7> doraMapRef;
    doraMapRef[NONE] = NONE;
    doraMapRef[DGNW] = DGNG; doraMapRef[DGNG] = DGNR; doraMapRef[DGNR] = DGNW;
    doraMapRef[WNDE] = WNDS; doraMapRef[WNDS] = WNDW; doraMapRef[WNDW] = WNDN; doraMapRef[WNDN] = WNDE;
    doraMapRef[PIN1] = PIN2; doraMapRef[PIN2] = PIN3; doraMapRef[PIN3] = PIN4; doraMapRef[PIN4] = PIN5; doraMapRef[PIN5] = PIN6; doraMapRef[PIN6] = PIN7; doraMapRef[PIN7] = PIN8; doraMapRef[PIN8] = PIN9; doraMapRef[PIN9] = PIN1;
    doraMapRef[SOU1] = SOU2; doraMapRef[SOU2] = SOU3; doraMapRef[SOU3] = SOU4; doraMapRef[SOU4] = SOU5; doraMapRef[SOU5] = SOU6; doraMapRef[SOU6] = SOU7; doraMapRef[SOU7] = SOU8; doraMapRef[SOU8] = SOU9; doraMapRef[SOU9] = SOU1;
    doraMapRef[MAN1] = MAN2; doraMapRef[MAN2] = MAN3; doraMapRef[MAN3] = MAN4; doraMapRef[MAN4] = MAN5; doraMapRef[MAN5] = MAN6; doraMapRef[MAN6] = MAN7; doraMapRef[MAN7] = MAN8; doraMapRef[MAN8] = MAN9; doraMapRef[MAN9] = MAN1;
    return doraMapRef;
}

// gets next tile in run
inline TileType nextInRun(TileType tileType) {
    return (tileType & 0b110000) && (tileType & 0b001111) != 9 ? tileType + 1 : NONE;
};

// returns whether tile is terminal
inline bool isSimple(TileType tileType) {
    return !(tileType & 0b110000) == 0 && (tileType & 0b001111) != 1 && (tileType & 0b001111) != 9;
}

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

// calculate basic points from hand and fu
int basicPoints(int han, int fu) {
    if (han >= DOUBLE_YAKUMAN_HAN) return 16000; // double yakuman
    if (han >= 13) return 8000; // yakuman
    if (han >= 11) return 6000; // sanbaiman
    if (han >= 8) return 4000; // baiman
    if (han >= 6) return 3000; // haneman
    if (han == 5 || (han == 4 && fu >= 40) || (han == 3 && fu >= 70)) return 2000; // mangan
    return (((fu * (1 << (2 + han))) + 99) / 100) * 100; // basic points
}

// add points from tile bonuses (dora, uradora, red fives, wind bonus) (yaku han and fu passed in via han and fu args)
int tilePoints(const Board& board, int8_t playerIndex, SortedHand& sortedHand, int han, int fu) {
    const Player& player = board.players[playerIndex];
    const Hand& hand = player.hand;
    
    // identify dora
    int doraTiles[MAX_DORA_INDICATORS << 1];
    int doraTilesCount = 0;
    for (int i = 0; i < board.revealedDora; ++i) {
        doraTiles[doraTilesCount++] = board.getDora(i, false);
        if (player.riichiActive) doraTiles[doraTilesCount++] = board.getDora(i, true);
    }

    // iterate through tiles
    for (int i = 0; i < sortedHand.size(); ++i) {
        const Tile& tile = hand.tiles[*sortedHand[i]];
        TileType tileType = *tile;

        // red
        han += tile.isRed();

        // dora
        for (int j = 0; j < doraTilesCount; ++j)
            han += tileType == doraTiles[j];
    }

    // return points
    return basicPoints(han, fu);
}

// add points from non-group yaku (group han and fu passed in via yakuHan and yakuFu args)
int yakuPoints(const Board& board, int8_t playerIndex, SortedHand& sortedHand, int yakuHan, const int fu) {
    // ready / riichi
    yakuHan += board.players[playerIndex].riichiActive;

    // TODO: all simples / tan'yao

    // TODO: common terminals / all terminals and honors / honro

    // TODO: half flush / common flush / hon'itsu

    // TODO: full flush / perfect flush / chin'itsu

    // all honors / tsuiso
    // all terminals / chinroto
    // all green / ryuiso
    bool allHonors = true;
    bool allTerminals = true;
    bool allGreen = true;
    for (int i = 0; i < sortedHand.size(); ++i) {
        TileType tileType = sortedHand[i].type;
        allHonors &= (tileType >> 4) == 0;
        allTerminals &= (tileType >> 4) != 0 && ((tileType & 0b1111) == 1 || (tileType & 0b1111) == 9);
        allGreen &= ((tileType >> 4) == 1 && (
                        (tileType & 0b1111) == 2 || (tileType & 0b1111) == 3 || (tileType & 0b1111) == 4 || (tileType & 0b1111) == 6 || (tileType & 0b1111) == 8)
                    ) || tileType == DGNG;
    }
    yakuHan += allHonors ? YAKUMAN_HAN : 0;
    yakuHan += allTerminals ? YAKUMAN_HAN : 0;
    yakuHan += allGreen ? YAKUMAN_HAN : 0;

    // TODO: nagashi mangan

    // TODO: self pick / tsumo

    // TODO: one shot / ippatsu

    // TODO: last tile draw / under the sea

    // TODO: last tile claim / under the river

    // TODO: dead wall draw / after a quad / after a kan

    // TODO: robbing a quad / robbing a kan

    // TODO: double ready / double riichi

    // return points
    if (yakuHan == 0) return 0;
    return tilePoints(board, playerIndex, sortedHand, yakuHan, fu);
}

// group set points
// note: groupset not modified (need non-const becuase of [])
int groupSetPoints(const Board& board, int8_t playerIndex, SortedHand& sortedHand, GroupSet& groupSet) {
    const Player& player = board.players[playerIndex];
    const Hand& hand = player.hand;
    int yakuHan = 0;
    int fu = 20;

    // find pair
    int8_t pairIndex;
    for (pairIndex = 0; groupSet[pairIndex].size() != 2; ++pairIndex);

    // count fu from groups
    for (int i = 0; i < groupSet.size(); ++i) {
        Group& group = groupSet[i];
        if (sortedHand[group[0]].type != sortedHand[group[1]].type) continue;
        TileType tileType = group[0];
        if (group.size() == 2) {
            // pair fu
            fu += ((tileType & 0b111100) == 0b000100) && ((tileType & 0b000011) == board.roundWind) ? 2 : 0; // matches round wind
            fu += ((tileType & 0b111100) == 0b000100) && ((tileType & 0b000011) == board.seatWind) ? 2 : 0; // matches prevailing wind
            fu += (tileType & 0b111100) == 0b000000 ? 2 : 0; // dragon
            continue;
        }
        int fuPower = ((group.size() - 2) << 1) + !group.open() + !isSimple(tileType);
        fu += 1 << fuPower;

        // TODO: count fu from winning tile position in group
    }

    // TODO: count fu from waits (requires implementing waits)

    // TODO: count fu from tsumo

    // TODO: count fu from menzen ron (closed hand ron)

    // TODO: count fu form open pinfu

    // count suit runs and sets
    int runCounter[3][7];
    std::memset(runCounter, 0, sizeof(runCounter));
    for (int i = 0; i < groupSet.size(); ++i) {
        Group& group = groupSet[i];
        if (sortedHand[group[0]].type == sortedHand[group[1]].type) continue; // not a run
        ++runCounter[(sortedHand[group[0]].type >> 4) - 1][((sortedHand[group[0]].type) & 0b1111) - 1];
    }
    int suitSetCounter[3][9];
    std::memset(suitSetCounter, 0, sizeof(suitSetCounter));
    for (int i = 0; i < groupSet.size(); ++i) {
        Group& group = groupSet[i];
        if (group.size() != 3 || sortedHand[group[0]].type != sortedHand[group[1]].type || (sortedHand[group[0]].type & 0b110000) == 0b000000) continue; // not a suit set
        ++runCounter[(sortedHand[group[0]].type >> 4) - 1][((sortedHand[group[0]].type) & 0b1111) - 1];
    }

    // no-points hand / pinfu
    yakuHan += fu == 0;

    // twin sequences / ipeiko
    // double twin sequences / ryanpeiko
    if (hand.callMeldCount == 0) {
        int ipeikoCount = 0;
        for (int suit = 0; suit < 3; ++suit)
            for (int num = 0; num < 7; ++num)
                ipeikoCount += runCounter[suit][num] >= 2;
        yakuHan += (int)(ipeikoCount >= 2) + (int)(ipeikoCount >= 1);
    }

    // mixed sequences / sanshoku doujun
    {
        bool sanshoku = false;
        for (int num = 0; !sanshoku && num < 7; ++num)
            sanshoku = runCounter[0][num] && runCounter[1][num] && runCounter[2][num];
        yakuHan += sanshoku ? 1 + (hand.callMeldCount == 0) : 0;
    }

    // full straight / ikkitsuu
    {
        bool ikkitsuu = 0;
        for (int suit = 0; !ikkitsuu && suit < 3; ++suit)
            ikkitsuu = runCounter[suit][0] && runCounter[suit][3] && runCounter[suit][6];
        yakuHan += ikkitsuu ? 1 + (hand.callMeldCount == 0) : 0;
    }

    // all sets / toitoi
    {
        bool toitoi = true;
        for (int suit = 0; toitoi && suit < 3; ++suit)
            for (int num = 0; toitoi && num < 7; ++num)
                toitoi = runCounter[suit][num] == 0;
        yakuHan += toitoi ? 2 : 0;
    }

    // three concealed sets / san'anko
    {
        int concealedSets = 0;
        for (int i = 0; i < groupSet.size(); ++i)
            concealedSets += groupSet[i].size() >= 3 && !groupSet[i].open() && sortedHand[groupSet[i][0]].type == sortedHand[groupSet[i][1]].type;
        yakuHan += concealedSets == 3 ? 2 : 0;
    }

    // mixed triplets / sanshoku douko
    {
        bool sanshoku = false;
        for (int num = 0; !sanshoku && num < 9; ++num)
            sanshoku = suitSetCounter[0][num] && suitSetCounter[1][num] && suitSetCounter[2][num];
        yakuHan += sanshoku ? 2 : 0;
    }

    // three quads / sankatsu
    // four quads / sukantsu
    {
        int kanCount = 0;
        for (int i = 0; i < groupSet.size(); ++i)
            kanCount += groupSet[i].size() == 4;
        yakuHan += kanCount == 4 ? YAKUMAN_HAN : kanCount == 3 ? 2 : 0;
    }

    // honor tiles / yakuhai
    bool hasHonors = false;
    for (int i = 0; i < sortedHand.size(); ++i) {
        TileType tileType = sortedHand[i].type;
        hasHonors |= (tileType >> 4) == 0;
    }
    if (hasHonors) {
        int honorCount = 0;
        for (int i = 0; i < groupSet.size(); ++i) {
            if (groupSet[i].size() < 3 || groupSet[i].open() || sortedHand[groupSet[i][0]].type != sortedHand[groupSet[i][1]].type)
                continue;
            TileType tileType = sortedHand[groupSet[i][0]].type;
            honorCount += ((tileType & 0b111100) == 0b000100) && ((tileType & 0b000011) == board.roundWind); // matches round wind
            honorCount += ((tileType & 0b111100) == 0b000100) && ((tileType & 0b000011) == board.seatWind); // matches prevailing wind
            honorCount += tileType & 0b111100 == 0b000000; // dragon
        }
        yakuHan += honorCount;
    }

    // common ends / chanta
    // pefect ends / junchan
    {
        bool ends = true;
        for (int i = 0; ends && i < groupSet.size(); ++i) {
            TileType left = sortedHand[groupSet[i][0]].type;
            TileType right = sortedHand[groupSet[i][groupSet[i].size()-1]].type;
            ends = ((left >> 4) != 0 && (left & 0b001111) == 1) || ((right >> 4) != 0 && (right & 0b001111) == 9);
        }
        yakuHan += ends ? 1 + (hand.callMeldCount == 0) + !hasHonors : 0;
    }

    {
        int dragons[3] = {};
        int winds[4] = {};
        for (int i = 0; i < sortedHand.size(); ++i) {
            TileType tileType = sortedHand[i].type;
            if ((tileType & 0b111100) == 0b000000)
                ++dragons[tileType & 0b11];
            else if ((tileType & 0b111100) == 0b000100)
                ++winds[tileType & 0b11];
        }

        // little three dragons
        // big three dragons
        std::sort(dragons, dragons + 3);
        yakuHan += dragons[1] == 3 ? dragons[0] == 3 ? YAKUMAN_HAN : 2 : 0;

        // little winds / shosushi
        // big winds / daisushi
        std::sort(winds, winds + 4);
        yakuHan += winds[1] >= 3 ? winds[0] >= 3 ? DOUBLE_YAKUMAN_HAN : YAKUMAN_HAN : 0;
    }

    // TODO: four concealed triplets / suanko

    // TODO: nine gates / churen poto

    // TODO: blessing of heaven

    // TODO: blessing of earth

    // TODO: blessing of man

    // round fu up to nearest 10
    fu = ((fu + 9) / 10) * 10;
    
    // return points
    return yakuPoints(board, playerIndex, sortedHand, yakuHan, fu);
}

// generate all groups sets (sets of non-overlapping groups, call melds are locked)
void generateGroupSets(std::deque<GroupSet>& groupSets, const Board& board, int8_t playerIndex, SortedHand& sortedHand, const std::vector<Group>& groups, GroupSet& groupSet, int groupIndex, uint16_t usedTiles, bool pairHandled) {
    if (groupSet.size() == MAX_GROUPS + 1 && pairHandled) {
        groupSets.push_back(groupSet);
        return;
    }
    for (; groupIndex < groups.size(); ++groupIndex) {
        const Group& group = groups[groupIndex];
        uint16_t groupMask = group.mask();
        if ((usedTiles & groupMask) || (pairHandled && group.size() == 2)) continue;
        groupSet.push(group);
        generateGroupSets(groupSets, board, playerIndex, sortedHand, groups, groupSet, groupIndex+1, usedTiles | groupMask, pairHandled || group.size() == 2);
        groupSet.pop();
    }
}

// handles scoring hands that do not follow standard 4 groups 1 pair (7 pairs, 13 orphans)
int specialPoints(const Board& board, int8_t playerIndex, SortedHand& sortedHand) {
    if (board.players[playerIndex].hand.callMeldCount || sortedHand.size() != 14) return 0;
    
    // 7 pairs / chiitoitsu
    {
        int pairs;
        for (pairs = 0; pairs < 7 && sortedHand[pairs * 2].type == sortedHand[pairs * 2 + 1].type; ++pairs)
        if (pairs == 7) return yakuPoints(board, playerIndex, sortedHand, 2, 25);
    }

    // 13 orphans / kokushi musou
    // TODO: handle 13 wait variant
    {
        int pairs = 0;
        for (int i = 1; i < 14 && pairs <= 1; ++i) pairs += sortedHand[i-1].type == sortedHand[i].type;
        if (pairs == 1) return yakuPoints(board, playerIndex, sortedHand, YAKUMAN_HAN, 0);
    }

    return 0;
}

// find value of hand
int Board::valueOfHand(int8_t playerIndex) const {
    const Player& player = players[playerIndex];
    const Hand& hand = player.hand;
    std::vector<Group> groups; // set of all valid groups (may overlap)
    SortedHand sortedHand(hand);

    // TODO: check if no-tenpai due to discards

    // put all call melds into groups
    for (int i = 0; i < hand.callMeldCount; ++i) {
        groups.push_back(hand.callMelds[i]);
    }

    // find all valid set groups
    auto findSets = [&](int8_t setSize) {
        for (int i = hand.callTiles; i < sortedHand.size()-setSize; ++i) {
            if (sortedHand[i].type != sortedHand[i+setSize-1].type)
                continue;
            groups.emplace_back(setSize);
            for (int j = 0; j < setSize; ++j)
                groups.back()[j] = *sortedHand[i+j];
        }
    };
    findSets(2);
    findSets(3);
    findSets(4);

    // find all valid run groups
    for (int i = hand.callTiles; i < sortedHand.size()-3; ++i) {
        TileType middle = nextInRun(sortedHand[i].type);
        TileType last = nextInRun(middle);
        if (last == NONE) continue; // since nextInRun(NONE) = NONE, checking last is sufficient
        for (int j = sortedHand[i].nextTypeIndice; j < sortedHand.size() && sortedHand[j].type == middle; ++j) {
            for (int k = sortedHand[j].nextTypeIndice; k < sortedHand.size() && sortedHand[k].type == last; ++k) {
                groups.emplace_back(3, false);
                groups.back()[0] = *sortedHand[i];
                groups.back()[1] = *sortedHand[j];
                groups.back()[2] = *sortedHand[k];
            }
        }
    }

    // sort groups (should keep call melds at front)
    sort(groups.begin(), groups.end(), [](Group& a, Group& b) {
        return a[0] < b[0];
    });

    // debug print
    std::cout << "VALID GROUPS" << std::endl;
    for (int i = 0; i < groups.size(); ++i) {
        for (int j = 0; j < groups[i].size(); ++j)
            std::cout << (int)groups[i][j] << " ";
        std::cout << std::endl;
    }
    std::cout << groups.size() << std::endl;

    // setup base group set with call melds
    GroupSet groupSet;
    for (int i = 0; i < hand.callMeldCount; ++i)
        groupSet.push(groups[i]);

    // get all valid group sets
    std::deque<GroupSet> groupSets;
    generateGroupSets(groupSets, *this, playerIndex, sortedHand, groups, groupSet, groupSet.size(), 0, false);

    // debug print
    std::cout << "VALID GROUP SETS" << std::endl;
    for (GroupSet& groupSet : groupSets) {
        for (int i = 0; i < groupSet.size(); ++i) {
            std::cout << "(" << (int)groupSet[i][0];
            for (int j = 1; j < groupSet[i].size(); ++j)
                std::cout << "," << (int)groupSet[i][j];
            std::cout << ") ";
        }
        std::cout << std::endl;
    }
    
    // handle group-based yaku scoring
    int maxPoints = 0;
    for (GroupSet& groupSet : groupSets)
        maxPoints = std::max(maxPoints, groupSetPoints(*this, playerIndex, sortedHand, groupSet));

    // handle special yaku scoring
    maxPoints = std::max(maxPoints, specialPoints(*this, playerIndex, sortedHand));

    return maxPoints;
}

void Board::initGame() {
    nextRound();
    roundWind = 0;
    seatWind = 0;
}

void Board::nextRound() {
    roundWind += seatWind == 0b11;
    seatWind = (seatWind + 1) & 0b11;
    memcpy(wall, GAME_TILES, sizeof(GAME_TILES));
    std::random_shuffle(std::begin(wall), std::end(wall));
    for (int i = 0; i < PLAYER_COUNT; ++i)
        players[i].hand.clear();
}

inline TileType Board::getDora(int index, bool ura) const {
    return *wall[DORA_OFFSET + (index << 1) | ura];
}

const Tile GAME_TILES[TILE_COUNT] = {
    Tile(DGNW), Tile(DGNW), Tile(DGNW), Tile(DGNW),
    Tile(DGNG), Tile(DGNG), Tile(DGNG), Tile(DGNG),
    Tile(DGNR), Tile(DGNR), Tile(DGNR), Tile(DGNR),

    Tile(WNDE), Tile(WNDE), Tile(WNDE), Tile(WNDE),
    Tile(WNDS), Tile(WNDS), Tile(WNDS), Tile(WNDS),
    Tile(WNDW), Tile(WNDW), Tile(WNDW), Tile(WNDW),
    Tile(WNDN), Tile(WNDN), Tile(WNDN), Tile(WNDN),
    
    Tile(PIN1), Tile(PIN1), Tile(PIN1), Tile(PIN1),
    Tile(PIN2), Tile(PIN2), Tile(PIN2), Tile(PIN2),
    Tile(PIN3), Tile(PIN3), Tile(PIN3), Tile(PIN3),
    Tile(PIN4), Tile(PIN4), Tile(PIN4), Tile(PIN4),
    Tile(PIN5), Tile(PIN5), Tile(PIN5), Tile(PIN5, true),
    Tile(PIN6), Tile(PIN6), Tile(PIN6), Tile(PIN6),
    Tile(PIN7), Tile(PIN7), Tile(PIN7), Tile(PIN7),
    Tile(PIN8), Tile(PIN8), Tile(PIN8), Tile(PIN8),
    Tile(PIN9), Tile(PIN9), Tile(PIN9), Tile(PIN9),

    Tile(SOU1), Tile(SOU1), Tile(SOU1), Tile(SOU1),
    Tile(SOU2), Tile(SOU2), Tile(SOU2), Tile(SOU2),
    Tile(SOU3), Tile(SOU3), Tile(SOU3), Tile(SOU3),
    Tile(SOU4), Tile(SOU4), Tile(SOU4), Tile(SOU4),
    Tile(SOU5), Tile(SOU5), Tile(SOU5), Tile(SOU5, true),
    Tile(SOU6), Tile(SOU6), Tile(SOU6), Tile(SOU6),
    Tile(SOU7), Tile(SOU7), Tile(SOU7), Tile(SOU7),
    Tile(SOU8), Tile(SOU8), Tile(SOU8), Tile(SOU8),
    Tile(SOU9), Tile(SOU9), Tile(SOU9), Tile(SOU9),

    Tile(MAN1), Tile(MAN1), Tile(MAN1), Tile(MAN1),
    Tile(MAN2), Tile(MAN2), Tile(MAN2), Tile(MAN2),
    Tile(MAN3), Tile(MAN3), Tile(MAN3), Tile(MAN3),
    Tile(MAN4), Tile(MAN4), Tile(MAN4), Tile(MAN4),
    Tile(MAN5), Tile(MAN5), Tile(MAN5), Tile(MAN5, true),
    Tile(MAN6), Tile(MAN6), Tile(MAN6), Tile(MAN6),
    Tile(MAN7), Tile(MAN7), Tile(MAN7), Tile(MAN7),
    Tile(MAN8), Tile(MAN8), Tile(MAN8), Tile(MAN8),
    Tile(MAN9), Tile(MAN9), Tile(MAN9), Tile(MAN9),
};