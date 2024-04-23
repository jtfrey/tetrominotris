
#ifndef __TSPRITE_H__
#define __TSPRITE_H__

#include "tetrominotris_config.h"
#include "TTetrominos.h"

typedef struct {
    TGridPos        P;
    unsigned int    orientation;
    int             shiftI, shiftJ;
    uint64_t        tetromino;
} TSprite;

static inline TSprite
TSpriteMake(
    uint64_t        tetromino,
    TGridPos        P,
    unsigned int    orientation
)
{
    TSprite     theSprite = {
                    .P = P,
                    .orientation = (orientation % 4),
                    .shiftI = 0, .shiftJ = 0,
                    .tetromino = tetromino
                };
    
    return theSprite;
}

static inline uint16_t
TSpriteGet4x4(
    TSprite     *sprite
)
{
    uint16_t    T = TTetrominoExtractOrientation(sprite->tetromino, sprite->orientation);
    
    if ( ! sprite->shiftJ ) T = TTetrominoOrientationShiftVertical(T, sprite->shiftJ);
    if ( ! sprite->shiftI ) T = TTetrominoOrientationShiftHorizontal(T, sprite->shiftI);
    return T;
}

static inline unsigned int
TSpriteGetInitialClearRows(
    TSprite     *sprite
)
{
    uint16_t        T = TSpriteGet4x4(sprite);
    unsigned int    rowCount = 0;
    
    while ( (rowCount < 4) && ! (T & 0x000F) ) rowCount++, T >>= 4;
    return rowCount;
}
    
#endif /* __TSPRITE_H__ */
