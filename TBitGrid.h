
#ifndef __TBITGRID_H__
#define __TBITGRID_H__

#include "tetrominotris_config.h"
#include "TCell.h"

/*
 * @typedef TBitGridDimensions
 *
 * Data structure that encapsulates off of the parameters that
 * determine the dimensioning of a TBitGrid object.  The width (w)
 * and height (h) determine the number of cells in the grid.  Each
 * cell is a sequence of bits coming from 1 to 8 (nChannels) ordered
 * channels.  Each channel is a vector of nWordsTotal units of bit
 * depth nBitsPerWord, with each row extending across nWordsPerRow
 * units.  If the width (w) is not on a nBitsPerWord boundary then
 * some number of the most-significant bits of the final word will
 * be unused.
 */
typedef struct {
    unsigned int        nChannels;          // Number of independent bits per cell
    unsigned int        nBitsPerWord;       // Number of bits in this instance's word
    unsigned int        nBytesPerWord;      // Number ofr bytes in this instance's word
    unsigned int        w, h;               // Nominal width and height of the grid
    unsigned int        nWordsPerRow;       // Number of words per row of the grid
    unsigned int        nWordsTotal;        // Total words in the grid
} TBitGridDimensions;

/* Forward declaration for the sake of declaring the callback
 * functions: */
struct TBitGrid;

/*
 * @typedef TBitGridGetCellValueAtIndexFn
 *
 * The type of a function that returns the value of the cell at
 * theIndex in theGrid.  The value is the sequence of bits from
 * all nChannels of theGrid.
 */
typedef TCell (*TBitGridGetCellValueAtIndexFn)(struct TBitGrid *theGrid, TGridIndex theIndex);

/*
 * @typedef TBitGridSetCellValueAtIndexFn
 *
 * The type of a function that sets the value of the cell at
 * theIndex in theGrid.  The sequence of bits in theValue are split
 * and all nChannels of theGrid set according to their on/off values.
 */
typedef void (*TBitGridSetCellValueAtIndexFn)(struct TBitGrid *theGrid, TGridIndex theIndex, TCell theValue);

/*
 * @typedef TBitGridStorage
 *
 * Opaque pointer to a data structure that represents the underlying
 * storage associated with a TBitGrid.
 */
typedef union TBitGridChannelPtr * TBitGridStorage;

/*
 * @typedef TBitGrid
 *
 * Semi-opaque data structure the contains a TBitGrid object created by
 * the TBitGridCreate() function.
 */
typedef struct TBitGrid {
    TBitGridDimensions  dimensions;
    TBitGridStorage     grid;
    struct {
        TBitGridGetCellValueAtIndexFn   getCellValueAtIndex;
        TBitGridSetCellValueAtIndexFn   setCellValueAtIndex;
    } callbacks;
} TBitGrid;

/*
 * @function TBitGridMakeGridPosWithIndex
 *
 * Given a bit grid and word/bit indices, initialize and return the
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
 * corresponding grid index.  Negative values for i and j will not
 * produce useful results.
 */
static inline TGridIndex
TBitGridMakeGridIndexWithPos(
    TBitGrid    *bitGrid,
    int         i,
    int         j
)
{
    unsigned int    offsetH = j * bitGrid->dimensions.nWordsPerRow;
    TGridIndex      I = {
                        .W = offsetH + i / bitGrid->dimensions.nBitsPerWord,
                        .b = i % bitGrid->dimensions.nBitsPerWord
                    };
    return I;
}

/*
 * @function TBitGridPosToIndex
 *
 * Given a game board and grid position P, initialize and return
 * the corresponding grid index.  Negative values for P.i and P.j
 *  will not produce useful results.
 */
static inline TGridIndex
TBitGridPosToIndex(
    TBitGrid        *bitGrid,
    TGridPos        P
)
{
    int    offsetH = P.j * bitGrid->dimensions.nWordsPerRow;
    TGridIndex      I = {
                        .W = offsetH + P.i / bitGrid->dimensions.nBitsPerWord,
                        .b = P.i % bitGrid->dimensions.nBitsPerWord
                    };
    return I;
}

/*
 * @function TBitGridIndexToPos
 *
 * Given a game board and grid index I, initialize and return
 * the corresponding grid position.
 */
static inline TGridPos
TBitGridIndexToPos(
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

/*
 * @function TBitGridGetValueAtIndex
 *
 * Given a bit grid and grid index I, return the value (across all
 * channels) at that location.
 */
static inline TCell
TBitGridGetValueAtIndex(
    TBitGrid        *bitGrid,
    TGridIndex      I
)
{
    return bitGrid->callbacks.getCellValueAtIndex(bitGrid, I);
}

/*
 * @function TBitGridSetValueAtIndex
 *
 * Given a bit grid and grid index I, set the bits across all
 * channels at that location using the sequence of bits in value.
 */
static inline void
TBitGridSetValueAtIndex(
    TBitGrid        *bitGrid,
    TGridIndex      I,
    TCell           value
)
{
    return bitGrid->callbacks.setCellValueAtIndex(bitGrid, I, value);
}

/*
 * @function TBitGridGetValueAtPosition
 *
 * Convenience function that converts the bit grid position P to the
 * corresponing grid index and passes that index to the bitGrid get
 * cell value function.
 */
static inline TCell
TBitGridGetValueAtPosition(
    TBitGrid        *bitGrid,
    TGridPos        P
)
{
    return bitGrid->callbacks.getCellValueAtIndex(bitGrid, TBitGridPosToIndex(bitGrid, P));
}

/*
 * @function TBitGridSetValueAtPosition
 *
 * Convenience function that converts the bit grid position P to the
 * corresponing grid index and passes that index (and value) to the bitGrid
 * set cell value function.
 */
static inline void
TBitGridSetValueAtPosition(
    TBitGrid        *bitGrid,
    TGridPos        P,
    TCell           value
)
{
    return bitGrid->callbacks.setCellValueAtIndex(bitGrid, TBitGridPosToIndex(bitGrid, P), value);
}

enum {
    TBitGridWordSizeDefault = 0,
    TBitGridWordSizeForce8Bit,
    TBitGridWordSizeForce16Bit,
    TBitGridWordSizeForce32Bit,
    TBitGridWordSizeForce64Bit
};

typedef unsigned int TBitGridWordSize;

/*
 * @function TBitGridCreate
 *
 * Allocate a new TBitGrid instance given the width (w) and height (h)
 * and anywhere from 1 to 8 channels (nChannels).
 *
 * Returns NULL on failure, otherwise the returned TBitGrid pointer is owned
 * by the caller and should eventually be deallocated using the TBitGridDestroy()
 * function.
 */
TBitGrid* TBitGridCreate(TBitGridWordSize wordSize, unsigned int nChannels, unsigned int w, unsigned int h);


/*
 * @function TBitGridDestroy
 *
 * Deallocate a TBitGrid instance.
 */
void TBitGridDestroy(TBitGrid* bitGrid);

void TBitGridScroll(TBitGrid *bitGrid);
void TBitGridClearLines(TBitGrid *bitGrid, unsigned int jLow, unsigned int jHigh);

void TBitGridFill(TBitGrid *bitGrid, TCell value);


uint16_t TBitGridExtract4x4AtPosition(TBitGrid *bitGrid, unsigned int channelIdx, TGridPos P);

void TBitGridChannelSummary(TBitGrid *bitGrid, unsigned int channelIdx);


struct TBitGridIterator;

typedef bool (*TBitGridIteratorNextFn)(struct TBitGridIterator *iterator, TGridPos *outP, TCell *value);
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

TBitGridIterator* TBitGridIteratorCreate(TBitGrid *bitGrid, TCell channelMask);

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
    TCell               *value
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
