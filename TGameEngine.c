/*	TGameEngine.c
	Copyright (c) 2024, J T Frey
*/

#include "TGameEngine.h"

struct timespec TGameEngineZeroTime = { .tv_sec = 0, .tv_nsec = 0 };

//

enum {
    TGameEngineBitGridChannelIsOccupied = 1 << 0,
    TGameEngineBitGridChannelIsFlashing = 1 << 1,
    TGameEngineBitGridChannelColorIndex = (1 << 2) | (1 << 3)
};

//

TGameEngine*
TGameEngineCreate(
    bool            useColor,
    unsigned int    w,
    unsigned int    h,
    unsigned int    startingLevel
)
{
    TGameEngine     *newEngine = NULL;
    TBitGrid        *gameBoard = TBitGridCreate(TBitGridWordSizeForce16Bit, useColor ? 3 : 1, w, h);
    
    if ( gameBoard ) {
        newEngine = (TGameEngine*)malloc(sizeof(TGameEngine));    
        if ( newEngine ) {
            //  Associate the bit grid with the new game engine:
            newEngine->gameBoard = gameBoard;
            
            // Using color?
            newEngine->doesUseColor = useColor;
            
            // Fill-in the starting level:
            newEngine->startingLevel = (startingLevel <= 9) ? startingLevel : 9;
            
            // Fill-in the legend tetromino representations for the stats display:
            newEngine->terominoRepsForStats[0] = TTetrominoOrientationShiftLeft(TTetrominoOrientationShiftUp(TTetrominosExtractOrientation(2, 3)));
            newEngine->terominoRepsForStats[1] = TTetrominoOrientationShiftLeft(TTetrominoOrientationShiftUp(TTetrominosExtractOrientation(6, 3)));
            newEngine->terominoRepsForStats[2] = TTetrominoOrientationShiftLeft(TTetrominoOrientationShiftUp(TTetrominosExtractOrientation(4, 1)));
            newEngine->terominoRepsForStats[3] = TTetrominoOrientationShiftUp(TTetrominosExtractOrientation(0, 0));
            newEngine->terominoRepsForStats[4] = TTetrominoOrientationShiftLeft(TTetrominoOrientationShiftUp(TTetrominosExtractOrientation(3, 1)));
            newEngine->terominoRepsForStats[5] = TTetrominoOrientationShiftLeft(TTetrominoOrientationShiftUp(TTetrominosExtractOrientation(5, 1)));
            newEngine->terominoRepsForStats[6] = TTetrominoOrientationShiftUp(TTetrominosExtractOrientation(1, 3));
            newEngine->tetrominoIdsForReps[0] = 3;
            newEngine->tetrominoIdsForReps[1] = 6;
            newEngine->tetrominoIdsForReps[2] = 0;
            newEngine->tetrominoIdsForReps[3] = 4;
            newEngine->tetrominoIdsForReps[4] = 2;
            newEngine->tetrominoIdsForReps[5] = 5;
            newEngine->tetrominoIdsForReps[6] = 1;
            
            // Reset game state:
            TGameEngineReset(newEngine);
        } else {
            TBitGridDestroy(gameBoard);   
        }
    }
    return newEngine;
}

//

void
TGameEngineReset(
    TGameEngine *gameEngine
)
{
    // Initialize the PRNG:
    initstate(time(NULL), gameEngine->randomState, TGAMEENGINE_RANDOM_NSTATE);
    
    // Ensure an empty game board to start:
    TBitGridFill(gameEngine->gameBoard, 0);
            
    //  Fill-in the rest of the game engine fields:
    gameEngine->hasBeenStarted = gameEngine->isPaused = gameEngine->hasEnded = false;
    gameEngine->startingPos = TGridPosMake(gameEngine->gameBoard->dimensions.w / 2, 0);
            
    // Shift two pieces into the engine, current and next:
    TGameEngineChooseNextPiece(gameEngine);
    TGameEngineChooseNextPiece(gameEngine);
    
    // Now fill-in the scoreboard:
    gameEngine->scoreboard = TScoreboardMake();
    gameEngine->scoreboard.level = gameEngine->startingLevel;
    gameEngine->extraPoints = 0;
    gameEngine->isInSoftDrop = 0;
            
    // Timings:
    gameEngine->tickCount = 0UL;
    gameEngine->tLastTick = TGameEngineZeroTime;
    gameEngine->tElapsed = TGameEngineZeroTime;
    gameEngine->tPerLine = timespec_tpl_with_level(gameEngine->scoreboard.level);
    gameEngine->tNextDrop = TGameEngineZeroTime;
}

//

void
TGameEngineChooseNextPiece(
    TGameEngine *gameEngine
)
{
    // Add the current piece to the tally:
    gameEngine->scoreboard.tetrominosOfType[gameEngine->tetrominoIdsForReps[gameEngine->currentTetrominoId]]++;
    
    // What was the next piece becomes the current...
    gameEngine->currentTetrominoId = gameEngine->nextTetrominoId;
    gameEngine->currentSprite = gameEngine->nextSprite;
    
    // ...and we select another new piece:
    gameEngine->nextTetrominoId = random() % TTetrominosCount;
    gameEngine->nextSprite = TSpriteMake(TTetrominos[gameEngine->nextTetrominoId], gameEngine->startingPos, 0);
    
    // To be fair, back the piece up as many rows as necessary to align the
    // next piece with the top of the game grid:
    gameEngine->nextSprite.P.j -= TSpriteGetInitialClearRows(&gameEngine->nextSprite);
}

//

void
TGameEngineCheckForCompleteRows(
    TGameEngine     *gameEngine
)
{
    TBitGridIterator    *iterator = TBitGridIteratorCreate(gameEngine->gameBoard, TGameEngineBitGridChannelIsOccupied);
    unsigned int        startFullRow = -1, nRow = 0, currentRow;
    bool                didClearRows = false;
    unsigned int        curLevel = gameEngine->scoreboard.level;
    
    while ( TBitGridIteratorNextFullRow(iterator, &currentRow) ) {
        if ( nRow == 0 ) {
            startFullRow = currentRow;
            nRow = 1;
        } else {
            if ( startFullRow + nRow == currentRow ) {
                // Extending a multirow match:
                nRow++;
            } else {
                TBitGridClearLines(gameEngine->gameBoard, startFullRow, startFullRow + nRow - 1);
                didClearRows = true;
                TScoreboardAddLinesOfType(&gameEngine->scoreboard, nRow);
                startFullRow = currentRow, nRow = 1;
            }
        }
    }
    if ( nRow > 0 ) {
        TBitGridClearLines(gameEngine->gameBoard, startFullRow, startFullRow + nRow - 1);
        didClearRows = true;
        TScoreboardAddLinesOfType(&gameEngine->scoreboard, nRow);
    }
    if ( didClearRows ) {
        // Level change?
        if ( gameEngine->scoreboard.level > curLevel ) gameEngine->tPerLine = timespec_tpl_with_level(gameEngine->scoreboard.level);
    }
}

//

TGameEngineUpdateNotification
TGameEngineTick(
    TGameEngine         *gameEngine,
    TGameEngineEvent    theEvent
)
{
    TGameEngineUpdateNotification       updates = 0;
    struct timespec                     t1, dt;
    
    if ( ! gameEngine->hasEnded ) {
        // Get current absolute cycle time:
        clock_gettime(CLOCK_REALTIME, &t1);
    
        if ( ! gameEngine->hasBeenStarted ) {
            if ( theEvent == TGameEngineEventStartGame ) {
                // Time to start the game:
                gameEngine->hasBeenStarted = true;
                timespec_add(&gameEngine->tNextDrop, &t1, &gameEngine->tPerLine);
                updates = TGameEngineUpdateNotificationAll;
            }
        } else if ( gameEngine->isPaused ) {
            if ( theEvent == TGameEngineEventTogglePause ) gameEngine->isPaused = false;
        } else {
            bool            shouldStopFalling = false;
        
            if ( timespec_is_ordered_asc(&gameEngine->tNextDrop, &t1) ) {
                // Time for an automatic drop:
                TGridPos    newP = gameEngine->currentSprite.P;
                uint16_t    board4x4, piece4x4 = TSpriteGet4x4(&gameEngine->currentSprite);
        
                newP.j++;
                board4x4 = TBitGridExtract4x4AtPosition(gameEngine->gameBoard, 0, newP);
                if ( (board4x4 & piece4x4) == 0 ) {
                    // The piece can fall another row by itself; revoke extra points and
                    // cancel soft drop:
                    gameEngine->currentSprite.P.j++;
                    gameEngine->extraPoints = 0;
                    gameEngine->isInSoftDrop = false;
                    updates |= TGameEngineUpdateNotificationGameBoard;
                } else {
                    // The piece cannot fall any further, so it must stop:
                    shouldStopFalling = true;
                }
                timespec_add(&gameEngine->tNextDrop, &t1, &gameEngine->tPerLine);
            }
        
            switch ( theEvent ) {
        
                case TGameEngineEventNoOp:
                    break;
            
                case TGameEngineEventTogglePause:
                    gameEngine->isPaused = true;
                    updates |= TGameEngineUpdateNotificationGameBoard;
                    break;
            
                case TGameEngineEventReset:
                    TGameEngineReset(gameEngine);
                    TGameEngineTick(gameEngine, TGameEngineEventStartGame);
                    return TGameEngineUpdateNotificationAll;
            
                case TGameEngineEventRotateClockwise: {
                    TSprite         newOrientation = TSpriteMakeRotated(&gameEngine->currentSprite);
                    uint16_t        board4x4 = TBitGridExtract4x4AtPosition(gameEngine->gameBoard, 0, newOrientation.P);
                    uint16_t        piece4x4 = TSpriteGet4x4(&newOrientation);
            
                    if ( (board4x4 & piece4x4) == 0 ) {
                        gameEngine->currentSprite = newOrientation;
                        updates |= TGameEngineUpdateNotificationGameBoard;
                    }
                    break;
                }
            
                case TGameEngineEventRotateAntiClockwise: {
                    TSprite         newOrientation = TSpriteMakeRotatedAnti(&gameEngine->currentSprite);
                    uint16_t        board4x4 = TBitGridExtract4x4AtPosition(gameEngine->gameBoard, 0, newOrientation.P);
                    uint16_t        piece4x4 = TSpriteGet4x4(&newOrientation);
            
                    if ( (board4x4 & piece4x4) == 0 ) {
                        gameEngine->currentSprite = newOrientation;
                        updates |= TGameEngineUpdateNotificationGameBoard;
                    }
                    break;
                }
            
                case TGameEngineEventMoveLeft: {
                    TGridPos    newP = gameEngine->currentSprite.P;
                    uint16_t    board4x4, piece4x4 = TSpriteGet4x4(&gameEngine->currentSprite);
            
                    newP.i--;
                    board4x4 = TBitGridExtract4x4AtPosition(gameEngine->gameBoard, 0, newP);
                    if ( (board4x4 & piece4x4) == 0 ) {
                        gameEngine->currentSprite.P.i--;
                        updates |= TGameEngineUpdateNotificationGameBoard;
                    }
                    break;
                }
            
                case TGameEngineEventMoveRight: {
                    TGridPos    newP = gameEngine->currentSprite.P;
                    uint16_t    board4x4, piece4x4 = TSpriteGet4x4(&gameEngine->currentSprite);
            
                    newP.i++;
                    board4x4 = TBitGridExtract4x4AtPosition(gameEngine->gameBoard, 0, newP);
                    if ( (board4x4 & piece4x4) == 0 ) {
                        gameEngine->currentSprite.P.i++;
                        updates |= TGameEngineUpdateNotificationGameBoard;
                    }
                    break;
                }
            
                case TGameEngineEventSoftDrop: {
                    TGridPos    newP = gameEngine->currentSprite.P;
                    uint16_t    board4x4, piece4x4 = TSpriteGet4x4(&gameEngine->currentSprite);
            
                    newP.j++;
                    board4x4 = TBitGridExtract4x4AtPosition(gameEngine->gameBoard, 0, newP);
                    if ( (board4x4 & piece4x4) == 0 ) {
                        gameEngine->currentSprite.P.j++;
                        gameEngine->isInSoftDrop = true;
                        gameEngine->extraPoints++;
                        updates |= TGameEngineUpdateNotificationGameBoard;
                    } else {
                        shouldStopFalling = true;
                    }
                    timespec_add(&gameEngine->tNextDrop, &t1, &gameEngine->tPerLine);
                    break;
                }
            
                case TGameEngineEventHardDrop: {
                    while ( 1 ) {
                        // Test for ok:
                        TGridPos    newP = gameEngine->currentSprite.P;
                        uint16_t    board4x4, piece4x4 = TSpriteGet4x4(&gameEngine->currentSprite);
            
                        newP.j++;
                        board4x4 = TBitGridExtract4x4AtPosition(gameEngine->gameBoard, 0, newP);
                        if ( (board4x4 & piece4x4) == 0 ) {
                            gameEngine->currentSprite.P.j++;
                            gameEngine->extraPoints += 2;
                        } else {
                            break;
                        }
                    }
                    shouldStopFalling = true;
                    break;
                }
            
            }
        
            if ( shouldStopFalling ) {
                uint16_t            board4x4, piece4x4 = TSpriteGet4x4(&gameEngine->currentSprite);
                
                TBitGridSet4x4AtPosition(gameEngine->gameBoard, 0, gameEngine->currentSprite.P, piece4x4);
                if ( gameEngine->doesUseColor ) {
                    TBitGridSet4x4AtPosition(gameEngine->gameBoard, 1, gameEngine->currentSprite.P, (gameEngine->currentSprite.colorIdx & 0x1) ? piece4x4 : 0x0000);
                    TBitGridSet4x4AtPosition(gameEngine->gameBoard, 2, gameEngine->currentSprite.P, (gameEngine->currentSprite.colorIdx & 0x2) ? piece4x4 : 0x0000);
                }
                TGameEngineCheckForCompleteRows(gameEngine);
                gameEngine->scoreboard.score += gameEngine->extraPoints;
                gameEngine->extraPoints = 0;
                gameEngine->isInSoftDrop = false;
                TGameEngineChooseNextPiece(gameEngine);
                
                // Test for game over:
                piece4x4 = TSpriteGet4x4(&gameEngine->currentSprite);
                board4x4 = TBitGridExtract4x4AtPosition(gameEngine->gameBoard, 0, gameEngine->currentSprite.P);
                if ( (board4x4 & piece4x4) != 0 ) {
                    gameEngine->hasEnded = true;
                    memset(&gameEngine->currentSprite, 0, sizeof(gameEngine->currentSprite));
                    memset(&gameEngine->nextSprite, 0, sizeof(gameEngine->nextSprite));
                    TBitGridFill(gameEngine->gameBoard, 1);
                }
                updates |= TGameEngineUpdateNotificationGameBoard | TGameEngineUpdateNotificationScoreboard | TGameEngineUpdateNotificationNextTetromino;
            }
        
            timespec_add(&gameEngine->tElapsed, &gameEngine->tElapsed, timespec_subtract(&dt, &t1, &gameEngine->tLastTick));
        }
    
        gameEngine->tLastTick = t1;
        gameEngine->tickCount++;
    } else if ( theEvent == TGameEngineEventReset ) {
        TGameEngineReset(gameEngine);
        TGameEngineTick(gameEngine, TGameEngineEventStartGame);
        return TGameEngineUpdateNotificationAll;
    }
    return updates;
}

