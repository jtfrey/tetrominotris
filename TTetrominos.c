/*	TTetrominos.c
	Copyright (c) 2024, J T Frey
*/

#include "TTetrominos.h"
                    
const uint64_t  TTetrominos[TTetrominosCount] = {
                        0x0660066006600660,     // Square
                        0x222200F044440F00,     // Straight
                        0x2620027046400E40,     // T
                        0x4620036046200360,     // Skew #1
                        0x2640063026400630,     // Skew #2
                        0x4460074062200170,     // L #1
                        0x2260047064400710      // L #2
                    };

//

void
TTetrominoSummary(
    uint64_t    fullTetromino
)
{
    static uint8_t      tBlockTop[10] = { 0xE2, 0x94, 0x8F, 0xE2, 0x94, 0x81, 0xE2, 0x94, 0x93, '\0' };
    static uint8_t      tBlockBot[10] = { 0xE2, 0x94, 0x97, 0xE2, 0x94, 0x81, 0xE2, 0x94, 0x9B, '\0' };
    static uint8_t      tEmpty[10] = { 0xE2, 0x96, 0x91, 0xE2, 0x96, 0x91, 0xE2, 0x96, 0x91, '\0' };
    unsigned int        i, j, k;
    uint64_t            row, rowMask = 0x000F000F000F000FULL, aggrColMask = 0x0001000100010001ULL;
    
    j = 4;
    // Loop over rows
    while ( j-- ) {
        char            tile1[16 * 10 + 3 * 2 + 1], *tile1Ptr = tile1;
        char            tile2[16 * 10 + 3 * 2 + 1], *tile2Ptr = tile2;
        int             tile1Len = sizeof(tile1);
        int             tile2Len = sizeof(tile2);
        uint64_t        row = fullTetromino & rowMask;
        uint64_t        colMask = aggrColMask & 0x000000000000FFFFULL;
        
        // Prep for the next row:
        rowMask <<= 4;
        aggrColMask <<= 4;
        
        // Loop over each orientation
        k = 4;
        while ( k-- ) {
            int         l;
            
            // Loop over each column of the orientation:
            i = 4;
            while ( i-- ) {
                if ( row & colMask ) {
                    l = snprintf(tile1Ptr, tile1Len, "%s", tBlockTop);
                    tile1Ptr += l;
                    tile1Len -= l;
                    l = snprintf(tile2Ptr, tile2Len, "%s", tBlockBot);
                    tile2Ptr += l;
                    tile2Len -= l;
                } else {
                    l = snprintf(tile1Ptr, tile1Len, "%s", tEmpty);
                    tile1Ptr += l;
                    tile1Len -= l;
                    l = snprintf(tile2Ptr, tile2Len, "%s", tEmpty);
                    tile2Ptr += l;
                    tile2Len -= l;
                }
                colMask <<= 1;
            }
            // Skip ahead to next orientation:
            colMask <<= 12;
            if ( colMask ) {
                l = snprintf(tile1Ptr, tile1Len, "  ");
                tile1Ptr += l;
                tile1Len -= l;
                l = snprintf(tile2Ptr, tile2Len, "  ");
                tile2Ptr += l;
                tile2Len -= l;
            }
        }
        // Print the two rows of text we accumulated:
        printf("    %s\n", tile1);
        printf("    %s\n", tile2);
    }                           
}

//

void
TTetrominoOrientationSummary(
    uint16_t    tetromino
)
{
    static uint8_t      tBlockTop[10] = { 0xE2, 0x94, 0x8F, 0xE2, 0x94, 0x81, 0xE2, 0x94, 0x93, '\0' };
    static uint8_t      tBlockBot[10] = { 0xE2, 0x94, 0x97, 0xE2, 0x94, 0x81, 0xE2, 0x94, 0x9B, '\0' };
    static uint8_t      tEmpty[10] = { 0xE2, 0x96, 0x91, 0xE2, 0x96, 0x91, 0xE2, 0x96, 0x91, '\0' };
    unsigned int        i, j;

    j = 4;
    // Loop over rows
    while ( j-- ) {
        char            tile1[4 * 10 + 1], *tile1Ptr = tile1;
        char            tile2[4 * 10 + 1], *tile2Ptr = tile2;
        int             tile1Len = sizeof(tile1);
        int             tile2Len = sizeof(tile2);
        
        i = 4;
        // Loop over columns
        while ( i-- ) {
            int         l;
            
            if ( tetromino & 0x0001 ) {
                l = snprintf(tile1Ptr, tile1Len, "%s", tBlockTop);
                tile1Ptr += l;
                tile1Len -= l;
                l = snprintf(tile2Ptr, tile2Len, "%s", tBlockBot);
                tile2Ptr += l;
                tile2Len -= l;
            } else {
                l = snprintf(tile1Ptr, tile1Len, "%s", tEmpty);
                tile1Ptr += l;
                tile1Len -= l;
                l = snprintf(tile2Ptr, tile2Len, "%s", tEmpty);
                tile2Ptr += l;
                tile2Len -= l;
            }
            // Shift the tetromino so that 0x0001 will always mask the
            // next bit on the next iteration:
            tetromino >>= 1;
        }
        // Print the two rows of text we accumulated:
        printf("    %s\n", tile1);
        printf("    %s\n", tile2);
    }                           
}

