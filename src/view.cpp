#include "view.h"
#include "board.h"
#include <iostream>

inline void loadTexture(sf::Texture& texture, std::string path) {
    if (!texture.loadFromFile(path))
        throw("cannot load texture from path " + path);
}

sf::Texture mahjongTilesTex;
sf::Sprite mahjongSprite;
const int TILE_PIXEL_WIDTH = 32;
const int TILE_PIXEL_HEIGHT = 42;
const float TILE_SCALE = 2.f;
const float TILE_WIDTH = (float)TILE_PIXEL_WIDTH * TILE_SCALE;
const float TILE_HEIGHT = (float)TILE_PIXEL_HEIGHT * TILE_SCALE;
const size_t TILE_SHEET_SIZE = 39;
int tile_spritesheet_index[1 << 7] = {};
const uint8_t OPEN_TILE_TEX = 0b1111111;
const uint8_t CLOSED_TILE_TEX = 0b1111110;

inline void setMahjongSpriteTexture(TileType tileType) {
    sf::IntRect rect;
    rect.width = TILE_PIXEL_WIDTH;
    rect.height = TILE_PIXEL_HEIGHT;
    rect.left = TILE_PIXEL_WIDTH * tile_spritesheet_index[(size_t)tileType];
    mahjongSprite.setTextureRect(rect);
}

void View::init() {
    loadTexture(mahjongTilesTex, "resources/mahjong_tiles.png");
    mahjongSprite.setTexture(mahjongTilesTex);
    const uint8_t tile_sheet_order[TILE_SHEET_SIZE] = {
        OPEN_TILE_TEX, CLOSED_TILE_TEX,
        WNDE, WNDS, WNDW, WNDN, DGNW, DGNG, DGNR,
        PIN1, PIN2, PIN3, PIN4, PIN5, (1 << 7) | PIN5, PIN6, PIN7, PIN8, PIN9,
        SOU1, SOU2, SOU3, SOU4, SOU5, (1 << 7) | SOU5, SOU6, SOU7, SOU8, SOU9,
        MAN1, MAN2, MAN3, MAN4, MAN5, (1 << 7) | MAN5, MAN6, MAN7, MAN8, MAN9
    };
    for (int i = 0; i < TILE_SHEET_SIZE; ++i)
        tile_spritesheet_index[tile_sheet_order[i]] = i;
}

void View::open(unsigned int width, unsigned int height, unsigned int fps) {
    window.create(sf::VideoMode(width, height), "Reach/Riichi Mahjong Engine");
    window.setFramerateLimit(fps);
}

void drawHand(sf::RenderTarget& canvas, const Hand& hand) {
    SortedHand sortedHand(hand);
    mahjongSprite.setScale({TILE_SCALE, TILE_SCALE});
    float x = ((float)canvas.getSize().x - (float)sortedHand.size() * TILE_WIDTH) * 0.5f;
    float y = (float)canvas.getSize().y * 0.85f;
    for (int i = 0; i < sortedHand.size(); ++i) {
        mahjongSprite.setPosition({x + TILE_WIDTH * (float)i, y});
        setMahjongSpriteTexture(OPEN_TILE_TEX);
        canvas.draw(mahjongSprite);
        setMahjongSpriteTexture(sortedHand[i].type);
        canvas.draw(mahjongSprite);
    }
}

void View::draw() {
    window.clear(sf::Color(200,200,200));

    // draw hands
    drawHand(window, board.players[0].hand);

    // display call
    window.display();
}