/*	TSprite.h
	Copyright (c) 2024, J T Frey
*/

/*!
	@header Sprites
	In this game a sprite is the encapsulation of a tetromino (as a
	64-bit value comprising all four 16-bit orientations); the
	chosen orientation of the tetromino; the grid position at which to
	start rendering the tetromino; horizontal and vertical shifts of
	the tetromino inside its bounding box; and the color index that
	should be used when rendering the tetromino.
	
	The API includes functions that honor side-effects of altering the
	orientation of the tetromino (like rotation of the shift values).
	A funtion that determines how many leading rows of the selected
	orientation are blank.
*/

#ifndef __TSPRITE_H__
#define __TSPRITE_H__

#include "tetrominotris_config.h"
#include "TTetrominos.h"

/*
 * @typedef TSprite
 *
 * In this context, a sprite is a chosen orientation of a tetromino
 * associated with bit grid position P.  The chosen orientation is
 * optionally shifted N steps (|N|) in the left/right or up/down
 * direction (according to the sign of N).
 *
 * The colorIdx in an integer in the range [1,3] corresponding to
 * the three-color palette associated with each level.
 */
typedef struct {
    TGridPos        P;
    unsigned int    orientation;
    int             shiftI, shiftJ;
    unsigned int    colorIdx;
    uint64_t        tetromino;
} TSprite;

/*
 * @function TSpriteMake
 *
 * Initializes and returns a TSprite containing the provided
 * 4-orientation tetromino associated with grid position P.
 * The sprite starts in the given orientation with no
 * shifting and a random color index.
 */
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
                    .colorIdx = random() % 3,
                    .tetromino = tetromino
                };
    
    return theSprite;
}

/*
 * @function TSpriteMakeRotated
 *
 * Return a copy of sprite that has been rotated.
 */
static inline TSprite
TSpriteMakeRotated(
    TSprite     *sprite
)
{
    TSprite     newSprite = {
                    .P = sprite->P,
                    .orientation = (sprite->orientation + 1) % 4,
                    .shiftI = -sprite->shiftJ, .shiftJ = -sprite->shiftI,
                    .colorIdx = sprite->colorIdx,
                    .tetromino = sprite->tetromino
                };
    return newSprite;
}

/*
 * @function TSpriteMakeRotatedAnti
 *
 * Return a copy of sprite that has been rotated counter-clockwise.
 */
static inline TSprite
TSpriteMakeRotatedAnti(
    TSprite     *sprite
)
{
    TSprite     newSprite = {
                    .P = sprite->P,
                    .orientation = (sprite->orientation - 1) % 4,
                    .shiftI = -sprite->shiftJ, .shiftJ = -sprite->shiftI,
                    .colorIdx = sprite->colorIdx,
                    .tetromino = sprite->tetromino
                };
    return newSprite;
}

/*
 * @function TSpriteGet4x4
 *
 * Extract the current 4x4 bitmap representation of the tetromino
 * in the indicated orientation.  The bitmap is shifted left/right
 * or up/down according to the shift values in sprite.
 */
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

/*
 * @function TSpriteRotate
 *
 * When the sprite is "rotated" the orientiation is incremented (modulus 4).
 * Orientation "rotation" goes clockwise, so the shifts must also rotate:
 *
 *    shiftI = -shiftJ    and    shiftJ = -shiftI
 */
static inline void
TSpriteRotate(
    TSprite     *sprite
)
{
    int         swap;
    
    sprite->orientation = (sprite->orientation + 1) % 4;
    swap = sprite->shiftI;
    sprite->shiftI = -sprite->shiftJ;
    sprite->shiftJ = -swap;
}

/*
 * @function TSpriteRotateAnti
 *
 * When the sprite is "rotated" the orientiation is decremented (modulus 4).
 * Orientation "rotation" goes counter-clockwise, so the shifts must also rotate:
 *
 *    shiftI = -shiftJ    and    shiftJ = -shiftI
 */
static inline void
TSpriteRotateAnti(
    TSprite     *sprite
)
{
    int         swap;
    
    sprite->orientation = (sprite->orientation - 1) % 4;
    swap = sprite->shiftI;
    sprite->shiftI = -sprite->shiftJ;
    sprite->shiftJ = -swap;
}

/*
 * @function TSpriteGetInitialClearRows
 *
 * The current 4x4 bitmap represenation of sprite is analyzed to
 * determine how many of its leading rows are empty.  Primarily used
 * to determine whether or not a next sprite needs to be shifted
 * up/down on the game board so that it appears immediately adjacent
 * to the top edge.
 */
static inline unsigned int
TSpriteGetInitialClearRows(
    TSprite     *sprite
)
{
    uint16_t        T = TSpriteGet4x4(sprite);
    
    if ( (T == 0) || (T & 0x000F) ) return 0;
    if ( T & 0x00F0 ) return 1;
    if ( T & 0x0F00 ) return 2;
    if ( T & 0xF000 ) return 3;
    return 0;
}
    
#endif /* __TSPRITE_H__ */
