/*	TTetrominos.h
	Copyright (c) 2024, J T Frey
*/

/*!
	@header Tetrominos (oes?)
	A domino is a rectangular brick composed of two squares:  there can
	be only one way that two squares are adjoined.  For the case of four
	squares, there are seven unique arrangements called tetrominos:
	
            ####  ###  ###  ###   ##     ##   ##
	              #     #     #    ##   ##    ##
	
	The maximum width is four squares; if the first tetromino were rotated
	90 degrees the maximum height would be four squares.  A 4x4 bitmap
	equates with a single 16-bit word, so each tetromino in four orientations
	(0, 90, 180, 270 degrees of rotation) can fit in a single 64-bit word.
	
	Assuming bit 0 is the upper-left corner of the tetromino and bit 15 is
	the lower-right corner, moving the bitmap inside the cell is easily
	effected with bit masking and shifting operations.  E.g. a right shift
	by 4 bits discards row 0, leaving row 1 (then 2 and 3) in its place.
	
	This unit contains the seven four-orientation tetrominos (packed in
	64-bit words) and functions to manipulate them.
*/

#ifndef __TTETROMINOS_H__
#define __TTETROMINOS_H__

#include "tetrominotris_config.h"

/*
 * @defined TTetrominosCount
 *
 * The number of distinct game pieces.
 */
#define TTetrominosCount 7

/*
 * @const TTetrominos
 *
 * The set of all seven tetrominos in their four orientations.
 *
 *     F    C      Each orientation of the tetromino is representable as a
 *     [....]      16-bit value.  Since there are 4 orientations, each
 *     B    8      tetromino can be represented by a 64-bit value.
 *     [.XX.]
 *     7    4      The "square" tetromino has symmetry such that each
 *     [.XX.]      orientation is exactly the same:
 *     3    0 
 *     [....]          0b0000011001100000 = 0x0660
 *     
 *     F    C      The "straight" tetromino.
 *     [..X.]
 *     B    8          0b0010001000100010 = 0x2222
 *     [..X.]          0b0000000011110000 = 0x00F0
 *     7    4          0b0100010001000100 = 0x4444
 *     [..X.]          0b0000111100000000 = 0x0F00
 *     3    0
 *     [..X.]
 *     
 *     F    C      The "T" tetromino.
 *     [..X.]
 *     B    8          0b0010011000100000 = 0x2620
 *     [.XX.]          0b0000001001110000 = 0x0270
 *     7    4          0b0100011001000000 = 0x4640
 *     [..X.]          0b0000111001000000 = 0x0E40
 *     3    0
 *     [....]
 *     
 *     F    C      The first "skew" tetromino.
 *     [.X..]
 *     B    8          0b0100011000100000 = 0x4620
 *     [.XX.]          0b0000001101100000 = 0x0360
 *     7    4          0b0100011000100000 = 0x4620
 *     [..X.]          0b0000001101100000 = 0x0360
 *     3    0
 *     [....]
 *     
 *     F    C      The chiral partner "skew" tetromino.
 *     [..X.]
 *     B    8          0b0010011001000000 = 0x2640
 *     [.XX.]          0b0000011000110000 = 0x0630
 *     7    4          0b0010011001000000 = 0x2640
 *     [.X..]          0b0000011000110000 = 0x0630
 *     3    0
 *     [....]
 *     
 *     F    C      The first "L" tetromino.
 *     [.X..]
 *     B    8          0b0100010001100000 = 0x4460
 *     [.X..]          0b0000011101000000 = 0x0740
 *     7    4          0b0110001000100000 = 0x6220
 *     [.XX.]          0b0000000111100000 = 0x0170
 *     3    0
 *     [....]
 *     
 *     F    C      The chiral partner "L" tetromino.
 *     [..X.]
 *     B    8          0b0010001001100000 = 0x2260
 *     [..X.]          0b0000010001110000 = 0x0470
 *     7    4          0b0110010001000000 = 0x6440
 *     [.XX.]          0b0000011100010000 = 0x0710
 *     3    0
 *     [....]
 *     
 */
extern const uint64_t  TTetrominos[TTetrominosCount];

/*
 * @function TTetrominosExtractOrientation
 *
 * Extracts the given orientation number ([0,3]) from the
 * tetromino at index tetromoniId of the TTetrominos array.
 */
static inline uint16_t
TTetrominosExtractOrientation(
    unsigned int    tetrominoId,
    unsigned int    orientation
)
{
    return (uint16_t)((TTetrominos[tetrominoId] & (0xFFFF000000000000 >> (orientation << 4))) >> ((3 - orientation) << 4));
}

/*
 * @function TTetrominoExtractOrientation
 *
 * Extracts the given orientation number ([0,3]) from the
 * given tetromino representation.
 */
static inline uint16_t
TTetrominoExtractOrientation(
    uint64_t        tetromino,
    unsigned int    orientation
)
{
    return (uint16_t)((tetromino & (0xFFFF000000000000 >> (orientation << 4))) >> ((3 - orientation) << 4));
}

/*
 * @function TTetrominoShiftUp
 *
 * Shift all four orientations of a tetromino in the unit cell.  If all four
 * orientations are clear at the top, the move is optimized as a single
 * bit shift.  Otherwise each orientation has to be handled individually.
 *
 * Returns the shifted 64-bit tetrominos representation.
 */
static inline uint64_t
TTetrominoShiftUp(
    uint64_t    tetromino
)
{
    // They're all clear at the bottom, shift en masse
    if ( ! (tetromino & 0x000F000F000F000FULL) ) {
        tetromino >>= 4;
    } else {
        // Check each orientation explicitly:
        if ( ! (tetromino & 0x000F000000000000ULL) )
            tetromino = ((tetromino & 0xFFF0000000000000ULL) >> 4) | (tetromino & 0x0000FFFFFFFFFFFFULL);
        if ( ! (tetromino & 0x0000000F00000000ULL) )
            tetromino = ((tetromino & 0x0000FFF000000000ULL) >> 4) | (tetromino & 0xFFFF0000FFFFFFFFULL);
        if ( ! (tetromino & 0x00000000000F0000ULL) )
            tetromino = ((tetromino & 0x00000000FFF00000ULL) >> 4) | (tetromino & 0xFFFFFFFF0000FFFFULL);
        if ( ! (tetromino & 0x000000000000000FULL) )
            tetromino = ((tetromino & 0x000000000000FFF0ULL) >> 4) | (tetromino & 0xFFFFFFFFFFFF0000ULL);
    }
    return tetromino;
}

/*
 * @function TTetrominoShiftDown
 *
 * Shift all four orientations of a tetromino in the unit cell.  If all four
 * orientations are clear at the bottom, the move is optimized as a single
 * bit shift.  Otherwise each orientation has to be handled individually.
 *
 * Returns the shifted 64-bit tetrominos representation.
 */
static inline uint64_t
TTetrominoShiftDown(
    uint64_t    tetromino
)
{
    // They're all clear at the top, shift en masse
    if ( ! (tetromino & 0xF000F000F000F000ULL) ) {
        tetromino <<= 4;
    } else {
        // Check each orientation explicitly:
        if ( ! (tetromino & 0xF000000000000000ULL) )
            tetromino = ((tetromino & 0x0FFF000000000000ULL) << 4) | (tetromino & 0x0000FFFFFFFFFFFFULL);
        if ( ! (tetromino & 0x0000F00000000000ULL) )
            tetromino = ((tetromino & 0x00000FFF00000000ULL) << 4) | (tetromino & 0xFFFF0000FFFFFFFFULL);
        if ( ! (tetromino & 0x00000000F0000000ULL) )
            tetromino = ((tetromino & 0x000000000FFF0000ULL) << 4) | (tetromino & 0xFFFFFFFF0000FFFFULL);
        if ( ! (tetromino & 0x000000000000F000ULL) )
            tetromino = ((tetromino & 0x0000000000000FFFULL) << 4) | (tetromino & 0xFFFFFFFFFFFF0000ULL);
    }
    return tetromino;
}

/*
 * @function TTetrominoShiftLeft
 *
 * Shift all four orientations of a tetromino in the unit cell.  If all four
 * orientations are clear to the left, the move is optimized as a single
 * bit shift.  Otherwise each orientation has to be handled individually.
 *
 * Returns the shifted 64-bit tetrominos representation.
 */
static inline uint64_t
TTetrominoShiftLeft(
    uint64_t    tetromino
)
{
    // They're all clear at the left, shift en masse
    if ( ! (tetromino & 0x1111111111111111ULL) ) {
        tetromino >>= 1;
    } else {
        // Check each orientation explicitly:
        if ( ! (tetromino & 0x1111000000000000ULL) )
            tetromino = ((tetromino & 0xEEEE000000000000ULL) >> 1) | (tetromino & 0x0000FFFFFFFFFFFFULL);
        if ( ! (tetromino & 0x0000111100000000ULL) )
            tetromino = ((tetromino & 0x0000EEEE00000000ULL) >> 1) | (tetromino & 0xFFFF0000FFFFFFFFULL);
        if ( ! (tetromino & 0x0000000011110000ULL) )
            tetromino = ((tetromino & 0x00000000EEEE0000ULL) >> 1) | (tetromino & 0xFFFFFFFF0000FFFFULL);
        if ( ! (tetromino & 0x0000000000001111ULL) )
            tetromino = ((tetromino & 0x000000000000EEEEULL) >> 1) | (tetromino & 0xFFFFFFFFFFFF0000ULL);
    }
    return tetromino;
}

/*
 * @function TTetrominoShiftRight
 *
 * Shift all four orientations of a tetromino in the unit cell.  If all four
 * orientations are clear to the right, the move is optimized as a single
 * bit shift.  Otherwise each orientation has to be handled individually.
 *
 * Returns the shifted 64-bit tetrominos representation.
 */
static inline uint64_t
TTetrominoShiftRight(
    uint64_t    tetromino
)
{
    // They're all clear at the right, shift en masse
    if ( ! (tetromino & 0x8888888888888888ULL) ) {
        tetromino <<= 1;
    } else {
        // Check each orientation explicitly:
        if ( ! (tetromino & 0x8888000000000000ULL) )
            tetromino = ((tetromino & 0x7777000000000000ULL) << 1) | (tetromino & 0x0000FFFFFFFFFFFFULL);
        if ( ! (tetromino & 0x0000888800000000ULL) )
            tetromino = ((tetromino & 0x0000777700000000ULL) << 1) | (tetromino & 0xFFFF0000FFFFFFFFULL);
        if ( ! (tetromino & 0x0000000088880000ULL) )
            tetromino = ((tetromino & 0x0000000077770000ULL) << 1) | (tetromino & 0xFFFFFFFF0000FFFFULL);
        if ( ! (tetromino & 0x0000000000008888ULL) )
            tetromino = ((tetromino & 0x0000000000007777ULL) << 1) | (tetromino & 0xFFFFFFFFFFFF0000ULL);
    }
    return tetromino;
}

/*
 * @function TTetrominoOrientationShiftUp
 *
 * Move the tetromino up one row in the unit cell.  The tetromino
 * is not allowed to move beyond the top edge of the cell.
 *
 * The movement boils down to deleting the first nibble (4 bits)
 * from the word by means of shifting 4 bits to the left.
 *
 * Returns the shifted 64-bit tetrominos representation.
 */
static inline uint16_t
TTetrominoOrientationShiftUp(
    uint16_t    tetromino
)
{
    return ( ! (tetromino & 0x000F) ) ? (tetromino >> 4) : tetromino;
}

/*
 * @function TTetrominoOrientationShiftDown
 *
 * Move the tetromino down one row in the unit cell.  The tetromino
 * is not allowed to move beyond the bottom edge of the cell.
 *
 * The movement boils down to deleting the final nibble (4 bits)
 * from the word by means of shifting 4 bits to the right.
 *
 * Returns the shifted 64-bit tetrominos representation.
 */
static inline uint16_t
TTetrominoOrientationShiftDown(
    uint16_t    tetromino
)
{
    return ( ! (tetromino & 0xF000) ) ? (tetromino << 4) : tetromino;
}

/*
 * @function TTetrominoOrientationShiftLeft
 *
 * Move the tetromino left one column in the unit cell.  The tetromino
 * is not allowed to move beyond the left edge of the cell.
 *
 * The movement boils down to deleting the 4th bit from each nibble
 * (3, 7, 11, 15) of the word by means of shifting 1 bit to the left.
 *
 * Returns the shifted 64-bit tetrominos representation.
 */
static inline uint16_t
TTetrominoOrientationShiftLeft(
    uint16_t    tetromino
)
{
    return ( ! (tetromino & 0x1111) ) ? (tetromino >> 1) : tetromino;
}

/*
 * @function TTetrominoOrientationShiftRight
 *
 * Move the tetromino right one column in the unit cell.  The tetromino
 * is not allowed to move beyond the right edge of the cell.
 *
 * The movement boils down to deleting the 0th bit from each nibble
 * (0, 4, 8, 12) of the word by means of shifting 1 bit to the right.
 *
 * Returns the shifted 64-bit tetrominos representation.
 */
static inline uint16_t
TTetrominoOrientationShiftRight(
    uint16_t    tetromino
)
{
    return ( ! (tetromino & 0x8888) ) ? (tetromino << 1) : tetromino;
}

/*
 * @function TTetrominoOrientationShiftVertical
 *
 * Move the tetromino up (negative) or down (positive) by the
 * specified number of steps (absolute value of shift).
 */
static inline uint16_t
TTetrominoOrientationShiftVertical(
    uint16_t    tetromino,
    int         shift
)
{
    if ( shift != 0 ) {
        if ( shift < 0 )
            while ( shift++ < 0 ) tetromino = TTetrominoOrientationShiftUp(tetromino);
        else
            while ( shift-- > 0 ) tetromino = TTetrominoOrientationShiftDown(tetromino);
    }
    return tetromino;
}


/*
 * @function TTetrominoOrientationShiftHorizontal
 *
 * Move the tetromino left (negative) or right (positive) by the
 * specified number of steps (absolute value of shift).
 */
static inline uint16_t
TTetrominoOrientationShiftHorizontal(
    uint16_t    tetromino,
    int         shift
)
{
    if ( shift != 0 ) {
        if ( shift < 0 )
            while ( shift++ < 0 ) tetromino = TTetrominoOrientationShiftLeft(tetromino);
        else
            while ( shift-- > 0 ) tetromino = TTetrominoOrientationShiftRight(tetromino);
    }
    return tetromino;
}


/*
 * @function TTetrominoSummary
 *
 * Display all four orientations of the given 64-bit tetromino set to
 * stdout.  The orientations are shown side-by-side 4x4 character blocks.
 */
void TTetrominoSummary(uint64_t fullTetromino);

/*
 * @function TTetrominoOrientationSummary
 *
 * Display the given orientation of a tetromino to stdout as a 4x4
 * character block.
 */
void TTetrominoOrientationSummary(uint16_t tetromino);
    
#endif /* __TTETROMINOS_H__ */
