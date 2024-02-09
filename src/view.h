#pragma once

#include <SFML/Graphics.hpp>
#include "board.h"

struct View {
    Board& board;
    sf::RenderWindow window;

    View(Board& board) : board(board) {}

    static void init(); // initializes shared resources (such as textures)

    void open(unsigned int width, unsigned int height, unsigned int fps); // opens display window

    void draw(); // draws window
};