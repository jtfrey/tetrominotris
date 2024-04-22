
#ifndef __TCELL_H__
#define __TCELL_H__

#include "tetrominotris_config.h"

/*
 * @enum TCell component fields
 *
 * The game board may be represented in three unique forms:
 *
 * - single-bit on/off mode:  the simplest form of the game
 *       which lacks all TUI embellishment
 * - on/off + flash mode:  adds a flashing state so that
 *       completed rows can be flashed before disappearing
 *  - color mode:  leverages a 2-bit (4-color) palette;
 *       transparency is implicit in the on/off flag and
 *       flash-on-completion is also used
 *
 * The bit positions correspond with TBitGrid channels, e.g.
 * a color game board requires a TBitGrid of four channels.
 */
enum {
    TCellColorMask      = 0b00001100,
    TCellIsFlashing     = 0b00000010,
    TCellIsOccupied     = 0b00000001
};

/*
 * @typedef TCell
 *
 * The type of a TCell value moved in/out of the TBitGrid.
 */
typedef uint8_t TCell;

/*
 * @function TCellMake1Bit
 *
 * Initializes and returns a TCell with only the on/off
 * state isOccupied represented.
 */
static inline TCell
TCellMake1Bit(
    bool        isOccupied
)
{
    return (uint8_t)(isOccupied ? TCellIsOccupied : 0);
}

/*
 * @function TCellMake2Bit
 *
 * Initializes and returns a TCell with the on/off state of
 * isOccupied and isFlashing represented.
 */
static inline TCell
TCellMake2Bit(
    bool        isOccupied,
    bool        isFlashing
)
{
    return (uint8_t)((isOccupied ? TCellIsOccupied : 0) | 
                     (isFlashing ? TCellIsFlashing : 0));
}

/*
 * @function TCellMake4Bit
 *
 * Initializes and returns a TCell with the on/off state of
 * isOccupied and isFlashing and the given colorIndex
 * represented.
 */
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

/*
 * @function TCellGetIsOccupied
 *
 * Returns true if theCell has the is-occupied bit set, false
 * otherwise.
 */
static inline bool
TCellGetIsOccupied(
    TCell       theCell
)
{
    return ((theCell & TCellIsOccupied) != 0);
}

/*
 * @function TCellGetIsFlashing
 *
 * Returns true if theCell has the is-flashing bit set, false
 * otherwise.
 */
static inline bool
TCellGetIsFlashing(
    TCell       theCell
)
{
    return ((theCell & TCellIsFlashing) != 0);
}

/*
 * @function TCellGetColorIndex
 *
 * Returns the color index present in theCell.
 */
static inline int
TCellGetColorIndex(
    TCell       theCell
)
{
    return (theCell & TCellColorMask) >> 2;
}
    
#endif /* __TCELL_H__ */
