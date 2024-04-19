
#ifndef __TGRIDSCANNER_H__
#define __TGRIDSCANNER_H__

#include "TBoard.h"

/*
 * @typedef TGridScanner
 *
 * A TGridScanner is used to faciliate the enumeration of game board
 * positions.  Starting at the top-left (0,0) position, the board is
 * walked by width and then by height.
 *
 * A TGridScanner can also be repositioned to an arbitrary position
 * where enumeration will resume on the next call to the
 * TGridScannerNext() function.
 *
 * The data structure does not directly reference the parent TBoard
 * from which is was initialized.  The cellsRemaining and w are
 * initialized accordingly, and the focus field is a pointer to the
 * parent's grid array.
 *
 * It is up to the consumer to NOT alter the state of a TGridScanner
 * or to alter the parent TBoard for the duration of the scan.
 */
typedef struct {
    unsigned int    i, j;
    unsigned int    W, b;
    unsigned int    cellsRemaining, w;
    bool            hasBeenStarted;
    uint32_t        *focus, mask;
} TGridScanner;

/*
 * @function TGridScannerMake
 *
 * Returns a TGridScanner initialized according to the properties
 * of parentBoard and ready to begin scanning at position (0,0).
 */
static inline TGridScanner
TGridScannerMake(
    TBoard          *parentBoard
)
{
    TGridScanner    newScanner = {
                        .w = parentBoard->w,
                        .cellsRemaining = parentBoard->w * parentBoard->h,
                        .i = 0, .j = 0,
                        .W = 0, .b = 0,
                        .hasBeenStarted = false,
                        .focus = &parentBoard->grid[0],
                        .mask = 0x00000001
                    };
    return newScanner;
}

/*
 * @function TGridScannerNext
 *
 * Attempt to move to the next position on the game board and return
 * the value at that position.  If successful, the i, j, B, and b
 * fields of scanner will indicate the cell location and true is
 * returned.
 *
 * If no cells remain to enumerate, false is returned, at which
 * point the scanner is no longer valid.
 */
static inline bool
TGridScannerNext(
    TGridScanner    *scanner,
    bool            *value
)
{
    if ( scanner->cellsRemaining == 0 ) return false;
    if ( scanner->hasBeenStarted ) {
        if ( ++scanner->b >= 32 ) {
            scanner->focus++;
            scanner->W++;
            scanner->b = 0;
            scanner->mask = 0x00000001;
        } else {
            scanner->mask <<= 1;
        }
        if ( ++scanner->i >= scanner->w ) {
            scanner->j++;
            scanner->i = 0;
        }
    } else {
        scanner->hasBeenStarted = true;
    }
    *value = (*scanner->focus & scanner->mask) != 0;
    --scanner->cellsRemaining;
    return true;
}

/*
 * @function TGridScannerMoveToPos
 *
 * Update the scanner so that it will next enumerate the cell at
 * position newPos.
 *
 * No checks of valid dimensioning of the component values of
 * newPos are performed, it is up to the consumer to ensure
 * they are valid.
 */
void TGridScannerMoveToPos(TGridScanner *scanner, TGridPos newPos);

#endif /* __TGRIDSCANNER_H__ */
