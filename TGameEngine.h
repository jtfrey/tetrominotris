
#ifndef __TGAMEENGINE_H__
#define __TGAMEENGINE_H__

#include "tetrominotris_config.h"

#include "TCell.h"
#include "TBitGrid.h"
#include "TTetrominos.h"
#include "TScoreboard.h"
#include "TSprite.h"

#ifndef TGAMEENGINE_RANDOM_NSTATE
#    define TGAMEENGINE_RANDOM_NSTATE 64
#endif

/*
 * @function timespec_subtract
 *
 * Calculate the difference (t1 - t0) and store the result in dt.  The
 * same pointer can be passed for t1/t0 and dt.  The pointer dt is
 * returned.
 */
static inline struct timespec*
timespec_subtract(
    struct timespec         *dt,
    struct timespec const   *t1,
    struct timespec const   *t0
)
{
    time_t      dt_sec = t1->tv_sec - t0->tv_sec;
    long        dt_nsec = t1->tv_nsec - t0->tv_nsec;
    while ( dt_nsec < 0 ) dt_nsec += 1000000000, dt_sec--;
    while ( dt_nsec > 1000000000 ) dt_nsec -= 1000000000, dt_sec++;
    dt->tv_sec = dt_sec, dt->tv_nsec = dt_nsec;
    return dt;
}

/*
 * @function timespec_add
 *
 * Calculate the sum (t0 + dt) and store the result in t1.  The
 * same pointer can be passed for t0/dt and t1.  The pointer t1 is
 * returned.
 */
static inline struct timespec*
timespec_add(
    struct timespec         *t1,
    struct timespec const   *t0,
    struct timespec const   *dt
)
{
    time_t      dt_sec = t0->tv_sec + dt->tv_sec;
    long        dt_nsec = t0->tv_nsec + dt->tv_nsec;
    while ( dt_nsec < 0 ) dt_nsec += 1000000000, dt_sec--;
    while ( dt_nsec > 1000000000 ) dt_nsec -= 1000000000, dt_sec++;
    t1->tv_sec = dt_sec, t1->tv_nsec = dt_nsec;
    return t1;
}

/*
 * @function timespec_is_ordered_asc
 *
 * Returns true if t0 < t1.
 *
 */
static inline bool
timespec_is_ordered_asc(
    struct timespec const   *t0,
    struct timespec const   *t1
)
{
    struct timespec         dt;
    
    timespec_subtract(&dt, t0, t1);
    return (dt.tv_sec < 0 || (dt.tv_sec == 0 && dt.tv_nsec < 0));
}

/*
 * @function timespec_is_ordered_desc
 *
 * Returns true if t0 > t1.
 *
 */
static inline bool
timespec_is_ordered_desc(
    struct timespec const   *t0,
    struct timespec const   *t1
)
{
    struct timespec         dt;
    
    timespec_subtract(&dt, t0, t1);
    return (dt.tv_sec > 0 || (dt.tv_sec == 0 && dt.tv_nsec > 0));
}

/*
 * @function timespec_is_ordered_equal
 *
 * Returns true if t0 == t1.
 *
 */
static inline bool
timespec_is_ordered_equal(
    struct timespec const   *t0,
    struct timespec const   *t1
)
{
    return ((t0->tv_sec == t1->tv_sec) && (t0->tv_nsec == t1->tv_nsec));
}

/*
 * @function timespec_compare
 *
 * Compares t0 and t1.  Returns +1 if t0 < t1, -1 if t0 > t1, or
 * 0 if t0 == t1.
 *
 */
static inline int
timespec_compare(
    struct timespec const   *t0,
    struct timespec const   *t1
)
{
    struct timespec         dt;
    timespec_subtract(&dt, t0, t1);
    
    if (dt.tv_sec < 0 || (dt.tv_sec == 0 && dt.tv_nsec < 0)) return +1;
    if (dt.tv_sec > 0 || (dt.tv_sec == 0 && dt.tv_nsec > 0)) return -1;
    return 0;
}

/*
 * @function timespec_to_double
 *
 * Convert the time specification (in whole seconds and nanoseconds) to
 * a whole + fractional floating-point representation.  E.g.
 *
 *     t = { .tv_sec = 5, .tv_nsec = 435679 }
 *
 * becomes
 *
 *     5.000435679
 */
static inline double
timespec_to_double(
    struct timespec *t
)
{
    return (double)t->tv_sec + (double)t->tv_nsec * 1e-9;
}


static inline struct timespec
timespec_tpl_with_level(
    unsigned int    level
)
{
    struct timespec tpl;
    
    if ( level == 0 ) {
        tpl.tv_sec = 1; tpl.tv_nsec = 0;
    } else {
        double      s = 0.8 - 0.007 * (double)level;
        double      st = s;
        
        while ( --level ) st *= s;
        tpl.tv_sec = floor(st);
        tpl.tv_nsec = (long)(1e9 * (st - floor(st)));
    }
    return tpl;
}



/*
 * @typedef TGameEngine
 *
 * Data structure encapsulting all elements of the game
 * engine.
 */
typedef struct {
    // The game board:
    TBitGrid            *gameBoard;
    
    // Paused?
    bool                isPaused;
    
    // The scoreboard for the game:
    TScoreboard         scoreboard;
    
    // The base starting postion on the grid of each new tetromino:
    TGridPos            startingPos;
    
    // The current tetromino sprite and the next one due to appear
    // on the board:
    unsigned int        currentTetrominoId, nextTetrominoId;
    TSprite             currentSprite, nextSprite;
    
    // The map of position in the status list of each tetromino
    // and it's 4x4 representation to display:
    unsigned int        tetrominoIdsForReps[TTetrominosCount];
    uint16_t            terominoRepsForStats[TTetrominosCount];
    
    // State array for the PRNG:
    char                randomState[TGAMEENGINE_RANDOM_NSTATE];
    
    // The different timing variables:
    struct timespec     tElapsed;           // total time the game has been in-play
    struct timespec     tPerLine;           // time a tetromino hangs stationary
                                            // (changes by level)
} TGameEngine;


TGameEngine* TGameEngineCreate(unsigned int nChannels, unsigned int w, unsigned int h);

void TGameEngineChooseNextPiece(TGameEngine *gameEngine);

void TGameEngineCheckForCompleteRows(TGameEngine *gameEngine);

#endif /* __TGAMEENGINE_H__ */
