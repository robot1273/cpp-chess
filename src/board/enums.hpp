#pragma once

namespace chess{
    /*Simple black or white enum*/
    enum Colour{
        WHITE, BLACK, NO_COLOUR,
    };

    enum Piece { //Order matters!
        PAWN   = 0,
        KNIGHT = 1, //PROMOTE_KNIGHT = 2
        BISHOP = 2, //PROMOTE_BISHOP = 3
        ROOK   = 3, //PROMOTE_ROOK   = 4
        QUEEN  = 4, //PROMOTE_QUEEN  = 5
        KING   = 5,
        NONE   = 6
    };

    //promotion-1 = corresponding piece enum
}
