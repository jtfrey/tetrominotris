
#ifndef __TSCOREBOARD_H__
#define __TSCOREBOARD_H__

#include "tetrominotris_config.h"
#include "TTetrominos.h"

/*
 * @enum Scoreboard line count indices
 *
 * Line-clearing happens for anywhere from 1 to 4 at a time
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

/*
 * @typedef TScoreboard
 *
 * The scoreboard data structure keeps track of:
 *
 * - the count of each tetromino type placed on the board
 * - the number of times each line-clearing type has occurred
 * - the total number of lines cleared
 * - the number of lines that must be cleared before the level
 *       increases (nLinesPerLevel)
 * - the current level and the line count at which the level next
 *       increases (nextLevelUp)
 * - the score
 *
 * The point value for each line-clearing type is present, as
 * well; the consumer could override the defaults that are
 * initialized using TScoreboardMake().
 */
typedef struct {
    unsigned int        tetrominosOfType[TTetrominosCount];
    unsigned int        pointsAwarded[TScoreboardLineCountTypeListLength];
    unsigned int        nLinesOfType[TScoreboardLineCountTypeListLength];
    unsigned int        nLinesTotal;
    unsigned int        level, nextLevelUp, nLinesPerLevel;
    unsigned int        score;
} TScoreboard;

/*
 * @function TScoreboardMake
 *
 * Initialize and return an empty scoreboard.
 */
static inline TScoreboard
TScoreboardMake(void)
{
    TScoreboard     newScoreboard = {
                        .score = 0,
                        .level = 0, .nextLevelUp = 10, .nLinesPerLevel = 10,
                        .nLinesTotal = 0,
                        .pointsAwarded = { 30, 100, 300, 1200 },
                        .nLinesOfType = { 0, 0, 0, 0 },
                        .tetrominosOfType = { 0, 0, 0, 0, 0, 0, 0 }
                    };
    return newScoreboard;
}

/*
 * @function TScoreboardAddLinesOfType
 *
 * Update the given scoreboard with the completion of a contiguous
 * block of lineCount full rows.  The line counts, score, and level
 * are updated accordingly.
 */
static inline void
TScoreboardAddLinesOfType(
    TScoreboard     *scoreboard,
    unsigned int    lineCount
)
{
    scoreboard->nLinesTotal += lineCount;
    scoreboard->nLinesOfType[lineCount - 1]++;
    scoreboard->score += scoreboard->pointsAwarded[lineCount - 1] * (scoreboard->level + 1);
    
    if ( scoreboard->nLinesTotal >= scoreboard->nextLevelUp ) {
        scoreboard->level++;
        scoreboard->nextLevelUp += scoreboard->nLinesPerLevel;
    }
}
    
#endif /* __TSCOREBOARD_H__ */
