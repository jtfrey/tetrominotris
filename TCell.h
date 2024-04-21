
#ifndef __TCELL_H__
#define __TCELL_H__

#include "tetrominotris_config.h"

enum {
    TCellColorMask      = 0b00001100,
    TCellIsFlashing     = 0b00000010,
    TCellIsOccupied     = 0b00000001
};

typedef uint8_t TCell;


static inline TCell
TCellMake1Bit(
    bool        isOccupied
)
{
    return (uint8_t)(isOccupied ? TCellIsOccupied : 0);
}

static inline TCell
TCellMake2Bit(
    bool        isOccupied,
    bool        isFlashing
)
{
    return (uint8_t)((isOccupied ? TCellIsOccupied : 0) | 
                     (isFlashing ? TCellIsFlashing : 0));
}

static inline TCell
TCellMake4Bit(
    bool        isOccupied,
    bool        isFlashing,
    int         colorIndex
)
{
    return (uint8_t)((isOccupied ? TCellIsOccupied : 0) | 
                     (isFlashing ? TCellIsFlashing : 0) |
                     (colorIndex & TCellColorMask));
}

static inline bool
TCellGetIsOccupied(
    TCell       theCell
)
{
    return ((theCell & TCellIsOccupied) != 0);
}

static inline bool
TCellGetIsFlashing(
    TCell       theCell
)
{
    return ((theCell & TCellIsFlashing) != 0);
}

static inline int
TCellGetColorIndex(
    TCell       theCell
)
{
    return (theCell & TCellColorMask) >> 2;
}
    
#endif /* __TCELL_H__ */
