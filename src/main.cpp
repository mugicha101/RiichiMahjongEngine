#include <SFML/Graphics.hpp>
#include <iostream>
#include "board.h"

int main()
{
    initDoraMap();

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
    int points = board.valueOfHand(0);
    std::cout << "points=" << points << std::endl;

    return 0;

    auto window = sf::RenderWindow{ { 1920u, 1080u }, "CMake SFML Project" };
    window.setFramerateLimit(144);
    while (window.isOpen()) {
        for (auto event = sf::Event{}; window.pollEvent(event);) {
            if (event.type == sf::Event::Closed) {
                window.close();
            }
        }

        window.clear();
        window.display();
    }
}