
#ifndef __TBITGRID_H__
#define __TBITGRID_H__

#include "tetrominotris_config.h"


typedef struct {
    unsigned int        nChannels;          // Number of independent bits per cell
    unsigned int        nBitsPerWord;       // Number of bits in this instance's word
    unsigned int        nBytesPerWord;      // Number ofr bytes in this instance's word
    unsigned int        w, h;               // Nominal width and height of the grid
    unsigned int        nWordsPerRow;       // Number of words per row of the grid
    unsigned int        nWordsTotal;        // Total words in the grid
} TBitGridDimensions;



struct TBitGrid;


typedef uint8_t (*TBitGridGetCellValueAtIndex)(struct TBitGrid *theGrid, TGridIndex theIndex);
typedef void (*TBitGridSetCellValueAtIndex)(struct TBitGrid *theGrid, TGridIndex theIndex, uint8_t theValue);


typedef union TBitGridChannelPtr * TBitGridStorage;


typedef struct TBitGrid {
    TBitGridDimensions  dimensions;
    TBitGridStorage     grid;
    struct {
        TBitGridGetCellValueAtIndex     getCellValueAtIndex;
        TBitGridSetCellValueAtIndex     setCellValueAtIndex;
    } callbacks;
} TBitGrid;

/*
 * @function TBitGridMakeGridPosWithIndex
 *
 * Given a bit grid and word, bit indices, initialize and return the
 * corresponding grid position.
 */
static inline TGridPos
TBitGridMakeGridPosWithIndex(
    TBitGrid        *bitGrid,
    unsigned int    W,
    unsigned int    b
)
{
    TGridPos        P = {
                        .i = (W % bitGrid->dimensions.nWordsPerRow) + b,
                        .j = W / bitGrid->dimensions.nWordsPerRow
                    };
    return P;
}

/*
 * @function TBitGridMakeGridIndexWithPos
 *
 * Given a bit grid and i,j positions, initialize and return the
 * corresponding grid index.
 */
static inline TGridIndex
TBitGridMakeGridIndexWithPos(
    TBitGrid        *bitGrid,
    unsigned int    i,
    unsigned int    j
)
{
    unsigned int    offsetH = j * bitGrid->dimensions.nWordsPerRow;
    TGridIndex      I = {
                        .W = offsetH + i / bitGrid->dimensions.nBitsPerWord,
                        .b = i % bitGrid->dimensions.nBitsPerWord
                    };
    return I;
}


static inline TGridIndex
TBitGridPositionToIndex(
    TBitGrid        *bitGrid,
    TGridPos        P
)
{
    unsigned int    offsetH = P.j * bitGrid->dimensions.nWordsPerRow;
    TGridIndex      I = {
                        .W = offsetH + P.i / bitGrid->dimensions.nBitsPerWord,
                        .b = P.i % bitGrid->dimensions.nBitsPerWord
                    };
    return I;
}


static inline TGridPos
TBitGridIndexToPosition(
    TBitGrid        *bitGrid,
    TGridIndex      I
)
{
    unsigned int    remnant = I.W % bitGrid->dimensions.nWordsPerRow;
    TGridPos        P = {
                        .j = I.W / bitGrid->dimensions.nWordsPerRow,
                        .i = ((remnant * bitGrid->dimensions.nBitsPerWord) + I.b)
                    };
    return P;
}


static inline uint8_t
TBitGridGetValueAtIndex(
    TBitGrid        *bitGrid,
    TGridIndex      gridIndex
)
{
    return bitGrid->callbacks.getCellValueAtIndex(bitGrid, gridIndex);
}

static inline void
TBitGridSetValueAtIndex(
    TBitGrid        *bitGrid,
    TGridIndex      gridIndex,
    uint8_t         value
)
{
    return bitGrid->callbacks.setCellValueAtIndex(bitGrid, gridIndex, value);
}


static inline uint8_t
TBitGridGetValueAtPosition(
    TBitGrid        *bitGrid,
    TGridPos        position
)
{
    return bitGrid->callbacks.getCellValueAtIndex(bitGrid, TBitGridPositionToIndex(bitGrid, position));
}

static inline void
TBitGridSetValueAtPosition(
    TBitGrid        *bitGrid,
    TGridPos        position,
    uint8_t         value
)
{
    return bitGrid->callbacks.setCellValueAtIndex(bitGrid, TBitGridPositionToIndex(bitGrid, position), value);
}

/*
 * @function TBitGridCreate
 *
 * Allocate a new TBitGrid instance given the width (w) and height (h)
 * and anywhere from 1 to 8 channels.
 */
TBitGrid* TBitGridCreate(unsigned int nChannels, unsigned int w, unsigned int h);


/*
 * @function TBitGridDestroy
 *
 * Deallocate a TBitGrid instance.
 */
void TBitGridDestroy(TBitGrid* bitGrid);

void TBitGridScroll(TBitGrid *bitGrid);
void TBitGridClearLines(TBitGrid *bitGrid, unsigned int jLow, unsigned int jHigh);

void TBitGridFill(TBitGrid *bitGrid, uint8_t value);

struct TBitGridIterator;

typedef bool (*TBitGridIteratorNextFn)(struct TBitGridIterator *iterator, TGridPos *outP, uint8_t *value);
typedef bool (*TBitGridIteratorNextFullRowFn)(struct TBitGridIterator *iterator, unsigned int *outJ);

typedef struct TBitGridIterator {
    TBitGridDimensions                  dimensions;
    unsigned int                        i, j;
    unsigned int                        nFullWords, nPartialBits;
    bool                                isStarted;
    TBitGridStorage                     grid;
    struct {
        TBitGridIteratorNextFn          nextFn;
        TBitGridIteratorNextFullRowFn   nextFullRowFn;
    } callbacks;
} TBitGridIterator;

TBitGridIterator* TBitGridIteratorCreate(TBitGrid *bitGrid, uint8_t channelMask);

void TBitGridIteratorDestroy(TBitGridIterator *iterator);


/*
 * @function TBitGridIteratorNext
 *
 * Perform a single iteration.  Sets P and value and returns true if successful,
 * returns false if the iterator has completed the entire bit grid.
 *
 * Do not mix calls to this function and TBitGridIteratorNextFullRow().
 */
static inline bool
TBitGridIteratorNext(
    TBitGridIterator    *iterator,
    TGridPos            *outP,
    uint8_t             *value
)
{
    return iterator->callbacks.nextFn(iterator, outP, value);
}

/*
 * @function TBitGridIteratorNextFullRow
 *
 * Perform a single iteration.  Sets row and returns true if successful,
 * returns false if the iterator has completed the entire bit grid.
 *
 * Do not mix calls to this function and TBitGridIteratorNext().
 */
static inline bool
TBitGridIteratorNextFullRow(
    TBitGridIterator    *iterator,
    unsigned int        *outJ
)
{
    return iterator->callbacks.nextFullRowFn(iterator, outJ);
}

#endif /* __TBITGRID_H__ */
