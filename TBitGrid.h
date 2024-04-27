/*	TBitGrid.h
	Copyright (c) 2024, J T Frey
*/

/*!
	@header Bit grids
	A bit grid forms the heart of the game.  The game board is at its most
	basic a 2D grid of single-bit values:  filled or empty.  The bits in a
	row are grouped into words of varying size; the number of bits per word
	can be chosen explicitly (8, 16, 32, or 64) or by a formula meant to
	simultaneously minimize the number of unused bits in the row and maximize
	the number of bits per word.  Based on the typical grid dimensions for
	the game, 16-bit words tend to win out.
	
	Bit 0 of word 0 corresponds with the upper-left corner of the game board.
	Each row consists of enough N-bit words to span the columns of the game
	board (a number of words per row), and each successive row going down the
	game board is offset from the start of the grid by a multiple of the words
	per row.  There thus exists a straightforward mapping between a grid
	position (i,j) and a word index and bit-within-word index (W,b).
	
	Since tetrominoes exist as a 16-bit representation of a 4x4 bitmap, testing
	for collisions is straightfoward.  Given the position of the in-play
	tetromino (i,j), the word/bit index is (W,b).  Extracting the 4 bits at
	(W,b) provides the first row of the 4x4 representation of the grid state;
	the 4 bits from (W + words-per-row,b) provide the second row; etc.  The
	bitwise-AND of the two (4x4) 16-bit values indicates a collision if non-zero:
	
	      tetromino  game board   result
	        ....        ....       ....
	        .##.   &    ....   =   ....
	        .##.        ..#.       ..#.
	        ....        ####       ....
	      0x0660      0xF400     0x0400
    
    In the circumstance that there is no collision, the tetromino can be added
    to the game board by bitwise-OR:
	
	      tetromino  game board   result
	        ....        ....       ....
	        .##.   |    ....   =   .##.
	        .##.        #..#       ####
	        ....        ####       ####
	      0x0660      0xF900     0xFF60
	
	Associating additional information with each grid position is accomplished
	by multiple bit planes.  Operations that manipulate or enumerate the grid
	values can target one or more of the bit planes using bit masks (plane
	0 = 1 << 0, plane 1 = 1 << 1, etc.).  Hit detection may only make use of
	plane 0, but the removal of rows will shift the values in every bit plane.
	(Note that this is how color information is optionally added to the grid
	cells.)
	
	Two enumeration methods are supplied.  Both start at position (0,0) in the
	upper-left corner and proceed across columns and then down rows.  The first
	method returns the value at each grid cell (enumerating all bits is useful
	for drawing the game board on-screen).  The second enumerates rows for
	which every column (bit) is set (detect completed rows by enumerating bit
	0 only).  A row is full when all whole-word values have all bits set (e.g.
	0xFFFF for 16-bit words) and the possible final partial word has the lowest
	N bits set.  If the game board is 16 squares wide, then a full row is
	established by bitwise-AND'ing a single 16-bit word with 0xFFFF.
*/

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

/*
 * @enum TBitGrid word size
 *
 * When creating a TBitGrid instance the default behavior is to choose a word
 * size such that:
 *
 * - for width <= 8, 8-bit words
 * - for 8 < width <= 16, 16-bit words
 * - for 16 < width <= 32, 32-bit words
 * - for 32 < width <= 64, 64-bit words
 * - for 64 < width, the word size that minimizes the number of wasted bits and
 *       maximizes the number of bits-per-word is chosen
 *
 * For the sake of testing etc. the word size can be explicitly chosen with
 * the TBitGridWordSizeForce8Bit et al. values.
 */
enum {
    TBitGridWordSizeDefault = 0,
    TBitGridWordSizeForce8Bit,
    TBitGridWordSizeForce16Bit,
    TBitGridWordSizeForce32Bit,
    TBitGridWordSizeForce64Bit
};

/*
 * @typedef TBitGridWordSize
 *
 * The type of a value from the TBitGrid word size enumeration.
 */
typedef unsigned int TBitGridWordSize;

/*
 * @function TBitGridCreate
 *
 * Allocate a new TBitGrid instance given the width (w) and height (h)
 * and anywhere from 1 to 8 channels (nChannels).  The underlying data
 * array type is dictated by the wordSize.
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

/*
 * @function TBitGridScroll
 *
 * Shift the entire bit grid forward one row, introducing a row of zeroes at
 * the head of the grid.
 */
void TBitGridScroll(TBitGrid *bitGrid);

/*
 * @function TBitGridClearLines
 *
 * Remove the rows [jLow,jHigh] from the bit grid.  Any rows leading up to jLow
 * are scrolled down to fill the gap and rows of zeroes are introduced at the
 * head of the grid.
 */
void TBitGridClearLines(TBitGrid *bitGrid, unsigned int jLow, unsigned int jHigh);

/*
 * @function TBitGridFill
 *
 * Fill every cell of the grid with value.
 */
void TBitGridFill(TBitGrid *bitGrid, TCell value);

/*
 * @function TBitGridExtract4x4AtPosition
 *
 * Starting at the grid position P extract the 4x4 sub-grid of bit values from
 * channel channelIdx:
 *
 *       P  —— i ——>
 *      |   0 .... 3
 *      j   4 ..X. 7    = 0b1111111001000000 = 0xFE40
 *      |   8 .XXX 11
 *      V  12 XXXX 15
 *
 * The extracted 4x4 representation can be collision-tested against a 4x4
 * tetromino by a simple bitwise AND:
 *
 *       P  —— i ——>
 *      |   0 .... 3         0 .XXX 3
 *      j   4 ..X. 7         4 ..X. 7
 *      |   8 .XXX 11        8 .... 11
 *      V  12 XXXX 15       12 .... 15
 *
 *           0xFE40    &     0x004E        = 0x0040  => collision
 *
 * versus
 *
 *       P  —— i ——>
 *      |   0 .... 3         0 .X.. 3
 *      j   4 ..X. 7         4 XX.. 7
 *      |   8 .XXX 11        8 X... 11
 *      V  12 XXXX 15       12 .... 15
 *
 *           0xFE40    &     0x0132        = 0x0000  => no collision
 */
uint16_t TBitGridExtract4x4AtPosition(TBitGrid *bitGrid, unsigned int channelIdx, TGridPos P);

/*
 * @function TBitGridSet4x4AtPosition
 *
 * Merge the 4x4 sub-grid represented by in4x4 into the bitGrid at grid position
 * P.  Bits are pushed to channel channelIdx.
 *
 *        Location on bitGrid    in4x4
 *
 *       P  —— i ——>            —— i ——>        P  —— i ——>
 *      |   0 .... 3            0 .... 3           0 .... 3
 *      j   4 .... 7            4 XXX. 7     =>    4 XXX. 7    = 0b1111111101110000 = 0xFF70
 *      |   8 X.XX 11           8 .X.. 11          8 XXXX 11
 *      V  12 XXXX 15          12 .... 15         12 XXXX 15
 *
 *          0xFD00       |      0x0270       =>    0xFF70
 */
void TBitGridSet4x4AtPosition(TBitGrid *bitGrid, unsigned int channelIdx, TGridPos P, uint16_t in4x4);

/*
 * @enum TBitGrid channel summary kind
 *
 * - visual:  display the channel as a textual grid pattern
 * - technical:  display all parameters and show the grid data as hexadecimal words
 */
enum {
    TBitGridChannelSummaryKindVisual    = 0,
    TBitGridChannelSummaryKindTechnical
};

/*
 * @typedef TBitGridChannelSummaryKind
 *
 * The type of a TBitGrid channel summary kind
 */
typedef unsigned int TBitGridChannelSummaryKind;

/*
 * @function TBitGridChannelSummary
 *
 * Write a summary of the bitGrid to stdout.  Under the "visual" kind, the
 * grid is drawn to stdout visually as an array of 2x2 character filled/empty
 * blocks.
 *
 * The "technical" mode displays the bitGrid sheerly as property values and
 * an indexed list of hexadecimal words comprising the grid.
 *
 * Bits are extracted from the given channel by channelIdx.
 */
void TBitGridChannelSummary(TBitGrid *bitGrid, unsigned int channelIdx, TBitGridChannelSummaryKind summaryKind);



/* Forward declaration for the sake of declaring the callback
 * functions: */
struct TBitGridIterator;

/*
 * @typedef TBitGridIteratorNextFn
 *
 * The type of a function that iteratively returns the next TBitGrid position
 * (in *outP) and cell value (in *value).  The function should return true if
 * a position was enumerated or false if no more positions remain.
 *
 * The cell value is the bitwise OR of each channel selected by the channelMask
 * passed at iterator creation.
 */
typedef bool (*TBitGridIteratorNextFn)(struct TBitGridIterator *iterator, TGridPos *outP, TCell *value);

/*
 * @typedef TBitGridIteratorNextFullRowFn
 *
 * The type of a function that iteratively returns the next TBitGrid row
 * (in *outJ) for which the value of each channel selected on the channelMask
 * passed at iterator creation is set in every cell in that row.
 */
typedef bool (*TBitGridIteratorNextFullRowFn)(struct TBitGridIterator *iterator, unsigned int *outJ);

/*
 * @typedef TBitGridIterator
 *
 * Data structure that is dynamically-allocated and initialized by the
 * TBitGridIteratorCreate() function in order to iterate of the cells of
 * a TBitGrid.
 *
 * Consumers of the TBitGridIterator functionality should not attempt to
 * alter the contents of this data structure directly.
 */
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

/*
 * @function TBitGridIteratorCreate
 *
 * Allocate a new TBitGridIterator that will enumerate (in the forward direction)
 * the cells in bitGrid.  Only the bit values in the channels selected bt channelMask
 * will be enumerated.
 *
 * The consumer is responsible for eventually deallocating the returned iterator by
 * passing it to TBitGridIteratorDestroy().
 */
TBitGridIterator* TBitGridIteratorCreate(TBitGrid *bitGrid, TCell channelMask);

/*
 * @function TBitGridIteratorDestroy
 *
 * Deallocate a TBitGridIterator.
 */
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
