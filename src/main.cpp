// Raycasting 3D adventure game, based on http://lodev.org/cgtutor/raycasting.html
// Inspired by seeing https://github.com/TheMozg/awk-raycaster
//
// Copyright Thomas van der Berg, 2016, some parts taken from aforementioned tutorial copyrighted by its author

#include <stdio.h>
#include <stdint.h>
#include <math.h>
#include <unordered_map>
#include <SFML/Graphics.hpp>

const int mapWidth = 24;
const int mapHeight = 24;

const int screenWidth = 1280;
const int screenHeight = 720;

// valid wall types and their colors for the world map
const std::unordered_map<char, sf::Color> wallTypes {
    {'#', sf::Color(0x80, 0x80, 0x80)},
    {'=', sf::Color(0x5E, 0x26, 0x12)},
    {'M', sf::Color(0x80, 0x00, 0xFF)},
    {'N', sf::Color(0x80, 0xFF, 0x00)},
    {'8', sf::Color(0x83, 0x7E, 0x7C)},
    {'!', sf::Color(0xFF, 0xFF, 0xFF)},
    {'^', sf::Color(0xFC, 0x15, 0x01)},
};

// top-down view of world map
const char worldMap[] =
    "########################"
    "#...#==================#"
    "#...#=....==..........=#"
    "#.........==.....=....=#"
    "#...#=....==.....=....=#"
    "#...#=....==.....=....=#"
    "#...#=======..======.==#"
    "#...########..######.###"
    "#......................#"
    "#...################...#"
    "#.............#....#...#"
    "#####.#######.#....#...#"
    "#MMMM.MMMMNNN.N.NN.#...#"
    "#M.......MNNN....N.#...#"
    "#M.8..8..MN......N.#...#"
    "#M.......MN......N.#...#"
    "#M.8..8..MNNNNNNNN.#...#"
    "#MMMMMMMMM.........#...#"
    "##########.#########...#"
    "#......................#"
    "#!!!!.!!!!!!!!!!!!!!!!!#"
    "#..........!...........#"
    "#......................^"
    "########################";

// return a tile from worldMap
char getTile(int x, int y) {
    return worldMap[y * mapWidth + x];
}

// checks worldMap for errors
// returns: true on success, false on errors found
bool mapCheck() {
    // check size
    int mapSize = sizeof(worldMap) - 1; // - 1 because sizeof also counts the final NULL character
    if (mapSize != mapWidth * mapHeight) {
        fprintf(stderr, "Map size(%d) is not mapWidth * mapHeight(%d)\n", mapSize, mapWidth * mapHeight);
        return false;
    }

    // check if edges are walls
    for (int y = 0; y < mapHeight; ++y) {
        for (int x = 0; x < mapWidth; ++x) {
            char tile = getTile(x, y);
            if (tile != '.' && wallTypes.find(tile) == wallTypes.end()) {
                fprintf(stderr, "map tile at [%3d,%3d] has an unknown tile type(%c)\n", x, y, tile);
                return false;
            }
            if ((y == 0 || x == 0 || y == mapHeight - 1 || x == mapWidth - 1) &&
                    tile == '.') {
                fprintf(stderr, "map edge at [%3d,%3d] is a floor (should be wall)\n", x, y);
                return false;
            }
        }
    }
    return true;
}

// rotate a given vector with given float value in radians and return the result
sf::Vector2f rotateVec(sf::Vector2f vec, float value) {
    return sf::Vector2f(
            vec.x * std::cos(value) - vec.y * std::sin(value),
            vec.x * std::sin(value) + vec.y * std::cos(value)
    );
}

int main() {

    // if the map is not correct, we can have segmentation faults. So check it.
    if (!mapCheck()) {
        fprintf(stderr, "Map is invalid!\n");
        return EXIT_FAILURE;
    }

    sf::Font font;
    if (!font.loadFromFile("data/font/opensans.ttf")) {
        fprintf(stderr, "Cannot open font!\n");
        return EXIT_FAILURE;
    }

    // player
    sf::Vector2f position(2.5f, 2.0f); // coordinates in worldMap
    sf::Vector2f direction(0.0f, 1.0f); // direction, relative to (0,0)
    sf::Vector2f plane(-0.66f, 0.0f); // 2d raycaster version of the camera plane,
                                     // must be perpendicular to rotation
    float moveSpeed = 5.0f; // player movement speed in tiles per second
    float rotateSpeed = 3.0f; // player rotation speed in radians per second

    // create window
    sf::RenderWindow window(sf::VideoMode(screenWidth, screenHeight), "Adventure 3D");
    window.setFramerateLimit(30); // video card will die without this limit
    bool hasFocus;

    // lines used to draw walls on the screen
    sf::VertexArray lines(sf::Lines, 2 * screenWidth);

    sf::Text fpsText("ERROR", font, 50); // text object for FPS counter, initialize with ERROR text
    sf::Clock clock; // timer
    char fpsString[sizeof("FPS: **.*")]; // string buffer to use for FPS counter

    while (window.isOpen()) {
        // calculate delta time
        int32_t dt_i = clock.restart().asMilliseconds(); // in miliseconds as int
        float dt = dt_i / 1000.0f; // in seconds as float

        // calculate FPS
        float fps = 1.0f / dt;
        snprintf(fpsString, sizeof(fpsString), "FPS: %2.1f", fps);
        fpsText.setString(fpsString);

        // handle SFML events
        sf::Event event;
        while (window.pollEvent(event)) {
            switch(event.type) {
            case sf::Event::Closed:
                window.close();
                break;
            case sf::Event::LostFocus:
                hasFocus = false;
                break;
            case sf::Event::GainedFocus:
                hasFocus = true;
                break;
            default:
                break;
            }
        }
        
        // handle keyboard input
        if (hasFocus) {
            using kb = sf::Keyboard;

            // moving forward or backwards (1.0 or -1.0)
            float moveForward = 0.0f;

            // handle input
            if (kb::isKeyPressed(kb::Up)) {
                moveForward = 1.0f;
            } else if (kb::isKeyPressed(kb::Down)) {
                moveForward = -1.0f;
            }

            // handle movement
            if (moveForward != 0.0f) {
                sf::Vector2f moveVec = direction * moveSpeed * moveForward * dt;

                if (getTile(int(position.x + moveVec.x), int(position.y)) == '.') {
                    position.x += moveVec.x;
                }
                if (getTile(int(position.x), int(position.y + moveVec.y)) == '.') {
                    position.y += moveVec.y;
                }
            }

            // rotating rightwards or leftwards(1.0 or -1.0)
            float rotateDirection = 0.0f;

            // handle input
            if (kb::isKeyPressed(kb::Left)) {
                rotateDirection = -1.0f;
            } else if (kb::isKeyPressed(kb::Right)) {
                rotateDirection = 1.0f;
            }

            // handle rotation
            if (rotateDirection != 0.0f) {
                float rotation = rotateSpeed * rotateDirection * dt;
                direction = rotateVec(direction, rotation);
                plane = rotateVec(plane, rotation);
            }
        }

        // loop through vertical screen lines, draw a line of wall for each
        for (int x = 0; x < screenWidth; ++x) {

            // ray to emit
            float cameraX = 2 * x / (float)screenWidth - 1.0f; // x in camera space (between -1 and +1)
            sf::Vector2f rayPos = position;
            sf::Vector2f rayDir = direction + plane * cameraX;

            if (rayDir.x == 0 || rayDir.y == 0) {
                continue;
            }

            // calculate distance traversed between each grid line for x and y based on direction
            sf::Vector2f deltaDist(
                    sqrt(1.0f + (rayDir.y * rayDir.y) / (rayDir.x * rayDir.x)),
                    sqrt(1.0f + (rayDir.x * rayDir.x) / (rayDir.y * rayDir.y))
            );

            sf::Vector2i mapPos(rayPos); // which box of the map we're in

            sf::Vector2i step; // what direction to step in (+1 or -1 for each dimension)
            sf::Vector2f sideDist; // length of ray from current position to next x or y side

            // calculate step and initial sideDist
            if (rayDir.x < 0.0f) {
                step.x = -1;
                sideDist.x = (rayPos.x - mapPos.x) * deltaDist.x;
            } else {
                step.x = 1;
                sideDist.x = (mapPos.x + 1.0f - rayPos.x) * deltaDist.x;
            }
            if (rayDir.y < 0.0f) {
                step.y = -1;
                sideDist.y = (rayPos.y - mapPos.y) * deltaDist.y;
            } else {
                step.y = 1;
                sideDist.y = (mapPos.y + 1.0f - rayPos.y) * deltaDist.y;
            }

            char tile = '.'; // tile type that got hit
            bool horizontal; // did we hit a horizontal side? Otherwise it's vertical

            // cast the ray until we hit a wall
            while (tile == '.') {
                if (sideDist.x < sideDist.y) {
                    sideDist.x += deltaDist.x;
                    mapPos.x += step.x;
                    horizontal = true;
                } else {
                    sideDist.y += deltaDist.y;
                    mapPos.y += step.y;
                    horizontal = false;
                }

                tile = getTile(mapPos.x, mapPos.y);
            }

            // calculate wall distance, projected on camera direction
            float perpWallDist;
            if (horizontal) {
                perpWallDist = std::abs((mapPos.x - rayPos.x + (1 - step.x) / 2) / rayDir.x);
            } else {
                perpWallDist = std::abs((mapPos.y - rayPos.y + (1 - step.y) / 2) / rayDir.y);
            }

            // calculate height of line to draw on the screen
            int lineHeight = abs(int(screenHeight / perpWallDist));

            // calculate lowest and highest pixel to fill in current line
            int drawStart = -lineHeight / 2 + screenHeight / 2;
            if (drawStart < 0) {
                drawStart = 0;
            }
            int drawEnd = lineHeight / 2 + screenHeight / 2;
            if (drawEnd < 0) {
                drawEnd = 0;
            }

            // get wall color
            sf::Color color = wallTypes.find(tile)->second;

            // illusion of shadows by making horizontal walls darker
            if (horizontal) {
                color.r /= 2;
                color.g /= 2;
                color.b /= 2;
            }

            // add lines to vertex buffer
            lines[x * 2].position = sf::Vector2f((float)x, (float)drawStart);
            lines[x * 2].color = color;
            lines[x * 2 + 1].position = sf::Vector2f((float)x, (float)drawEnd);
            lines[x * 2 + 1].color = color;
        }

        window.clear();
        window.draw(lines);
        window.draw(fpsText);
        window.display();
    }

    return EXIT_SUCCESS;
}
