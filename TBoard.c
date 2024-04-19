
#include "TBoard.h"

void
TBoardSummary(
    TBoard          *board
)
{
    uint32_t        *grid = board->grid, mask = 0x00000001;
    unsigned int    b = 0;
    unsigned int    w, h;
    
    h = 0;
    printf("\n");
    while ( h < board->h ) {
        //
        // The Unicode big box uses 3 bytes for each character in UTF-8, so
        // we need to reserve 9 bytes per block per line:
        //
        static uint8_t      tBlockTop[10] = { 0xE2, 0x94, 0x8F, 0xE2, 0x94, 0x81, 0xE2, 0x94, 0x93, '\0' };
        static uint8_t      tBlockBot[10] = { 0xE2, 0x94, 0x97, 0xE2, 0x94, 0x81, 0xE2, 0x94, 0x9B, '\0' };
        char                line1[board->w * 9 + 1], *line1Ptr = line1;
        char                line2[board->w * 9 + 1], *line2Ptr = line2;
        int                 line1Len = sizeof(line1), line2Len = sizeof(line2);
        
        w = 0;
        while ( w < board->w ) {
            int     l;
            
            if ( *grid & mask ) {
                l = snprintf(line1Ptr, line1Len, "%s", (const char*)tBlockTop); line1Len -= l; line1Ptr += l;
                l = snprintf(line2Ptr, line2Len, "%s", (const char*)tBlockBot); line2Len -= l; line2Ptr += l;
            } else {
                l = snprintf(line1Ptr, line1Len, "   "); line1Len -= l; line1Ptr += l;
                l = snprintf(line2Ptr, line2Len, "   "); line2Len -= l; line2Ptr += l;
            }
            b++;
            mask <<= 1;
            if ( b >= 32 ) {
                grid++;
                b = 0;
                mask = 0x00000001;
            }
            w++;
        }
        printf("%s\n%s\n", line1, line2);
        h++;
    }
}

//

void
TBoardScroll(
    TBoard          *board
)
{
    // Determine how many full words we need to shift versus how many
    // bits within those words:
    unsigned int        nWord = (board->w / 32), nBit = (board->w % 32);
    
    // Scrolling is accomplished by shifting the entire bit vector
    // right by the width of the board.  Let's start by moving as many
    // whole words as we need to:
    if ( nWord > 0 ) {
        uint32_t        *src, *dst;
        
        // Do a memmove() and reset any leading byte(s) to zero:
        src = &board->grid[0];
        dst = &board->grid[nWord];
        memmove(dst, src, (board->nGridWords - nWord) * sizeof(uint32_t));
        memset(src, 0, nWord * sizeof(uint32_t));
    }
    // If the width is on a word boundary, we're actually done now.
    // Otherwise, begin shifting by the bit count:
    if ( nBit > 0 ) {
        // Our source is the location of "nWord" in the array:
        uint32_t        *src = &board->grid[nWord];
        uint32_t        carryover = 0x00000000;
        uint32_t        willMoveMask = ~((1 << (32 - nBit)) - 1);
        unsigned int    count = board->nGridWords - nWord;
        
        while ( count-- ) {
            // These are the bits which will get OR'ed into the next
            // word (rolled off the right):
            uint32_t    willMove = (*src & willMoveMask) >> (32 - nBit);
            
            // Shift the source and OR the carryover:
            *src = (*src << nBit) | carryover;
            
            // Hold the willMove as carryover for next iteration:
            carryover = willMove;
            src++;
        }
    }
}

//

void
TBoardClearLines(
    TBoard          *board,
    unsigned int    jLow,
    unsigned int    jHigh
)
{
    unsigned int    nLines = (jHigh - jLow) + 1, nBits = nLines * board->w;
    unsigned int    dW = nBits / 32, db = nBits % 32;
    TGridIndex      IStart = TBoardGridIndexMakeWithPos(board, 0, jLow);
    TGridIndex      IEnd = TBoardGridIndexMakeWithPos(board, 0, jHigh + 1);
    
    // IStart and IEnd now contain the starting and ending word and bit
    // indices:  ISW, ISb, IEW, IEb.
    //
    // The distance data is being moved is dW and db.
    //
    // The value IEb
    //   
    
}

//
