#include <SFML/Graphics.hpp>
#include <iostream>
#include "board.h"
#include "view.h"

int main()
{
    initDoraMap();
    View::init();

    Tile tiles[MAX_HAND_SIZE] = {
        Tile(SOU5),
        Tile(SOU5),
        Tile(SOU6),
        Tile(SOU6),
        Tile(SOU6),
        Tile(SOU7),
        Tile(SOU7),
        Tile(SOU7),
        Tile(SOU8),
        Tile(SOU8),
        Tile(SOU8),
        Tile(SOU9),
        Tile(SOU9),
        Tile(SOU9),
        Tile(NONE),
        Tile(NONE),
        Tile(NONE),
        Tile(NONE),
        Tile(NONE),
        Tile(NONE)
    };
    Board board;
    memcpy(board.players[0].hand.tiles, tiles, sizeof(tiles));
    board.players[0].riichiTurn = 1;
    ScoreInfo scoreInfo = board.valueOfHand(0);
    std::cout << "basic points=" << scoreInfo.basicPoints() << std::endl;
    for (auto& [yaku, han] : scoreInfo.yakuHan) {
        std::cout << YAKU_INFO_MAP[yaku].name << " " << han << std::endl;
    }

    View view(board);
    const float init_scale = 0.8f;
    view.open(sf::VideoMode::getDesktopMode().width * init_scale, sf::VideoMode::getDesktopMode().height * init_scale, 60);
    while (view.window.isOpen()) {
        for (auto event = sf::Event{}; view.window.pollEvent(event);) {
            if (event.type == sf::Event::Closed) {
                view.window.close();
            }
        }

        view.draw();
    }
}