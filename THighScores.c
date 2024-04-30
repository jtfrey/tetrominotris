/*	THighScores.h
	Copyright (c) 2024, J T Frey
*/

#include "THighScores.h"

#include <fcntl.h>
#include <sys/stat.h>

#define ENABLE_BIT_REARRANGEMENT


static const unsigned char TCipherFwd[64] = "0AaBbC1cDdEe2FfG"
                                            "gH3hIiJj4KkLlM5m"
                                            "NnOo6PpQqR7rSsTt"
                                            "8UuVvW9wXxYyZz@/";

static const signed char TCipherRev[122 - 47 + 1] = {
                        63, 0, 6, 12, 18, 24, 30, 36, 42, 48, 54, -1, -1, -1, -1, -1,
                        -1, 62, 1, 3, 5, 8, 10, 13, 15, 17, 20, 22, 25, 27, 29, 32,
                        34, 37, 39, 41, 44, 46, 49, 51, 53, 56, 58, 60, -1, -1, -1, -1,
                        -1, -1, 2, 4, 7, 9, 11, 14, 16, 19, 21, 23, 26, 28, 31, 33,
                        35, 38, 40, 43, 45, 47, 50, 52, 55, 57, 59, 61
                    };


static inline void
__TCipher64Encode(
    uint8_t     *inB,
    uint8_t     *outB
)
{
#ifdef ENABLE_BIT_REARRANGEMENT
/*
     00000000  11111111  22222222
    [76543210][76543210][76543210]
    
       012012    012012    012012    012012
    [--000111][--222333][--444555][--666777]

*/
    outB[0] = TCipherFwd[
                ((inB[0] & 0b00000001) << 5) | 
                ((inB[1] & 0b00000001) << 4) | 
                ((inB[2] & 0b00000001) << 3) |
                ((inB[0] & 0b00000010) << 1) | 
                ((inB[1] & 0b00000010)     ) | 
                ((inB[2] & 0b00000010) >> 1)
            ];
    outB[1] = TCipherFwd[
                ((inB[0] & 0b00000100) << 3) | 
                ((inB[1] & 0b00000100) << 2) | 
                ((inB[2] & 0b00000100) << 1) |
                ((inB[0] & 0b00001000) >> 1) | 
                ((inB[1] & 0b00001000) >> 2) | 
                ((inB[2] & 0b00001000) >> 3)
            ];
    outB[2] = TCipherFwd[
                ((inB[0] & 0b00010000) << 1) | 
                ((inB[1] & 0b00010000)     ) | 
                ((inB[2] & 0b00010000) >> 1) |
                ((inB[0] & 0b00100000) >> 3) | 
                ((inB[1] & 0b00100000) >> 4) | 
                ((inB[2] & 0b00100000) >> 5)
            ];
    outB[3] = TCipherFwd[
                ((inB[0] & 0b01000000) >> 1) | 
                ((inB[1] & 0b01000000) >> 2) | 
                ((inB[2] & 0b01000000) >> 3) |
                ((inB[0] & 0b10000000) >> 5) | 
                ((inB[1] & 0b10000000) >> 6) | 
                ((inB[2] & 0b10000000) >> 7)
            ];
#else
    outB[0] = TCipherFwd[inB[0] & 0b00111111];
    outB[1] = TCipherFwd[((inB[0] & 0b11000000) >> 6) | ((inB[1] & 0b00001111) << 2)];
    outB[2] = TCipherFwd[((inB[1] & 0b11110000) >> 4) | ((inB[2] & 0b00000011) << 4)];
    outB[3] = TCipherFwd[((inB[2] & 0b11111100) >> 2)];
#endif
}

static inline bool
__TCipher64Decode(
    uint8_t     *inB,
    uint8_t     *outB
)
{
    uint8_t     tmpB[4];
    
    if ( inB[0] < 47 || inB[0] > 122 ) return false;
    if ( (tmpB[0] = TCipherRev[inB[0] - 47]) < 0 ) return false;
    if ( inB[1] < 47 || inB[1] > 122 ) return false;
    if ( (tmpB[1] = TCipherRev[inB[1] - 47]) < 0 ) return false;
    if ( inB[2] < 47 || inB[2] > 122 ) return false;
    if ( (tmpB[2] = TCipherRev[inB[2] - 47]) < 0 ) return false;
    if ( inB[3] < 47 || inB[3] > 122 ) return false;
    if ( (tmpB[3] = TCipherRev[inB[3] - 47]) < 0 ) return false;

#ifdef ENABLE_BIT_REARRANGEMENT
/*
     00000000  11111111  22222222
    [76543210][76543210][76543210]
    
       012012    012012    012012    012012
    [--000111][--222333][--444555][--666777]

*/
    outB[0] =   ((tmpB[0] >> 5) & 0b00000001) |
                ((tmpB[0] >> 1) & 0b00000010) |
                ((tmpB[1] >> 3) & 0b00000100) |
                ((tmpB[1] << 1) & 0b00001000) |
                ((tmpB[2] >> 1) & 0b00010000) |
                ((tmpB[2] << 3) & 0b00100000) |
                ((tmpB[3] << 1) & 0b01000000) |
                ((tmpB[3] << 5) & 0b10000000);
    outB[1] =   ((tmpB[0] >> 4) & 0b00000001) |
                ((tmpB[0]     ) & 0b00000010) |
                ((tmpB[1] >> 2) & 0b00000100) |
                ((tmpB[1] << 2) & 0b00001000) |
                ((tmpB[2]     ) & 0b00010000) |
                ((tmpB[2] << 4) & 0b00100000) |
                ((tmpB[3] << 2) & 0b01000000) |
                ((tmpB[3] << 6) & 0b10000000);
    outB[2] =   ((tmpB[0] >> 3) & 0b00000001) |
                ((tmpB[0] << 1) & 0b00000010) |
                ((tmpB[1] >> 1) & 0b00000100) |
                ((tmpB[1] << 3) & 0b00001000) |
                ((tmpB[2] << 1) & 0b00010000) |
                ((tmpB[2] << 5) & 0b00100000) |
                ((tmpB[3] << 3) & 0b01000000) |
                ((tmpB[3] << 7) & 0b10000000);
#else
    outB[0] = tmpB[0] | ((tmpB[1] & 0b00000011) << 6);
    outB[1] = ((tmpB[1] & 0b00111100) >> 2) | ((tmpB[2] & 0b00001111) << 4);
    outB[2] = ((tmpB[2] & 0b00110000) >> 4) | ((tmpB[3] & 0b00111111) << 2);
#endif
    return true;
}

//
////
//

static inline bool
__THighScoreHostIsBE(void)
{
    uint32_t    one = 1;
    
    return ( *((uint8_t*)&one) != 1 );
}

static inline bool
__THighScoreHostIsLE(void)
{
    uint32_t    one = 1;
    
    return ( *((uint8_t*)&one) == 1 );
}

static inline uint32_t
__THighScoreHostSwapBEUInt32(
    uint32_t    v
)
{
    if ( __THighScoreHostIsLE() ) {
        uint8_t swap, *vPtr = (uint8_t*)&v;
        
        swap = vPtr[0]; vPtr[0] = vPtr[3]; vPtr[3] = swap;
        swap = vPtr[1]; vPtr[1] = vPtr[2]; vPtr[2] = swap;
    }
    return v;
}

static inline uint64_t
__THighScoreHostSwapBEUInt64(
    uint64_t    v
)
{
    if ( __THighScoreHostIsLE() ) {
        uint8_t swap, *vPtr = (uint8_t*)&v;
        
        swap = vPtr[0]; vPtr[0] = vPtr[7]; vPtr[7] = swap;
        swap = vPtr[1]; vPtr[1] = vPtr[6]; vPtr[6] = swap;
        swap = vPtr[2]; vPtr[2] = vPtr[5]; vPtr[5] = swap;
        swap = vPtr[3]; vPtr[3] = vPtr[4]; vPtr[4] = swap;
    }
    return v;
}

//
////
//

typedef struct THighScores * THighScoresRef;

#ifndef THIGHSCORES_COUNT
#   define THIGHSCORES_COUNT 3
#endif

static const uint64_t THighScoreFileMagicBytes = 0x7273836779826983;    // "HISCORES"

typedef struct {
    uint32_t        score;
    uint32_t        level;
    char            timestamp[20];  // YYYY-MM-DD HH:MM:SS
    char            initials[3];
} THighScore;

typedef struct THighScores {
    uint64_t        magic;
    uint32_t        nScores;
    THighScore      scores[0];
} THighScores;

//

THighScoresRef
THighScoresLoad(
    const char      *filepath
)
{
    THighScores     *highScores = NULL;
    int             fd = open(filepath, O_RDONLY);
    
    if ( fd >= 0 ) {
        struct stat fInfo;
        
        // The file needs to be at least 8 + 4 bytes long.  Encoded, that
        // would be (12 bytes) * (4/3) = 16 bytes.
        if ( (fstat(fd, &fInfo) == 0) && ((fInfo.st_size >= 16) && ((fInfo.st_size % 4) == 0)) ) {
            uint8_t *readBuffer = (uint8_t*)malloc(fInfo.st_size);
            
            if ( readBuffer ) {
                ssize_t nBytesRead = read(fd, readBuffer, fInfo.st_size);
                
                if ( nBytesRead == fInfo.st_size ) {
                    // Decode the data.  The __TCipher64Decode() function can
                    // do it in-place, so we'll just transform the buffer that's
                    // 4/3 oversized into the object:
                    uint8_t     *inB = readBuffer, *outB = readBuffer,
                                *endB = readBuffer + fInfo.st_size;
                    
                    while ( inB < endB ) {
                        if ( ! __TCipher64Decode(inB, outB) ) break;
                        inB += 4; outB += 3;
                    }
                    if ( inB >= endB ) {
                        int         i;
                        
                        highScores = (THighScores*)readBuffer;
                        highScores->magic = __THighScoreHostSwapBEUInt64(highScores->magic);
                        highScores->nScores = __THighScoreHostSwapBEUInt32(highScores->nScores);
                        i = 0;
                        while ( i < highScores->nScores ) {
                            highScores->scores[i].score = __THighScoreHostSwapBEUInt32(highScores->scores[i].score);
                            highScores->scores[i].level = __THighScoreHostSwapBEUInt32(highScores->scores[i].level);
                            i++;
                        }
                    } else {
                        free(readBuffer);
                    }
                } else {
                    free(readBuffer);
                }
            }
        }
        close(fd);
    }
    return highScores;
}

//

THighScoresRef
THighScoresCreate(void)
{
    THighScores     *highScores = (THighScores*)malloc(sizeof(THighScores) + 
                                            THIGHSCORES_COUNT * sizeof(THighScore));
    
    if ( highScores ) {
        int         i;
        
        highScores->magic = THighScoreFileMagicBytes;
        highScores->nScores = THIGHSCORES_COUNT;
        
        highScores->scores[0].score = 999999;
        highScores->scores[0].level = 99;
        strcpy(highScores->scores[0].timestamp, "1977-03-25 00:00:00");
        memcpy(highScores->scores[0].initials, "JTF", 3);
        i = 1;
        while ( i < THIGHSCORES_COUNT ) {
            highScores->scores[i].score = 0;
            highScores->scores[i].level = 0;
            strcpy(highScores->scores[i].timestamp, "0000-00-00 00:00:00");
            memcpy(highScores->scores[i].initials, "???", 3);
            i++;
        }
    }
    return (THighScoresRef)highScores;
}

//

void
THighScoresDestroy(
    THighScoresRef  highScores
)
{
    free((void*)highScores);
}

//

unsigned int
THighScoresGetCount(
    THighScoresRef  highScores
)
{
    return highScores->nScores;
}

//

bool
THighScoresGetRecord(
    THighScoresRef  highScores,
    unsigned int    idx,
    unsigned int    *score,
    unsigned int    *level,
    char            initials[3],
    char            *timestamp,
    int             timestampLen
)
{
    if ( idx < highScores->nScores ) {
        if ( score ) *score = highScores->scores[idx].score;
        if ( level ) *level = highScores->scores[idx].level;
        if ( timestamp && timestampLen ) strncpy(timestamp, highScores->scores[idx].timestamp, timestampLen);
        memcpy(initials, highScores->scores[idx].initials, 3);
        return true;
    }
    return false;
}

//

bool
THighScoresDoesQualify(
    THighScoresRef  highScores,
    unsigned int    score,
    unsigned int    *rank
)
{
    int             i = 0;
    
    while ( i < highScores->nScores ) {
        if ( highScores->scores[i].score < score ) {
            if ( rank ) *rank = i;
            return true;
        }
        i++;
    }
    return false;
}

//

bool
THighScoresRegister(
    THighScoresRef  highScores,
    unsigned int    score,
    unsigned int    level,
    char            initials[3]
)
{
    int             i = 0;
    
    while ( i < highScores->nScores ) {
        if ( highScores->scores[i].score < score ) break;
        i++;
    }
    if ( i < highScores->nScores ) {
        int         j;
        time_t      now = time(NULL);
        
        // i indicates the slot where this score goes; shift
        // i and all lower slots (minus 1) down one position
        // to make room:
        j = highScores->nScores - 1;
        while ( j > i ) {
            highScores->scores[j] = highScores->scores[j - 1];
            j--;
        }
        highScores->scores[i].score = score;
        highScores->scores[i].level = level;
        strftime(highScores->scores[i].timestamp, sizeof(highScores->scores[i].timestamp), "%Y-%m-%d %H:%M:%S", localtime(&now));
        memcpy(highScores->scores[i].initials, initials, 3);
        return true;
    }
    return false;
}

//

bool
THighScoresSave(
    THighScoresRef  highScores,
    const char      *filepath
)
{
    bool            rc = false;
    int             i = 0, iMax = highScores->nScores;
    size_t          origByteSize = sizeof(THighScores) + iMax * sizeof(THighScore), nBytes;
    size_t          encodedByteSize = 4 * ((origByteSize / 3) + ((origByteSize % 3) > 0));
    uint8_t         *origBytes = (uint8_t*)highScores, *encodedBytes = (uint8_t*)malloc(encodedByteSize),
                    *endOrigBytes = origBytes + origByteSize, *outB = encodedBytes;
    
    if ( encodedBytes ) {
        int         fd;
        ssize_t     nBytesWritten;
        
        // Byte-swap everything if necessary:
        if ( __THighScoreHostIsLE() ) {
            highScores->magic = __THighScoreHostSwapBEUInt64(highScores->magic);
            highScores->nScores = __THighScoreHostSwapBEUInt32(highScores->nScores);
            while ( i < iMax ) {
                highScores->scores[i].score = __THighScoreHostSwapBEUInt32(highScores->scores[i].score);
                highScores->scores[i].level = __THighScoreHostSwapBEUInt32(highScores->scores[i].level);
                i++;
            }
        }
        
        // Encode into the buffer:
        nBytes = origByteSize;
        while ( nBytes >= 3 ) {
            __TCipher64Encode(origBytes, outB);
            nBytes -= 3;
            origBytes += 3; outB += 4;
        }
        if ( nBytes ) {
            uint8_t     tmpB[3];
            
            tmpB[0] = *origBytes++;
            if ( nBytes == 1 ) {
                tmpB[1] = tmpB[2] = 0x00;
            }
            else if ( nBytes == 2 ) {
                tmpB[1] = *origBytes++;
                tmpB[2] = 0x00;
            }
            __TCipher64Encode(tmpB, outB);
        }
        
        // Byte-swap everything back if necessary:
        if ( __THighScoreHostIsLE() ) {
            highScores->magic = __THighScoreHostSwapBEUInt64(highScores->magic);
            highScores->nScores = __THighScoreHostSwapBEUInt32(highScores->nScores);
            i = 0;
            while ( i < iMax ) {
                highScores->scores[i].score = __THighScoreHostSwapBEUInt32(highScores->scores[i].score);
                highScores->scores[i].level = __THighScoreHostSwapBEUInt32(highScores->scores[i].level);
                i++;
            }
        }
        
        // Write to the file:
        fd = open(filepath, O_WRONLY | O_CREAT | O_TRUNC, 0666);
        if ( fd ) {
            ssize_t nBytesWritten = write(fd, encodedBytes, encodedByteSize);
            
            rc = ( nBytesWritten == encodedByteSize );
            close(fd);
        }
        free(encodedBytes);
    }
    return rc;
}

//
////
//

#ifdef THIGHSCORES_HELPER_EXE

int
main()
{
    THighScoresRef  highScores = THighScoresLoad("tetrominotris.hiscores");
    
    if ( ! highScores ) {
        highScores = THighScoresCreate();
        if ( highScores ) {
            printf("THighScoresSave() => %d\n", THighScoresSave(highScores, "tetrominotris.hiscores"));
        }
    } else {
        printf("Loaded from file.\n");
        printf("%016llX  %u\n", highScores->magic, highScores->nScores);
    }
    if ( highScores ) {
        int     i = 0;
        
        if ( THighScoresDoesQualify(highScores, 32768, NULL) ) {
            char        playerInitials[3] = { 'D', 'r', 'J' };
            
            THighScoresRegister(highScores, 32768, playerInitials, 1001);
        } else {
            printf("Sorry, no high score.\n");
        }
        
        printf("%016llX  %u\n", highScores->magic, highScores->nScores);
        while ( i < highScores->nScores ) {
            printf("%d.    %c%c%c    %8u (%u)\n", i + 1,
                        highScores->scores[i].initials[0], highScores->scores[i].initials[1],
                        highScores->scores[i].initials[2], highScores->scores[i].score,
                        highScores->scores[i].playerUid
                    );
            i++;
        }
        
        printf("THighScoresSave() => %d\n", THighScoresSave(highScores, "tetrominotris.hiscores"));
        
        THighScoresDestroy(highScores);
    }
    return 0;
}

#endif /* THIGHSCORES_HELPER_EXE */
