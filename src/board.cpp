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
inline void Tile::setLastAction(Action action, int turn) {
    lastAction = action;
    lastActionTurn = turn;
}
inline Tile::Action Tile::getLastAction() const { return lastAction; }
inline int Tile::getLastActionTurn() const { return lastActionTurn; }


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
    callMeldCount = 0;
    callTiles = 0;
    waitCount = 0;
}

void Hand::updateWaits(const Player& player) {
    // TODO
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

inline uint32_t Group::mask() const {
    uint32_t mask = 0;
    for (int i = 0; i < _size; ++i)
        mask |= 1 << tileIndices[i];
    return mask;
}

inline int8_t& Group::operator[](int8_t index) { return tileIndices[index]; }
inline int8_t Group::size() const { return _size; }
inline bool Group::open() const { return _open; }
inline bool Group::locked() const { return _locked; }

const std::array<TileType, 1 << 7> initDoraMap() {
    std::array<TileType, 1 << 7> doraMap;
    doraMap[NONE] = NONE;
    doraMap[DGNW] = DGNG; doraMap[DGNG] = DGNR; doraMap[DGNR] = DGNW;
    doraMap[WNDE] = WNDS; doraMap[WNDS] = WNDW; doraMap[WNDW] = WNDN; doraMap[WNDN] = WNDE;
    doraMap[PIN1] = PIN2; doraMap[PIN2] = PIN3; doraMap[PIN3] = PIN4; doraMap[PIN4] = PIN5; doraMap[PIN5] = PIN6; doraMap[PIN6] = PIN7; doraMap[PIN7] = PIN8; doraMap[PIN8] = PIN9; doraMap[PIN9] = PIN1;
    doraMap[SOU1] = SOU2; doraMap[SOU2] = SOU3; doraMap[SOU3] = SOU4; doraMap[SOU4] = SOU5; doraMap[SOU5] = SOU6; doraMap[SOU6] = SOU7; doraMap[SOU7] = SOU8; doraMap[SOU8] = SOU9; doraMap[SOU9] = SOU1;
    doraMap[MAN1] = MAN2; doraMap[MAN2] = MAN3; doraMap[MAN3] = MAN4; doraMap[MAN4] = MAN5; doraMap[MAN5] = MAN6; doraMap[MAN6] = MAN7; doraMap[MAN7] = MAN8; doraMap[MAN8] = MAN9; doraMap[MAN9] = MAN1;
    return doraMap;
}

void Player::initRound() {
    discardCount = 0;
    hand.clear();
    lastTurn = 0;
    firstTurn = 0;
    ronActive = false;
}

// gets next tile in run
inline TileType nextInRun(TileType tileType) {
    return (tileType & 0b110000) && (tileType & 0b001111) != 9 ? tileType + 1 : NONE;
}

// gets prev tile in run
inline TileType prevInRun(TileType tileType) {
    return (tileType & 0b110000) && (tileType & 0b001111) != 1 ? tileType - 1 : NONE;
}

// returns whether tile is terminal
inline bool isSimple(TileType tileType) {
    return !(tileType & 0b110000) == 0 && (tileType & 0b001111) != 1 && (tileType & 0b001111) != 9;
}

// calculate basic points from han and fu
int ScoreInfo::basicPoints() {
    if (han == 0) return 0;
    if (han >= DOUBLE_YAKUMAN_HAN) return 16000; // double yakuman
    if (han >= 13) return 8000; // yakuman
    if (han >= 11) return 6000; // sanbaiman
    if (han >= 8) return 4000; // baiman
    if (han >= 6) return 3000; // haneman
    if (han == 5 || (han == 4 && fu >= 40) || (han == 3 && fu >= 70)) return 2000; // mangan
    return (((fu * (1 << (2 + han))) + 99) / 100) * 100; // basic points
}

// add points from tile bonuses (dora, uradora, red fives, wind bonus) (yaku han and fu passed in via han and fu args)
void tilePoints(const Board& board, int8_t playerIndex, SortedHand& sortedHand, ScoreInfo& scoreInfo) {
    const Player& player = board.players[playerIndex];
    const Hand& hand = player.hand;
    
    // identify dora
    int doraTiles[MAX_DORA_INDICATORS << 1];
    int doraTilesCount = 0;
    for (int i = 0; i < board.revealedDora; ++i) {
        doraTiles[doraTilesCount++] = board.getDora(i, false);
        if (player.riichiTurn) doraTiles[doraTilesCount++] = (1 << 16) | board.getDora(i, true);
    }

    // iterate through tiles
    for (int i = 0; i < sortedHand.size(); ++i) {
        const Tile& tile = hand.tiles[*sortedHand[i]];
        TileType tileType = *tile;

        // red
        if (tile.isRed()) scoreInfo.addRedDora();

        // dora
        for (int j = 0; j < doraTilesCount; ++j)
            if (tileType == doraTiles[j] & (0xffff))
                doraTiles[j] & (1 << 16) ? scoreInfo.addUradora() : scoreInfo.addDora();
    }
}

// add points from non-group yaku (group han and fu passed in via yakuHan and yakuFu args)
// called with special hands such as 13 orphans and 7 pairs
void yakuPoints(const Board& board, int8_t playerIndex, SortedHand& sortedHand, ScoreInfo& scoreInfo) {
    const Player& player = board.players[playerIndex];
    const Hand& hand = player.hand;

    // ready / riichi
    // double ready / double riichi
    if (player.riichiTurn != 0) {
        if (player.riichiTurn == player.firstTurn && board.lastCallTurn < player.firstTurn)
            scoreInfo.addYaku(DoubleRiichi, 2);
        else
            scoreInfo.addYaku(Riichi, 1);
    }

    // all simples / tan'yao
    {
        bool tanyao = true;
        for (int i = 0; tanyao && i < sortedHand.size(); ++i) {
            TileType tileType = sortedHand[i].type;
            tanyao = (tileType & 0b110000) != 0 && (tileType & 0b001111) != 1 && (tileType & 0b001111) != 9;
        }
        if (tanyao) scoreInfo.addYaku(AllSimples, 1);
    }

    // TODO: half flush / common flush / hon'itsu

    // TODO: full flush / perfect flush / chin'itsu

    // common terminals / all terminals and honors / honro
    // all honors / tsuiso
    // all terminals / chinroto
    // all green / ryuiso
    {
        bool commonTerminals = true;
        bool allHonors = true;
        bool allTerminals = true;
        bool allGreen = true;
        for (int i = 0; i < sortedHand.size(); ++i) {
            TileType tileType = sortedHand[i].type;
            bool isHonor = (tileType >> 4) == 0;
            bool isTerminal = (tileType >> 4) != 0 && ((tileType & 0b1111) == 1 || (tileType & 0b1111) == 9);
            commonTerminals &= isHonor | isTerminal;
            allHonors &= isHonor;
            allTerminals &= isTerminal;
            allGreen &= ((tileType >> 4) == 1 && (
                            (tileType & 0b1111) == 2 || (tileType & 0b1111) == 3 || (tileType & 0b1111) == 4 || (tileType & 0b1111) == 6 || (tileType & 0b1111) == 8)
                        ) || tileType == DGNG;
        }
        if (commonTerminals) scoreInfo.addYaku(CommonTerminals, 2);
        if (allHonors) scoreInfo.addYaku(AllHonors, YAKUMAN_HAN);
        if (allTerminals) scoreInfo.addYaku(AllTerminals, YAKUMAN_HAN);
        if (allGreen) scoreInfo.addYaku(AllGreen, YAKUMAN_HAN);
    }

    // TODO: nagashi mangan

    // one shot / ippatsu
    if (player.lastTurn == player.riichiTurn && board.lastCallTurn < player.riichiTurn)
        scoreInfo.addYaku(Ippatsu, 1);

    // TODO: last tile draw / under the sea

    // TODO: last tile claim / under the river

    // TODO: dead wall draw / after a quad / after a kan

    // TODO: robbing a quad / robbing a kan

    // return if hand incomplete
    if (scoreInfo.han == 0) return;

    // self pick / tsumo
    if (!player.ronActive && hand.callMeldCount)
        scoreInfo.addYaku(Tsumo, 1);

    tilePoints(board, playerIndex, sortedHand, scoreInfo);
}

// group set points
// note: groupset not modified (need non-const becuase of [])
void groupSetPoints(const Board& board, int8_t playerIndex, SortedHand& sortedHand, ScoreInfo& scoreInfo, GroupSet& groupSet) {
    const Player& player = board.players[playerIndex];
    const Hand& hand = player.hand;
    int& fu = scoreInfo.fu;
    fu = player.ronActive ? 30 : 20;

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
    }

    // count fu from waits
    for (int i = 0; i < hand.waitCount; ++i)
        fu += hand.waits[i].fu;

    // count fu from closed hand ron / menzen-kafu
    fu += hand.callMeldCount == 0 && player.ronActive ? 10 : 0;

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

    // count fu from tsumo
    fu += !player.ronActive && hand.callMeldCount == 0 && fu > 20 ? 2 : 0;

    // open pinfu / kui-pinfu
    fu += fu == 20 && hand.callMeldCount != 0 ? 2 : 0;

    // no-points hand / pinfu
    if (fu == 20 && hand.callMeldCount == 0)
        scoreInfo.addYaku(Pinfu, 1);

    // twin sequences / ipeiko
    // double twin sequences / ryanpeiko
    if (hand.callMeldCount == 0) {
        int ipeikoCount = 0;
        for (int suit = 0; suit < 3; ++suit)
            for (int num = 0; num < 7; ++num)
                ipeikoCount += runCounter[suit][num] >= 2;
        if (ipeikoCount >= 2) scoreInfo.addYaku(DoubleTwinSequences, 2);
        else if (ipeikoCount == 1) scoreInfo.addYaku(TwinSequences, 1);
    }

    // mixed sequences / sanshoku doujun
    {
        bool sanshoku = false;
        for (int num = 0; !sanshoku && num < 7; ++num)
            sanshoku = runCounter[0][num] && runCounter[1][num] && runCounter[2][num];
        if (sanshoku) scoreInfo.addYaku(MixedSequences, 1 + (hand.callMeldCount == 0));
    }

    // full straight / ikkitsuu
    {
        bool ikkitsuu = 0;
        for (int suit = 0; !ikkitsuu && suit < 3; ++suit)
            ikkitsuu = runCounter[suit][0] && runCounter[suit][3] && runCounter[suit][6];
        if (ikkitsuu) scoreInfo.addYaku(FullStraight, 1 + (hand.callMeldCount == 0));
    }

    // all triplets / toitoi
    {
        bool toitoi = true;
        for (int suit = 0; toitoi && suit < 3; ++suit)
            for (int num = 0; toitoi && num < 7; ++num)
                toitoi = runCounter[suit][num] == 0;
        if (toitoi) scoreInfo.addYaku(AllTriplets, 2);
    }

    // three concealed triplets / san'anko
    // four concealed triplets / suanko
    {
        int concealedSets = 0;
        for (int i = 0; i < groupSet.size(); ++i)
            concealedSets += groupSet[i].size() >= 3 && !groupSet[i].open() && sortedHand[groupSet[i][0]].type == sortedHand[groupSet[i][1]].type;
        if (concealedSets >= 4) scoreInfo.addYaku(FourConcealedTriplets, YAKUMAN_HAN);
        else if (concealedSets == 3) scoreInfo.addYaku(ThreeConcealedTriplets, 2);
    }

    // mixed triplets / sanshoku douko
    {
        bool sanshoku = false;
        for (int num = 0; !sanshoku && num < 9; ++num)
            sanshoku = suitSetCounter[0][num] && suitSetCounter[1][num] && suitSetCounter[2][num];
        if (sanshoku) scoreInfo.addYaku(ThreeMixedTriplets, 2);
    }

    // three quads / sankatsu
    // four quads / sukantsu
    {
        int kanCount = 0;
        for (int i = 0; i < groupSet.size(); ++i)
            kanCount += groupSet[i].size() == 4;
        if (kanCount == 4) scoreInfo.addYaku(FourKan, YAKUMAN_HAN);
        else if (kanCount == 3) scoreInfo.addYaku(ThreeKan, 2);
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
        if (honorCount) scoreInfo.addYaku(HonorTiles, honorCount);
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
        if (ends) {
            if (hasHonors) scoreInfo.addYaku(CommonEnds, 1 + (hand.callMeldCount == 0));
            else scoreInfo.addYaku(PerfectEnds, 2 + (hand.callMeldCount == 0));
        }
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
        if (dragons[1] >= 3) {
            if (dragons[0] >= 3) scoreInfo.addYaku(BigThreeDragons, YAKUMAN_HAN);
            else if (dragons[0] == 2) scoreInfo.addYaku(LittleThreeDragons, 2);
        }

        // little winds / shosushi
        // big winds / daisushi
        std::sort(winds, winds + 4);
        if (winds[1] >= 3) {
            if (winds[0] >= 3) scoreInfo.addYaku(BigFourWinds, DOUBLE_YAKUMAN_HAN);
            else if (winds[0] == 2) scoreInfo.addYaku(LittleFourWinds, YAKUMAN_HAN);
        }
    }

    // TODO: nine gates / churen poto

    // TODO: blessing of heaven

    // TODO: blessing of earth

    // TODO: blessing of man

    // round fu up to nearest 10
    fu = ((fu + 9) / 10) * 10;
    
    yakuPoints(board, playerIndex, sortedHand, scoreInfo);
}

// generate all groups sets (sets of non-overlapping groups, call melds are locked)
void generateGroupSets(std::deque<GroupSet>& groupSets, const Board& board, int8_t playerIndex, SortedHand& sortedHand, const std::vector<Group>& groups, GroupSet& groupSet, int groupIndex, uint32_t usedTiles, bool pairHandled) {
    if (groupSet.size() == MAX_GROUPS + 1 && pairHandled) {
        groupSets.push_back(groupSet);
        return;
    }
    for (; groupIndex < groups.size(); ++groupIndex) {
        const Group& group = groups[groupIndex];
        uint32_t groupMask = group.mask();
        if ((usedTiles & groupMask) || (pairHandled && group.size() == 2)) continue;
        groupSet.push(group);
        generateGroupSets(groupSets, board, playerIndex, sortedHand, groups, groupSet, groupIndex+1, usedTiles | groupMask, pairHandled || group.size() == 2);
        groupSet.pop();
    }
}

// handles scoring hands that do not follow standard 4 groups 1 pair (7 pairs, 13 orphans)
void specialPoints(const Board& board, int8_t playerIndex, SortedHand& sortedHand, ScoreInfo& scoreInfo) {
    if (board.players[playerIndex].hand.callMeldCount || sortedHand.size() != 14) return;
    
    // 7 pairs / chiitoitsu
    {
        int pairs;
        for (pairs = 0; pairs < 7 && sortedHand[pairs * 2].type == sortedHand[pairs * 2 + 1].type; ++pairs)
        if (pairs == 7) {
            scoreInfo.addYaku(SevenPairs, 2);
            scoreInfo.fu = 25;
            yakuPoints(board, playerIndex, sortedHand, scoreInfo);
            return;
        }
    }

    // 13 orphans / kokushi musou
    // TODO: handle 13 wait variant
    {
        int pairs = 0;
        for (int i = 1; i < 14 && pairs <= 1; ++i) pairs += sortedHand[i-1].type == sortedHand[i].type;
        if (pairs <= 1) {
            scoreInfo.addYaku(ThirteenOrphans, YAKUMAN_HAN);
            yakuPoints(board, playerIndex, sortedHand, scoreInfo);
            return;
        }
    }
}

// find value of hand
ScoreInfo Board::valueOfHand(int8_t playerIndex) const {
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
        for (int i = hand.callTiles; i <= sortedHand.size()-setSize; ++i) {
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
    for (int i = hand.callTiles; i <= sortedHand.size()-3; ++i) {
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
        int cap = std::min(a.size(), b.size());
        for (int i = 0; i < cap; ++i)
            if (a[i] != b[i]) return a[i] < b[i];
        return a.size() < b.size();
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
    ScoreInfo maxScoreInfo;
    static ScoreInfo currScoreInfo;
    for (GroupSet& groupSet : groupSets) {
        currScoreInfo.clear();
        groupSetPoints(*this, playerIndex, sortedHand, currScoreInfo, groupSet);
        int points = currScoreInfo.basicPoints();
        if (points > maxPoints) {
            maxPoints = points;
            maxScoreInfo = currScoreInfo;
        }
    }

    // handle special yaku scoring
    currScoreInfo.clear();
    specialPoints(*this, playerIndex, sortedHand, currScoreInfo);
    int points = currScoreInfo.basicPoints();
    if (points > maxPoints) {
        maxPoints = points;
        maxScoreInfo = currScoreInfo;
    }

    return maxScoreInfo;
}

void Board::initGame() {
    nextRound();
    roundWind = 0;
    seatWind = 0;
}

void Board::nextRound() {
    roundWind += seatWind == 0b11;
    seatWind = (seatWind + 1) & 0b11;
    turn = 1;
    lastCallTurn = 0;
    lastDrawAction = natural;
    lastDiscardPlayer = -1;
    revealedDora = 1;
    memcpy(wall, GAME_TILES, sizeof(GAME_TILES));
    std::random_shuffle(std::begin(wall), std::end(wall));
    for (int i = 0; i < PLAYER_COUNT; ++i)
        players[i].initRound();
}

inline TileType Board::getDora(int index, bool ura) const {
    return *wall[DORA_OFFSET + (index << 1) | ura];
}

inline void Board::drawTile(int8_t playerIndex, DrawAction drawAction) {
    Tile tile = wall[drawIndex--];
    tile.setLastAction(Tile::Action::Drawn, turn);
    Player& player = players[playerIndex];
    player.hand.tiles[DRAWN_I] = tile;
    lastDrawAction = drawAction;
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

void ScoreInfo::clear() {
    yakuHan.clear();
    doraCount = 0;
    uradoraCount = 0;
    redDoraCount = 0;
    han = 0;
    fu = 0;
}

inline void ScoreInfo::addYaku(Yaku yaku, int hanValue) {
    yakuHan.emplace_back(yaku, hanValue);
    han += hanValue;
}

inline void ScoreInfo::addDora() {
    ++doraCount;
    ++han;
}

inline void ScoreInfo::addUradora() {
    ++uradoraCount;
    ++han;
}

inline void ScoreInfo::addRedDora() {
    ++redDoraCount;
    ++han;
}

inline int ScoreInfo::totalDora() {
    return doraCount + uradoraCount + redDoraCount;
}

const std::array<YakuInfo, 40> initYakuInfoMap() {
    std::array<YakuInfo, 40> yakuInfoMap;
    yakuInfoMap[Riichi] = YakuInfo("Riichi");
    yakuInfoMap[DoubleRiichi] = YakuInfo("Double Riichi");
    yakuInfoMap[AllSimples] = YakuInfo("All Simples");
    yakuInfoMap[SevenPairs] = YakuInfo("Seven Pairs");
    yakuInfoMap[NagashiMangan] = YakuInfo("Nagashi Mangan");
    yakuInfoMap[Tsumo] = YakuInfo("Tsumo");
    yakuInfoMap[Ippatsu] = YakuInfo("Ippatsu");
    yakuInfoMap[UnderTheSea] = YakuInfo("Under the Sea");
    yakuInfoMap[UnderTheRiver] = YakuInfo("Under the River");
    yakuInfoMap[DeadWallDraw] = YakuInfo("Dead Wall Draw");
    yakuInfoMap[RobbingAKan] = YakuInfo("Robbing a Kan");
    yakuInfoMap[Pinfu] = YakuInfo("Pinfu");
    yakuInfoMap[TwinSequences] = YakuInfo("Twin Sequences");
    yakuInfoMap[MixedSequences] = YakuInfo("Mixed Sequences");
    yakuInfoMap[FullStraight] = YakuInfo("Full Straight");
    yakuInfoMap[DoubleTwinSequences] = YakuInfo("Double Twin Sequences");
    yakuInfoMap[AllTriplets] = YakuInfo("All Triplets");
    yakuInfoMap[ThreeConcealedTriplets] = YakuInfo("Three Concealed Triplets");
    yakuInfoMap[FourConcealedTriplets] = YakuInfo("Four Concealed Triplets");
    yakuInfoMap[ThreeMixedTriplets] = YakuInfo("Three Mixed Triplets");
    yakuInfoMap[ThreeKan] = YakuInfo("Three Kan");
    yakuInfoMap[FourKan] = YakuInfo("Four Kan");
    yakuInfoMap[HonorTiles] = YakuInfo("Honor Tiles");
    yakuInfoMap[CommonEnds] = YakuInfo("Common Ends");
    yakuInfoMap[PerfectEnds] = YakuInfo("Perfect Ends");
    yakuInfoMap[CommonTerminals] = YakuInfo("Common Terminals");
    yakuInfoMap[LittleThreeDragons] = YakuInfo("Little Three Dragons");
    yakuInfoMap[BigThreeDragons] = YakuInfo("Big Three Dragons");
    yakuInfoMap[LittleFourWinds] = YakuInfo("Little Four Winds");
    yakuInfoMap[BigFourWinds] = YakuInfo("Big Four Winds");
    yakuInfoMap[HalfFlush] = YakuInfo("Half Flush");
    yakuInfoMap[FullFlush] = YakuInfo("Full Flush");
    yakuInfoMap[ThirteenOrphans] = YakuInfo("Thirteen Orphans");
    yakuInfoMap[AllHonors] = YakuInfo("All Honors");
    yakuInfoMap[AllTerminals] = YakuInfo("All Terminals");
    yakuInfoMap[AllGreen] = YakuInfo("All Green");
    yakuInfoMap[NineGates] = YakuInfo("Nine Gates");
    yakuInfoMap[BlessingOfHeaven] = YakuInfo("Blessing of Heaven");
    yakuInfoMap[BlessingOfEarth] = YakuInfo("Blessing of Earth");
    yakuInfoMap[BlessingOfMan] = YakuInfo("Blessing of Man");
    return yakuInfoMap;
}