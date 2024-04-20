
#ifndef __TSCOREBOARD_H__
#define __TSCOREBOARD_H__

#include "tetrominotris_config.h"
#include "TTetrominos.h"

/*
 * @enum Scoreboard line count indices
 *
 * Line-clearing happens as anywhere from 1 to 4 at a time
 * yielding specific point values.
 */
enum {
    TScoreboardLineCountTypeSingle = 0,
    TScoreboardLineCountTypeDouble,
    TScoreboardLineCountTypeTriple,
    TScoreboardLineCountTypeQuadruple,
    TScoreboardLineCountTypeListLength
};

/*
 * @typedef TScoreboardLineCountType
 *
 * The type of a line count type from the enumeration.
 */
typedef unsigned int TScoreboardLineCountType;

typedef struct {
    unsigned int        tetrominosOfType[TTetrominosCount];
    unsigned int        pointsAwarded[TScoreboardLineCountTypeListLength];
    unsigned int        nLinesOfType[TScoreboardLineCountTypeListLength];
    unsigned int        nLinesTotal;
    unsigned int        level;
    unsigned int        score;
} TScoreboard;

static inline TScoreboard
TScoreboardMake(void)
{
    TScoreboard     newScoreboard = {
                        .score = 0,
                        .level = 0,
                        .nLinesTotal = 0,
                        .pointsAwarded = { 30, 100, 300, 1200 },
                        .nLinesOfType = { 0, 0, 0, 0 },
                        .tetrominosOfType = { 0, 0, 0, 0, 0, 0, 0 }
                    };
    return newScoreboard;
}

static inline void
TScoreboardAddLinesOfType(
    TScoreboard     *scoreboard,
    unsigned int    lineCount
)
{
    scoreboard->nLinesTotal += lineCount;
    scoreboard->nLinesOfType[lineCount - 1]++;
    scoreboard->score += scoreboard->pointsAwarded[lineCount - 1] * (scoreboard->level + 1);
    
    if ( scoreboard->nLinesTotal % 10 == 0 ) scoreboard->level++;
}
    
#endif /* __TSCOREBOARD_H__ */
