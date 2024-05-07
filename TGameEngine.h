/*	TGameEngine.h
	Copyright (c) 2024, J T Frey
*/

/*!
	@header Game engine
	A game engine encapsulates all components that cooperate in managing
	the game state:
	
	    - simple state flags (game is started or ended, game is paused,
	      game board uses color)
	    - the game board (bit grid)
	    - the scoreboard
	    - the in-play tetromino (as a sprite)
	    - the next tetromino that will be in-play (as a sprite)
	    - the state array for the PRNG
	    - game timing values (elapsed time, time of last tick, time
	      tetrominos hang per line, future time when in-play tetromino
	      should automatically drop)
	
	The most important function in this unit is TGameEngineTick().  It
	accepts a single game event (e.g. move tetromino left) and handles
	all state change required to implement that event (or not).  All
	timers are updated.  The TGameEngineTick() function should be called
	in a relatively fast loop that watches for user input.
	
	Game events are the very same events that get associated with keys in
	the TKeymap unit.
*/

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

/*
 * @function timespec_tpl_with_level
 *
 * The amount of time a tetromino is suspended at each line
 * is modeled by the function:
 *
 *     (0.8-(Level*0.007))^Level
 *
 * This function evaluates that function for the given
 * level.
 */
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
 * @enum TGameEngine external events
 *
 * Enumerates the events an external system can communicate
 * to the game engine on each tick.
 *
 * The external system is free to implement the user interface
 * in whatever manner it decides since the user-directed
 * changes to the game engine are abstracted from key presses,
 * etc.
 */
enum {
    TGameEngineEventNoOp = 0,
    TGameEngineEventStartGame,
    TGameEngineEventMoveLeft,
    TGameEngineEventMoveRight,
    TGameEngineEventRotateClockwise,
    TGameEngineEventRotateAntiClockwise,
    TGameEngineEventSoftDrop,
    TGameEngineEventHardDrop,
    TGameEngineEventTogglePause,
    TGameEngineEventReset
};

/*
 * @typedef TGameEngineEvent
 *
 * The type of a game engine event.
 */
typedef uint8_t TGameEngineEvent;

/*
 * @enum TGameEngine external update notifications
 *
 * The result of the game engine's processing an external event
 * is notification of what aspects of the game engine changed
 * state as a result -- and probably need to have their visual
 * representation(s) updated.
 */
enum {
    TGameEngineUpdateNotificationGameBoard = 1 << 0,
    TGameEngineUpdateNotificationScoreboard = 1 << 1,
    TGameEngineUpdateNotificationNextTetromino = 1 << 2,
    //
    TGameEngineUpdateNotificationAll = 0x7
};

/*
 * @typedef TGameEngineUpdateNotification
 *
 * The type of a game engine update notification.
 */
typedef unsigned int TGameEngineUpdateNotification;

/*
 * @enum TGameEngine state
 *
 * The game engine progresses through a series of states that
 * influence what events it will process, what should be
 * displayed on the game board, etc.
 *
 * The engine is initialized into the startup state.  In this
 * state, a TGameEngineEventStartGame event will begin play.
 *
 * Once play has begun the engine enters the
 * TGameEngineStateGameHasStarted state.
 *
 * A TGameEngineEventTogglePause event will transition to the
 * TGameEngineStateGameIsPaused state; a subsequent 
 * TGameEngineEventTogglePause event will transition back to
 * the TGameEngineStateGameHasStarted state.
 *
 * After a tetromino is placed on the game board, if completed
 * lines are found they are marked as completed in the bit grid
 * and the engine enters the TGameEngineStateHoldClearedLines
 * state for 500 ms.  This allows for visual indication of the
 * completed lines before they are cleared (when the 500 ms expires)
 * and the engine returns to TGameEngineStateGameHasStarted.
 *
 * Once the game board has been filled and no new tetromino can
 * be introduced, the engine enters the TGameEngineStateCheckHighScore
 * state.  The external logic (e.g. in tetrominotris.c) is at this
 * point responsible for handling that action and transitioning the
 * engine to the TGameEngineStateGameHasEnded state.
 *
 * In the TGameEngineStateGameHasEnded state, the only possible
 * state change is to return to TGameEngineStateStartup on a
 * TGameEngineEventReset event.
 */
enum {
    TGameEngineStateStartup = 0,
    TGameEngineStateGameHasStarted,
    TGameEngineStateGameIsPaused,
    TGameEngineStateHoldClearedLines,
    TGameEngineStateCheckHighScore,
    TGameEngineStateGameHasEnded,
    TGameEngineStateMax
};

/*
 * @typedef TGameEngineState
 *
 * The type of a value from the TGameEngine state enumeration.
 */
typedef unsigned int TGameEngineState;

/*
 * @typedef TGameEngine
 *
 * Data structure encapsulting all elements of the game
 * engine.
 */
typedef struct {
    // The game board:
    TBitGrid            *gameBoard;
    
    // Game engine state:
    TGameEngineState    gameState;
    bool                isInSoftDrop;
    bool                doesUseColor;
    unsigned int        completionFlashIdx;
    
    // Starting level for the game(s):
    unsigned int        startingLevel;
    
    // The scoreboard for the game:
    TScoreboard         scoreboard;
    unsigned int        extraPoints;
    
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
    unsigned long       tickCount;          // number of times the tick function has
                                            // been called with game time running
    struct timespec     tLastTick;          // last time the tick function was
                                            // called (for calculated elapsed time)
    struct timespec     tElapsed;           // total time the game has been in-play
    struct timespec     tPerLine;           // time a tetromino hangs stationary
                                            // (changes by level)
    struct timespec     tNextDrop;          // time at which next automatic line
                                            // drop (or completed line clear) occurs
} TGameEngine;

/*
 * @function TGameEngineCreate
 *
 * Create a new game engine with the given game board dimensions ([w]idth and
 * [h]eight) and either 1 (useColor = false) or 3 (useColor = true) bit
 * channels.
 *
 * All elements of the engine are initialized to their starting values.  The
 * startingLevel must be between 0 and 9 (per the original game).
 */
TGameEngine* TGameEngineCreate(TBitGridWordSize wordSize, bool useColor, unsigned int w, unsigned int h, unsigned int startingLevel);

/*
 * @function TGameEngineReset
 *
 * Reset all elements of the gameEngine to their initial starting values.
 * The game board is cleared, new tetromino pieces are selected, timers are
 * reset, etc.
 */
void TGameEngineReset(TGameEngine *gameEngine);

/*
 * @function TGameEngineChooseNextPiece
 *
 * Add the current tetromino piece to the tally; shift the next piece into being
 * the current; and select a new next piece.
 */
void TGameEngineChooseNextPiece(TGameEngine *gameEngine);

/*
 * @function TGameEngineCheckForCompleteRows
 *
 * Check for any ranges of completed rows and return true if any were present.
 */
bool TGameEngineCheckForCompleteRows(TGameEngine *gameEngine, bool shouldTestOnly);

/*
 * @function TGameEngineCheckForCompleteRowsInRange
 *
 * Check for any ranges of completed rows between the startRow and endRow,
 * inclusive; remove them from the game board and update the line type stats;
 * and update the score.
 *
 * The original code checked the entire game board for completed rows, but
 * a single tetromino can only ever complete the 1 to 4 rows in which is has
 * settled.  So it's more optimal to just check the 4 rows the tetromino
 * sprite occupies.
 */
bool TGameEngineCheckForCompleteRowsInRange(TGameEngine *gameEngine, bool shouldTestOnly, unsigned int startRow, unsigned int endRow);

/*
 * @function TGameEngineTick
 *
 * Make all updates associated with theEvent to the gameEngine.  The components
 * whose state have changed is returned as a TGameEngineUpdateNotification bit
 * vector.
 */
TGameEngineUpdateNotification TGameEngineTick(TGameEngine *gameEngine, TGameEngineEvent theEvent);

#endif /* __TGAMEENGINE_H__ */
