
#ifndef __TBOARD_H__
#define __TBOARD_H__

#include "tetrominotris_config.h"

/*
 * @typedef TBoard
 *
 * The Tetris game board will be represented as an array of
 * 32-bit words.  Given the width (W) and height (H), the array
 * must be ((H)(W) + 31) / 32 words in size.
 *
 * The default game board is 10W x 20H.
 *
 * The data structure is variably-sized w.r.t. the grid array
 * so instances must be dynamically allocated using the
 * TBoardCreate() function.  The TBoardDestroy() function is used
 * to dispose of a TBoard instance.
 */
typedef struct {
    unsigned int    w, h;
    size_t          nGridWords;
    uint32_t        grid[];
} TBoard;

/*
 * @function TBoardCreate
 *
 * Allocate a new TBoard instance given the width (w) and height (h)
 * of the game board.  The grid field will be sized according to
 * the number of words required to represent the board.
 */
static inline TBoard*
TBoardCreate(
    unsigned int    w,
    unsigned int    h
)
{
    size_t          reqWords = (w * h + 31) / 32;
    TBoard          *newBoard = (TBoard*)malloc(sizeof(TBoard) + reqWords * sizeof(uint32_t));
    
    if ( newBoard ) {
        newBoard->w = w;
        newBoard->h = h;
        newBoard->nGridWords = reqWords;
        memset(newBoard->grid, 0, newBoard->nGridWords * sizeof(uint32_t));
    }
    return newBoard;
}

/*
 * @function TBoardDestroy
 *
 * Deallocate a TBoard instance.
 */
static inline void
TBoardDestroy(
    TBoard      *board
)
{
    free((void*)board);
}

/*
 * @function TBoardFill
 *
 * Reset a game board grid such that all positions are the
 * given value.
 */
static inline void
TBoardFill(
    TBoard      *board,
    bool        value
)
{
    memset(board->grid, value ? 0xFF : 0x00, board->nGridWords * sizeof(uint32_t));
}

/*
 * @function TBoardScroll
 *
 * Shift the entire game board down one line — e.g. when a row has
 * been completed.
 */
void TBoardScroll(TBoard *board);

/*
 * @function TBoardClearLines
 *
 * Shift the game board down one line — e.g. when a row has been
 * completed somewhere other than the bottom row.
 */
void TBoardClearLines(TBoard *board, unsigned int jLow, unsigned int jHigh);

/*
 * @function TBoardSummary
 *
 * Display the game board as a grid of Unicode characters.  Positions
 * that are "true" are drawn as a bounded, unfilled box.  Positions
 * that are "false" are drawn as whitespace (ASCII 0x20).
 *
 */
void TBoardSummary(TBoard *board);

/*
 * @function TBoardGridPosMakeWithIndex
 *
 * Given a game board and byte + bit indices, initialize and return the
 * corresponding grid position.
 */
static inline TGridPos
TBoardGridPosMakeWithIndex(
    TBoard          *board,
    unsigned int    B,
    unsigned int    b
)
{
    unsigned int    p = B * 32 + b;
    TGridPos        P = { .i = p / board->w, .j = p % board->w };
    return P;
}

/*
 * @function TBoardGridIndexMakeWithPos
 *
 * Given a game board and i,j positions, initialize and return the
 * corresponding grid index.
 */
static inline TGridIndex
TBoardGridIndexMakeWithPos(
    TBoard          *board,
    unsigned int    i,
    unsigned int    j
)
{
    unsigned int    ii = j * board->w + i;
    TGridIndex      I = { .W = ii / 32, .b = ii % 32 };
    return I;
}

/*
 * @function TBoardGridPosGetBaseOffsetAndMask
 *
 * Given a game board and grid position P, calculate the correponding
 * baseOffset into the grid array and in-word mask.
 */
static inline void
TBoardGridPosGetBaseOffsetAndMask(
    TBoard          *board,
    TGridPos        P,
    unsigned int    *baseOffset,
    uint32_t        *mask
)
{
    unsigned        o = P.j * board->w + P.i;
    
    *baseOffset = (o / 32);
    *mask = 1 << (o % 32);
}

/*
 * @function TBoardGridPosToIndex
 *
 * Given a game board and grid position P, initialize and return
 * the corresponding grid index.
 */
static inline TGridIndex
TBoardGridPosToIndex(
    TBoard      *board,
    TGridPos    P
)
{
    unsigned int    i = P.j * board->w + P.i;
    TGridIndex      I = { .W = i / 32, .b = i % 32 };
    return I;
}

/*
 * @function TBoardGridIndexToPos
 *
 * Given a game board and grid index I, initialize and return
 * the corresponding grid position.
 */
static inline TGridPos
TBoardGridIndexToPos(
    TBoard      *board,
    TGridIndex  I
)
{
    unsigned int    p = I.W * 32 + I.b;
    TGridPos        P = { .i = p / board->w, .j = p % board->w };
    return P;
}

/*
 * @function TBoardGetValueAtGridIndex
 *
 * Given a game board and grid index I, return the binary value
 * of that position on the board.
 */
static inline bool
TBoardGetValueAtGridIndex(
    TBoard      *board,
    TGridIndex  I
)
{
    return ((board->grid[I.W] & (1 << I.b)) != 0);
}

/*
 * @function TBoardSetValueAtGridIndex
 *
 * Given a game board and grid index I, set that position on the
 * board to the given binary value.
 */
static inline void
TBoardSetValueAtGridIndex(
    TBoard      *board,
    TGridIndex  I,
    bool        value
)
{
    if ( value ) {
        board->grid[I.W] |= (1 << I.b);
    } else
        board->grid[I.W] &= ~(1 << I.b);
}

#endif /* __TBOARD_H__ */
