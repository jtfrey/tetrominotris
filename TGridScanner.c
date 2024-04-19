
#include "TGridScanner.h"

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
void
TGridScannerMoveToPos(
    TGridScanner    *scanner,
    TGridPos        newPos
)
{
    int             dW = 0, db = 0;
    
    if ( newPos.j != scanner->j ) {
        // Moving by this many whole lines:
        int         shift = scanner->w * (newPos.j - scanner->j);
        
        if ( shift < 0 ) {
            // Moving backwards by this many bits:
            dW = -((-shift) / 32), db = -((-shift) % 32);
        } else {
            dW = shift / 32, db = shift % 32;
        }
    }
    //printf("dW = %d, db = %d\n", dW, db);
    if ( newPos.i != scanner->i ) {
        // Moving forward or backward on the line:
        int     shift = newPos.i - scanner->i;
        
        if ( shift < 0 ) {
            // Moving backwards:
            dW -= ((-shift) / 32), db -= ((-shift) % 32);
        } else {
            // Moving forwards:
            dW += (shift / 32), db += (shift % 32);
        }
        // Fix bit position carry:
        if ( db >= 32 ) {
            dW++;
            db -= 32;
        } else if ( db <= -32 ) {
            dW--;
            db += 32;
        }
    }
    //printf("dW = %d, db = %d\n", dW, db);
    if ( dW ) {
        // Move the word pointer:
        scanner->focus += dW;
        scanner->W += dW;
        scanner->cellsRemaining -= 32 * dW;
    }
    if ( db ) {
        // Move the mask:
        if ( scanner->mask ) {
            if ( db < 0)
                scanner->mask >>= (-db);
            else
                scanner->mask <<= db;
        }
        scanner->b += db;
        scanner->cellsRemaining -= db;
    }
    scanner->i = newPos.i;
    scanner->j = newPos.j;
    scanner->hasBeenStarted = false;
    //printf("%u %u %u %u %u %08X\n", scanner->i, scanner->j, scanner->W, scanner->b, scanner->cellsRemaining, scanner->mask);
}

