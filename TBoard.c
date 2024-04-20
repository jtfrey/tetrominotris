
#include "TBoard.h"

//

bool
__TBoardGetValueAtGridIndex8(
    TBoard      *board,
    TGridIndex  I
)
{
    return ((board->grid.b8[I.W]) & (1 << I.b)) != 0;
}
void
__TBoardSetValueAtGridIndex8(
    TBoard      *board,
    TGridIndex  I,
    bool        value
)
{
    if ( value )
        board->grid.b8[I.W] |= (1 << I.b);
    else
        board->grid.b8[I.W] &= ~(1 << I.b);
}
bool
__TBoardIteratorNext8(
    TBoardIterator  *iterator,
    bool            *value
)
{
    if ( iterator->rowIdx < iterator->nRows ) {
        if ( iterator->isStarted ) {
            if ( ++iterator->i >= iterator->nBitsPerRow ) {
                iterator->i = 0;
                iterator->j++;
            }
        } else {
            iterator->isStarted = true;
        }
        *value = *iterator->grid.b8 & (1 << (iterator->bitIdx++ % 8));
        if ( iterator->bitIdx >= iterator->nBitsPerRow ) {
            iterator->grid.b8++;
            iterator->rowIdx++;
            iterator->bitIdx = 0;
        } else if ( (iterator->bitIdx % 8) == 0 ) {
            iterator->grid.b8++;
        }
        return true;
    }
    return false;
}
bool
__TBoardIteratorNextFullRow8(
    TBoardIterator  *iterator,
    unsigned int    *row
)
{
    while ( iterator->rowIdx < iterator->nRows ) {
        unsigned int    nWholeWords, nPartialWordBits;
        
        if ( iterator->isStarted ) {
            iterator->j++;
            iterator->rowIdx++;
        } else {
            iterator->isStarted = true;
        }
        
        // Examine all whole words in the row:
        nWholeWords = iterator->nBitsPerRow / 8;
        nPartialWordBits = iterator->nBitsPerRow % 8;
        while ( nWholeWords ) {
            if ( *iterator->grid.b8++ != 0xFF ) break;
            nWholeWords--;
        }
        if ( nWholeWords == 0 ) {
            // Examine any partial word:
            uint8_t         mask = (1 << nPartialWordBits) - 1;
            
            if ( (nPartialWordBits == 0) || ((*iterator->grid.b8++ & mask) == mask) ) {
                *row = iterator->j;
                return true;
            }
        } else {
            iterator->grid.b8 += nWholeWords + (nPartialWordBits != 0);
        }
    }
    return false;
}
void
__TBoardIteratorInit8(
    TBoard          *board,
    TBoardIterator  *iterator
)
{
    iterator->nextFn = __TBoardIteratorNext8;
    iterator->nextFullRowFn = __TBoardIteratorNextFullRow8;
}

//

bool
__TBoardGetValueAtGridIndex16(
    TBoard      *board,
    TGridIndex  I
)
{
    return ((board->grid.b16[I.W]) & (1 << I.b)) != 0;
}
void
__TBoardSetValueAtGridIndex16(
    TBoard      *board,
    TGridIndex  I,
    bool        value
)
{
    if ( value )
        board->grid.b16[I.W] |= (1 << I.b);
    else
        board->grid.b16[I.W] &= ~(1 << I.b);
}
bool
__TBoardIteratorNext16(
    TBoardIterator  *iterator,
    bool            *value
)
{
    if ( iterator->rowIdx < iterator->nRows ) {
        if ( iterator->isStarted ) {
            if ( ++iterator->i >= iterator->nBitsPerRow ) {
                iterator->i = 0;
                iterator->j++;
            }
        } else {
            iterator->isStarted = true;
        }
        *value = *iterator->grid.b16 & (1 << (iterator->bitIdx++ % 16));
        if ( iterator->bitIdx >= iterator->nBitsPerRow ) {
            iterator->grid.b16++;
            iterator->rowIdx++;
            iterator->bitIdx = 0;
        } else if ( (iterator->bitIdx % 16) == 0 ) {
            iterator->grid.b16++;
        }
        return true;
    }
    return false;
}
bool
__TBoardIteratorNextFullRow16(
    TBoardIterator  *iterator,
    unsigned int    *row
)
{
    while ( iterator->rowIdx < iterator->nRows ) {
        unsigned int    nWholeWords, nPartialWordBits;
        
        if ( iterator->isStarted ) {
            iterator->j++;
            iterator->rowIdx++;
        } else {
            iterator->isStarted = true;
        }
        
        // Examine all whole words in the row:
        nWholeWords = iterator->nBitsPerRow / 16;
        nPartialWordBits = iterator->nBitsPerRow % 16;
        while ( nWholeWords ) {
            if ( *iterator->grid.b16++ != 0xFFFF ) break;
            nWholeWords--;
        }
        if ( nWholeWords == 0 ) {
            // Examine any partial word:
            uint8_t         mask = (1 << nPartialWordBits) - 1;
            
            if ( (nPartialWordBits == 0) || ((*iterator->grid.b16++ & mask) == mask) ) {
                *row = iterator->j;
                return true;
            }
        } else {
            iterator->grid.b16 += nWholeWords + (nPartialWordBits != 0);
        }
    }
    return false;
}
void
__TBoardIteratorInit16(
    TBoard          *board,
    TBoardIterator  *iterator
)
{
    iterator->nextFn = __TBoardIteratorNext16;
    iterator->nextFullRowFn = __TBoardIteratorNextFullRow16;
}

//

bool
__TBoardGetValueAtGridIndex32(
    TBoard      *board,
    TGridIndex  I
)
{
    return ((board->grid.b32[I.W]) & (1 << I.b)) != 0;
}
void
__TBoardSetValueAtGridIndex32(
    TBoard      *board,
    TGridIndex  I,
    bool        value
)
{
    if ( value )
        board->grid.b32[I.W] |= (1 << I.b);
    else
        board->grid.b32[I.W] &= ~(1 << I.b);
}
bool
__TBoardIteratorNext32(
    TBoardIterator  *iterator,
    bool            *value
)
{
    if ( iterator->rowIdx < iterator->nRows ) {
        if ( iterator->isStarted ) {
            if ( ++iterator->i >= iterator->nBitsPerRow ) {
                iterator->i = 0;
                iterator->j++;
            }
        } else {
            iterator->isStarted = true;
        }
        *value = *iterator->grid.b32 & (1 << (iterator->bitIdx++ % 32));
        if ( iterator->bitIdx >= iterator->nBitsPerRow ) {
            iterator->grid.b32++;
            iterator->rowIdx++;
            iterator->bitIdx = 0;
        } else if ( (iterator->bitIdx % 32) == 0 ) {
            iterator->grid.b32++;
        }
        return true;
    }
    return false;
}
bool
__TBoardIteratorNextFullRow32(
    TBoardIterator  *iterator,
    unsigned int    *row
)
{
    while ( iterator->rowIdx < iterator->nRows ) {
        unsigned int    nWholeWords, nPartialWordBits;
        
        if ( iterator->isStarted ) {
            iterator->j++;
            iterator->rowIdx++;
        } else {
            iterator->isStarted = true;
        }
        
        // Examine all whole words in the row:
        nWholeWords = iterator->nBitsPerRow / 32;
        nPartialWordBits = iterator->nBitsPerRow % 32;
        while ( nWholeWords ) {
            if ( *iterator->grid.b32++ != 0xFFFFFFFF ) break;
            nWholeWords--;
        }
        if ( nWholeWords == 0 ) {
            // Examine any partial word:
            uint8_t         mask = (1 << nPartialWordBits) - 1;
            
            if ( (nPartialWordBits == 0) || ((*iterator->grid.b32++ & mask) == mask) ) {
                *row = iterator->j;
                return true;
            }
        } else {
            iterator->grid.b32 += nWholeWords + (nPartialWordBits != 0);
        }
    }
    return false;
}
void
__TBoardIteratorInit32(
    TBoard          *board,
    TBoardIterator  *iterator
)
{
    iterator->nextFn = __TBoardIteratorNext32;
    iterator->nextFullRowFn = __TBoardIteratorNextFullRow32;
}

//

bool
__TBoardGetValueAtGridIndex64(
    TBoard      *board,
    TGridIndex  I
)
{
    return ((board->grid.b64[I.W]) & (1 << I.b)) != 0;
}
void
__TBoardSetValueAtGridIndex64(
    TBoard      *board,
    TGridIndex  I,
    bool        value
)
{
    if ( value )
        board->grid.b64[I.W] |= (1 << I.b);
    else
        board->grid.b64[I.W] &= ~(1 << I.b);
}
bool
__TBoardIteratorNext64(
    TBoardIterator  *iterator,
    bool            *value
)
{
    if ( iterator->rowIdx < iterator->nRows ) {
        if ( iterator->isStarted ) {
            if ( ++iterator->i >= iterator->nBitsPerRow ) {
                iterator->i = 0;
                iterator->j++;
            }
        } else {
            iterator->isStarted = true;
        }
        *value = *iterator->grid.b64 & (1 << (iterator->bitIdx++ % 64));
        if ( iterator->bitIdx >= iterator->nBitsPerRow ) {
            iterator->grid.b64++;
            iterator->rowIdx++;
            iterator->bitIdx = 0;
        } else if ( (iterator->bitIdx % 64) == 0 ) {
            iterator->grid.b64++;
        }
        return true;
    }
    return false;
}
bool
__TBoardIteratorNextFullRow64(
    TBoardIterator  *iterator,
    unsigned int    *row
)
{
    while ( iterator->rowIdx < iterator->nRows ) {
        unsigned int    nWholeWords, nPartialWordBits;
        
        if ( iterator->isStarted ) {
            iterator->j++;
            iterator->rowIdx++;
        } else {
            iterator->isStarted = true;
        }
        
        // Examine all whole words in the row:
        nWholeWords = iterator->nBitsPerRow / 64;
        nPartialWordBits = iterator->nBitsPerRow % 64;
        while ( nWholeWords ) {
            if ( *iterator->grid.b64++ != 0xFFFFFFFFFFFFFFFF ) break;
            nWholeWords--;
        }
        if ( nWholeWords == 0 ) {
            // Examine any partial word:
            uint8_t         mask = (1 << nPartialWordBits) - 1;
            
            if ( (nPartialWordBits == 0) || ((*iterator->grid.b64++ & mask) == mask) ) {
                *row = iterator->j;
                return true;
            }
        } else {
            iterator->grid.b64 += nWholeWords + (nPartialWordBits != 0);
        }
    }
    return false;
}
void
__TBoardIteratorInit64(
    TBoard          *board,
    TBoardIterator  *iterator
)
{
    iterator->nextFn = __TBoardIteratorNext64;
    iterator->nextFullRowFn = __TBoardIteratorNextFullRow64;
}

//

TBoard*
TBoardCreate(
    unsigned int    w,
    unsigned int    h
)
{
    TBoard          *newBoard = NULL;
    unsigned int    nBits, nWords, nWordsPerRow;
    
    switch ( (w - 1) / 8 ) {
        case 0:
            nBits = 8;
            nWordsPerRow = 1;
            break;
        case 1:
            nBits = 16;
            nWordsPerRow = 1;
            break;
        case 3:
            nBits = 32;
            nWordsPerRow = 1;
            break;
        case 4:
        case 5:
        case 6:
        case 7:
            nBits = 64;
            nWordsPerRow = 1;
            break;
        default: {
            struct {
                uint8_t         nBits;
                unsigned int    nWord;
                unsigned int    nExtraBits;
            } swap, byWordSize[4] = {
                                        {  8,  (w + 7) / 8,  (((w + 7) / 8) << 3) - w },
                                        { 16, (w + 15) / 16, (((w + 15) / 16) << 4) - w },
                                        { 32, (w + 31) / 32, (((w + 31) / 32) << 5) - w },
                                        { 64, (w + 63) / 64, (((w + 63) / 64) << 6) - w },
                                    };
#define COMPARE_AND_SWAP(I,J) \
            if ( (byWordSize[I].nExtraBits > byWordSize[J].nExtraBits) || \
                 ((byWordSize[I].nExtraBits == byWordSize[J].nExtraBits) && \
                  (byWordSize[I].nBits < byWordSize[J].nBits)) ) \
            { \
                swap = byWordSize[I]; byWordSize[I] = byWordSize[J]; byWordSize[J] = swap; \
            }
            COMPARE_AND_SWAP(0,1);
            COMPARE_AND_SWAP(2,3);
            COMPARE_AND_SWAP(0,2);
            COMPARE_AND_SWAP(1,3);
            COMPARE_AND_SWAP(1,2);
#undef COMPARE_AND_SWAP
#ifdef TBOARD_DEBUG
            printf("%2hhu %4u %4u\n", byWordSize[0].nBits, byWordSize[0].nWord, byWordSize[0].nExtraBits);
            printf("%2hhu %4u %4u\n", byWordSize[1].nBits, byWordSize[1].nWord, byWordSize[1].nExtraBits);
            printf("%2hhu %4u %4u\n", byWordSize[2].nBits, byWordSize[2].nWord, byWordSize[2].nExtraBits);
            printf("%2hhu %4u %4u\n", byWordSize[3].nBits, byWordSize[3].nWord, byWordSize[3].nExtraBits);
#endif
            nBits = byWordSize[0].nBits;
            nWordsPerRow = byWordSize[0].nWord;
            break;
        }
    }
    nWords = nWordsPerRow * h;

    newBoard = (TBoard*)malloc(sizeof(TBoard) + nWords * (nBits / 8));
    if ( newBoard ) {
        newBoard->w = w;
        newBoard->h = h;
        newBoard->nBits = nBits;
        newBoard->nBytesPerWord = nBits / 8;
        newBoard->nWords = nWords;
        newBoard->nWordsPerRow = nWordsPerRow;
        
        // If we set one pointer in the grid union, they're all set:
        newBoard->grid.b8 = (uint8_t*)((void*)newBoard + sizeof(TBoard));
        
        // Zero the board:
        memset((void*)newBoard->grid.b8, 0, newBoard->nBytesPerWord * newBoard->nWords);
        
        // Set the callbacks:
        switch ( nBits ) {
            case 8:
                newBoard->callbacks.getValueAtIndex = __TBoardGetValueAtGridIndex8;
                newBoard->callbacks.setValueAtIndex = __TBoardSetValueAtGridIndex8;
                newBoard->callbacks.iteratorInit = __TBoardIteratorInit8;
                break;
            case 16:
                newBoard->callbacks.getValueAtIndex = __TBoardGetValueAtGridIndex16;
                newBoard->callbacks.setValueAtIndex = __TBoardSetValueAtGridIndex16;
                newBoard->callbacks.iteratorInit = __TBoardIteratorInit16;
                break;
            case 32:
                newBoard->callbacks.getValueAtIndex = __TBoardGetValueAtGridIndex32;
                newBoard->callbacks.setValueAtIndex = __TBoardSetValueAtGridIndex32;
                newBoard->callbacks.iteratorInit = __TBoardIteratorInit32;
                break;
            case 64:
                newBoard->callbacks.getValueAtIndex = __TBoardGetValueAtGridIndex64;
                newBoard->callbacks.setValueAtIndex = __TBoardSetValueAtGridIndex64;
                newBoard->callbacks.iteratorInit = __TBoardIteratorInit64;
                break;
        }
#ifdef TBOARD_DEBUG
    printf("(w,h) = (%u,%u), nBits = %u, nWords = %u, nWordsPerRow = %u, nBytesPerWord = %u\n",
            newBoard->w, newBoard->h, newBoard->nBits, newBoard->nWords, newBoard->nWordsPerRow, newBoard->nBytesPerWord
        );
#endif
    }
    return newBoard;
}

//

void
TBoardSummary(
    TBoard          *board
)
{
    TBoardIterator  gameIterator = TBoardIteratorMake(board);
    unsigned int    i, j = board->h;
    
    printf("\n");
    while ( j-- ) {
        //
        // The Unicode big box uses 3 bytes for each character in UTF-8, so
        // we need to reserve 9 bytes per block per line:
        //
        static uint8_t      tBlockTop[10] = { 0xE2, 0x94, 0x8F, 0xE2, 0x94, 0x81, 0xE2, 0x94, 0x93, '\0' };
        static uint8_t      tBlockBot[10] = { 0xE2, 0x94, 0x97, 0xE2, 0x94, 0x81, 0xE2, 0x94, 0x9B, '\0' };
        char                line1[board->w * 9 + 1], *line1Ptr = line1;
        char                line2[board->w * 9 + 1], *line2Ptr = line2;
        int                 line1Len = sizeof(line1), line2Len = sizeof(line2);
        
        i = board->w;
        while ( i-- ) {
            int     l;
            bool    value;
            
            TBoardIteratorNext(&gameIterator, &value);
            if ( value ) {
                l = snprintf(line1Ptr, line1Len, "%s", (const char*)tBlockTop); line1Len -= l; line1Ptr += l;
                l = snprintf(line2Ptr, line2Len, "%s", (const char*)tBlockBot); line2Len -= l; line2Ptr += l;
            } else {
                l = snprintf(line1Ptr, line1Len, "   "); line1Len -= l; line1Ptr += l;
                l = snprintf(line2Ptr, line2Len, "   "); line2Len -= l; line2Ptr += l;
            }
        }
        printf("%s\n%s\n", line1, line2);
    }
}

//

void
TBoardScroll(
    TBoard          *board
)
{
    size_t          nBytesMove = (board->nWords - board->nWordsPerRow) * board->nBytesPerWord;
    size_t          nBytesSet = board->nWordsPerRow * board->nBytesPerWord;
    void            *src = (void*)board->grid.b8, *dst = src + board->nWordsPerRow * board->nBytesPerWord;
    
    memmove(dst, src, nBytesMove);
    memset(src, 0, nBytesSet);
}

//

void
TBoardClearLines(
    TBoard          *board,
    unsigned int    jLow,
    unsigned int    jHigh
)
{
    if ( jHigh < jLow ) return;
    
    if ( jLow >= board->h ) jLow = board->h - 1;
    if ( jHigh >= board->h ) jHigh = board->h - 1;
    
    // Is it the entire board?
    if ( (jLow == 0) && (jHigh >= board->h - 1) ) {
        memset((void*)board->grid.b8, 0, board->nWords * board->nBytesPerWord);
    } else {
        if ( jLow == 0 ) {
            // The region starts at the top row and extends down through the
            // jHigh row.  It's just a memset:
            memset((void*)board->grid.b8, 0, board->nWordsPerRow * board->nBytesPerWord * (jHigh - jLow + 1));
        } else {
            void        *dst = (void*)board->grid.b8 + (board->nWordsPerRow * board->nBytesPerWord * ((jHigh + 1) - jLow));
            
            // We will start by moving the leading rows (up to jLow) down above
            // jHigh:
            memmove(dst, (void*)board->grid.b8, board->nWordsPerRow * board->nBytesPerWord * jLow);
            
            // Then we have to zero-out everything up to dst:
            memset((void*)board->grid.b8, 0, dst - (void*)board->grid.b8);
        }
    }
}

//
