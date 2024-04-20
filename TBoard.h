
#ifndef __TBOARD_H__
#define __TBOARD_H__

#include "tetrominotris_config.h"

struct TBoard;
struct TBoardIterator;

/*
 * @typedef TBoardGetValueAtGridIndexFn
 *
 * The type of a function that retrieves the value of the game board
 * cell at the given grid index I.
 */
typedef bool (*TBoardGetValueAtGridIndexFn)(struct TBoard *board, TGridIndex I);

/*
 * @typedef TBoardSetValueAtGridIndexFn
 *
 * The type of a function that alters the game board cell at the given
 * grid index I to have the given value.
 */
typedef void (*TBoardSetValueAtGridIndexFn)(struct TBoard *board, TGridIndex I, bool value);

/*
 * @typedef TBoardIteratorInitFn
 *
 * The type of a function that initializes a TBoardIterator in
 * preparation for enumerating the game board.
 */
typedef void (*TBoardIteratorInitFn)(struct TBoard *board, struct TBoardIterator *iterator);

/*
 * @function TBoardIteratorNextFn
 *
 * The type of a function that enumerates the next position on
 * a game board.  Sets value and returns true if a position was
 * available, returns false if the iterator has completed the
 * entire board.
 */
typedef bool (*TBoardIteratorNextFn)(struct TBoardIterator *iterator, bool *value);

/*
 * @function TBoardIteratorNextFullRowFn
 *
 * The type of a function that locates a full line on the game
 * board.  Sets the row index and returns true if a full row is
 * located, returns false otherwise.
 */
typedef bool (*TBoardIteratorNextFullRowFn)(struct TBoardIterator *iterator, unsigned int *row);

/*
 * @typedef TBoardIterator
 *
 * Data structure that encapsulates all of the state for a
 * forward enumeration of the game board bitmap.  All fields
 * are set from the parent TBoard _except_ for the nextFn.
 * The nextFn is a pointer to the private function specific
 * to the nBits of the game board.
 */
typedef struct TBoardIterator {
    unsigned int                nBitsPerRow, nRows, bitIdx, rowIdx;
    unsigned int                i, j;
    bool                        isStarted;
    TBoardIteratorNextFn        nextFn;
    TBoardIteratorNextFullRowFn nextFullRowFn;
    union {
        uint64_t    *b64;
        uint32_t    *b32;
        uint16_t    *b16;
        uint8_t     *b8;
    } grid;
} TBoardIterator;

/*
 * @function TBoardIteratorNext
 *
 * Perform a single iteration of the iterator.  Sets value and returns
 * true if successful, returns false if the iterator has completed
 * the entire board.
 *
 * Do not mix calls to this function and TBoardIteratorNextFullRow().
 */
static inline bool
TBoardIteratorNext(
    TBoardIterator  *iterator,
    bool            *value
)
{
    return iterator->nextFn(iterator, value);
}

/*
 * @function TBoardIteratorNextFullRow
 *
 * Perform a single iteration of the iterator.  Sets row and returns
 * true if successful, returns false if the iterator has completed
 * the entire board.
 *
 * Do not mix calls to this function and TBoardIteratorNext().
 */
static inline bool
TBoardIteratorNextFullRow(
    TBoardIterator  *iterator,
    unsigned int    *row
)
{
    return iterator->nextFullRowFn(iterator, row);
}

/*
 * @typedef TBoard
 *
 * The Tetris game board will be represented as an array of
 * N-bit words.  The word size varies according to the width
 * of the game board:  a 10W board can't fit each row in an
 * 8-bit word, but a 16-bit word will suffice.  A board wider
 * than 64 will use multiple words of the size that yields
 * the least bit waste and largest word size.  E.g. for 79:
 *
 *      uint8_t     = 10, 1 unused
 *      uint16_t    = 5, 1 unused
 *      uint32_t    = 3, 17 unused
 *      uint64_t    = 2, 49 unused
 *
 * So it's basically a descending sort by unused bit count
 * with ties broken by ascending sort by word size.  The
 * uint16_t would be chosen in this case.
 *
 * The default game board is 10W x 20H.
 *
 * The data structure is variably-sized w.r.t. the grid array
 * so instances must be dynamically allocated using the
 * TBoardCreate() function.  The TBoardDestroy() function is used
 * to dispose of a TBoard instance.
 */
typedef struct TBoard {
    unsigned int    w, h, nBits, nBytesPerWord, nWords, nWordsPerRow;
    union {
        uint64_t    *b64;
        uint32_t    *b32;
        uint16_t    *b16;
        uint8_t     *b8;
    } grid;
    struct {
        TBoardGetValueAtGridIndexFn     getValueAtIndex;
        TBoardSetValueAtGridIndexFn     setValueAtIndex;
        TBoardIteratorInitFn            iteratorInit;
    } callbacks;
} TBoard;

/*
 * @function TBoardCreate
 *
 * Allocate a new TBoard instance given the width (w) and height (h)
 * of the game board.  The grid field will be sized according to
 * the number of words required to represent the board.
 */
TBoard* TBoardCreate(unsigned int w, unsigned int h);

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
    memset((void*)board->grid.b8, (value ? 0xFF : 0x00), board->nWords * board->nBytesPerWord);
}

/*
 * @function TBoardIterator
 *
 * Initialize and return a TBoardIterator structure that can be
 * used to enumerate the game board values.
 */
static inline TBoardIterator
TBoardIteratorMake(
    TBoard      *board
)
{
    TBoardIterator  newIterator = {
                            .nBitsPerRow = board->w,
                            .nRows = board->h,
                            .bitIdx = 0,
                            .rowIdx = 0,
                            .i = 0, .j = 0,
                            .isStarted = false,
                            .grid.b8 = board->grid.b8
                        };
    board->callbacks.iteratorInit(board, &newIterator);
    return newIterator;
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
 * @function TBoardClearFullLines
 *
 * Locate all full lines and clear them from the board.
 */

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
 * Given a game board and word, bit indices, initialize and return the
 * corresponding grid position.
 */
static inline TGridPos
TBoardGridPosMakeWithIndex(
    TBoard          *board,
    unsigned int    W,
    unsigned int    b
)
{
    TGridPos        P = {
                        .i = (W % board->nWordsPerRow) + b,
                        .j = W / board->nWordsPerRow
                    };
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
    TGridIndex      I = {
                        .W = board->nWordsPerRow * j + (i / board->nBits),
                        .b = i % board->nBits
                    };
    return I;
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
    TGridIndex      I = {
                        .W = board->nWordsPerRow * P.j + (P.i / board->nBits),
                        .b = P.i % board->nBits
                    };
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
    TGridPos        P = {
                        .i = (I.W % board->nWordsPerRow) + I.b,
                        .j = I.W / board->nWordsPerRow
                    };
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
    return board->callbacks.getValueAtIndex(board, I);
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
    board->callbacks.setValueAtIndex(board, I, value);
}

#endif /* __TBOARD_H__ */
