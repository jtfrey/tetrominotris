/*	TCell.h
	Copyright (c) 2024, J T Frey
*/

/*!
	@header Grid cell
	A bit grid contains one or more bitmap planes; the aggregation of
	each plane's bit from an arbitrary grid position is the cell value.
	There can be anywhere from 1 to 8 bit planes in a grid, so the
	TCell is a wrapper for an 8-bit unsigned integer.
	
	Bit 0 is used to determine positional occupation on the grid:  a
	1 means filled, 0 means empty.  The same bit is used regardless
	of whether or not color information is present on the grid.
	
	In color mode, two additional channels (bits 1 and 2) allow up to
	four colors to be associated with the cells.
	
	All functions are very simple and are declared for static
	inlining to avoid function calls as much as possible.
*/

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
 * - color mode:  leverages a 2-bit (4-color) palette;
 *       transparency is implicit in the on/off flag and
 *       flash-on-completion is also used
 *
 * The bit positions correspond with TBitGrid channels, e.g.
 * a color game board requires a TBitGrid of four channels.
 */
enum {
    TCellColorMask      = 0b00000110,
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
 * @function TCellMake3Bit
 *
 * Initializes and returns a TCell with the on/off state of
 * isOccupied and the given colorIndex represented.
 */
static inline TCell
TCellMake3Bit(
    bool        isOccupied,
    int         colorIndex
)
{
    return (uint8_t)((isOccupied ? TCellIsOccupied : 0) |
                     ((uint8_t)(colorIndex << 1) & TCellColorMask));
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
 * @function TCellGetColorIndex
 *
 * Returns the color index present in theCell.
 */
static inline int
TCellGetColorIndex(
    TCell       theCell
)
{
    return (theCell & TCellColorMask) >> 1;
}
    
#endif /* __TCELL_H__ */
