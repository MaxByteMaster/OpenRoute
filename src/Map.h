#ifndef MAP_H
#define MAP_H

namespace map {

struct Tile
{
    Tile() : x_index(0), y_index(0)
    {

    }

    Tile(unsigned int x_index, unsigned int y_index) : x_index(x_index), y_index(y_index)
    {

    }

    unsigned int x_index;
    unsigned int y_index;
};

struct TileRect
{
    TileRect()
    {

    }

    Tile bottom_left_tile;
    Tile top_right_tile;
};

} // namespace map

#endif // MAP_H
