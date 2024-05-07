/*	TGameEngine.c
	Copyright (c) 2024, J T Frey
*/

#include "TGameEngine.h"

struct timespec TGameEngineZeroTime = { .tv_sec = 0, .tv_nsec = 0 };

struct timespec TGameEngine500ms = { .tv_sec = 0, .tv_nsec = 500000000 };

//

enum {
    TGameEngineBitGridChannelIsOccupied = 0,
    TGameEngineBitGridChannelIsCompleted = 1,
    TGameEngineBitGridChannelColorIndexBit0 = 2,
    TGameEngineBitGridChannelColorIndexBit1 = 3,
};

//

TGameEngine*
TGameEngineCreate(
    TBitGridWordSize    wordSize, 
    bool                useColor,
    unsigned int        w,
    unsigned int        h,
    unsigned int        startingLevel
)
{
    TGameEngine     *newEngine = NULL;
    TBitGrid        *gameBoard = TBitGridCreate(wordSize, useColor ? 4 : 2, w, h);
    
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
    TBitGridFillCells(gameEngine->gameBoard, 0);
            
    //  Fill-in the rest of the game engine fields:
    gameEngine->gameState = TGameEngineStateStartup;
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
    gameEngine->nextSprite.P.i = (gameEngine->gameBoard->dimensions.w - 4) / 2;
    gameEngine->nextSprite.P.j -= TSpriteGetInitialClearRows(&gameEngine->nextSprite);
}

//

bool
TGameEngineCheckForCompleteRows(
    TGameEngine     *gameEngine,
    bool            shouldTestOnly
)
{
    return TGameEngineCheckForCompleteRowsInRange(gameEngine, shouldTestOnly, 0, gameEngine->gameBoard->dimensions.h - 1);
}

bool
TGameEngineCheckForCompleteRowsInRange(
    TGameEngine     *gameEngine,
    bool            shouldTestOnly,
    unsigned int    startRow,
    unsigned int    endRow
)
{
    TBitGridIterator    *iterator = TBitGridIteratorCreateWithRowRange(gameEngine->gameBoard, 1 << TGameEngineBitGridChannelIsOccupied, startRow, endRow);
    unsigned int        startFullRow = -1, nRow = 0, currentRow;
    unsigned int        curLevel = gameEngine->scoreboard.level;
    bool                didClearRows = false;

    while ( TBitGridIteratorNextFullRow(iterator, &currentRow) ) {
        if ( nRow == 0 ) {
            startFullRow = currentRow;
            nRow = 1;
        } else {
            if ( startFullRow + nRow == currentRow ) {
                // Extending a multirow match:
                nRow++;
            } else {
                if ( shouldTestOnly ) {
                    TBitGridSetChannelInRowRange(gameEngine->gameBoard, TGameEngineBitGridChannelIsCompleted, startFullRow, startFullRow + nRow - 1, true);
                } else {
                    TBitGridClearLines(gameEngine->gameBoard, startFullRow, startFullRow + nRow - 1);
                    TScoreboardAddLinesOfType(&gameEngine->scoreboard, nRow);
                    startFullRow = currentRow, nRow = 1;
                }
                didClearRows = true;
            }
        }
    }
    if ( nRow > 0 ) {
        if ( shouldTestOnly ) {
            TBitGridSetChannelInRowRange(gameEngine->gameBoard, TGameEngineBitGridChannelIsCompleted, startFullRow, startFullRow + nRow - 1, true);
        } else {
            TBitGridClearLines(gameEngine->gameBoard, startFullRow, startFullRow + nRow - 1);
            TScoreboardAddLinesOfType(&gameEngine->scoreboard, nRow);
            startFullRow = currentRow, nRow = 1;
        }
        didClearRows = true;
    }
    if ( didClearRows && ! shouldTestOnly ) {
        // Level change?
        if ( gameEngine->scoreboard.level > curLevel ) gameEngine->tPerLine = timespec_tpl_with_level(gameEngine->scoreboard.level);
    }
    return didClearRows;
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
    
    // Get current absolute cycle time:
    clock_gettime(CLOCK_REALTIME, &t1);
    
    switch ( gameEngine->gameState ) {
        
        case TGameEngineStateStartup:
            if ( theEvent == TGameEngineEventStartGame ) {
                // Time to start the game:
                gameEngine->gameState++;
                timespec_add(&gameEngine->tNextDrop, &t1, &gameEngine->tPerLine);
                updates = TGameEngineUpdateNotificationAll;
            }
            break;
        
        case TGameEngineStateGameHasStarted: {
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
                    gameEngine->gameState = TGameEngineStateGameIsPaused;
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
                    TBitGridSet4x4AtPosition(gameEngine->gameBoard, 2, gameEngine->currentSprite.P, (gameEngine->currentSprite.colorIdx & 0x1) ? piece4x4 : 0x0000);
                    TBitGridSet4x4AtPosition(gameEngine->gameBoard, 3, gameEngine->currentSprite.P, (gameEngine->currentSprite.colorIdx & 0x2) ? piece4x4 : 0x0000);
                }
                if ( TGameEngineCheckForCompleteRowsInRange(gameEngine, true, gameEngine->currentSprite.P.j, gameEngine->currentSprite.P.j + 3) ) {
                    gameEngine->gameState = TGameEngineStateHoldClearedLines;
                    gameEngine->completionFlashIdx = 0;
                    timespec_add(&gameEngine->tNextDrop, &t1, &TGameEngine500ms);
                    updates |= TGameEngineUpdateNotificationGameBoard;
                } else {
                    gameEngine->scoreboard.score += gameEngine->extraPoints;
                    gameEngine->extraPoints = 0;
                    gameEngine->isInSoftDrop = false;
                    TGameEngineChooseNextPiece(gameEngine);
                    timespec_add(&gameEngine->tNextDrop, &t1, &gameEngine->tPerLine);
            
                    // Test for game over:
                    piece4x4 = TSpriteGet4x4(&gameEngine->currentSprite);
                    board4x4 = TBitGridExtract4x4AtPosition(gameEngine->gameBoard, 0, gameEngine->currentSprite.P);
                    if ( (board4x4 & piece4x4) == 0 ) {
                        gameEngine->gameState = TGameEngineStateGameHasStarted;
                    } else {
                        gameEngine->gameState = TGameEngineStateCheckHighScore;
                        memset(&gameEngine->currentSprite, 0, sizeof(gameEngine->currentSprite));
                        memset(&gameEngine->nextSprite, 0, sizeof(gameEngine->nextSprite));
                        TBitGridFillCells(gameEngine->gameBoard, 1);
                    }
                    updates |= TGameEngineUpdateNotificationGameBoard | TGameEngineUpdateNotificationScoreboard | TGameEngineUpdateNotificationNextTetromino;
                }
            }
    
            timespec_add(&gameEngine->tElapsed, &gameEngine->tElapsed, timespec_subtract(&dt, &t1, &gameEngine->tLastTick));                
            break;
        }
        
        case TGameEngineStateGameIsPaused:
            if ( theEvent == TGameEngineEventTogglePause ) gameEngine->gameState = TGameEngineStateGameHasStarted;
            break;
        
        case TGameEngineStateHoldClearedLines:
            if ( timespec_is_ordered_asc(&gameEngine->tNextDrop, &t1) ) {
                uint16_t            board4x4, piece4x4;
                
                TGameEngineCheckForCompleteRowsInRange(gameEngine, false, gameEngine->currentSprite.P.j, gameEngine->currentSprite.P.j + 3);

                gameEngine->scoreboard.score += gameEngine->extraPoints;
                gameEngine->extraPoints = 0;
                gameEngine->isInSoftDrop = false;
                TGameEngineChooseNextPiece(gameEngine);
                timespec_add(&gameEngine->tNextDrop, &t1, &gameEngine->tPerLine);
            
                // Test for game over:
                piece4x4 = TSpriteGet4x4(&gameEngine->currentSprite);
                board4x4 = TBitGridExtract4x4AtPosition(gameEngine->gameBoard, 0, gameEngine->currentSprite.P);
                if ( (board4x4 & piece4x4) == 0 ) {
                    gameEngine->gameState = TGameEngineStateGameHasStarted;
                } else {
                    gameEngine->gameState = TGameEngineStateCheckHighScore;
                    memset(&gameEngine->currentSprite, 0, sizeof(gameEngine->currentSprite));
                    memset(&gameEngine->nextSprite, 0, sizeof(gameEngine->nextSprite));
                    TBitGridFillCells(gameEngine->gameBoard, 1);
                }
                updates |= TGameEngineUpdateNotificationScoreboard | TGameEngineUpdateNotificationNextTetromino;
            } else {
                gameEngine->completionFlashIdx = ! gameEngine->completionFlashIdx;
            }
            updates |= TGameEngineUpdateNotificationGameBoard;
            break;
            
        case TGameEngineStateGameHasEnded:
            if ( theEvent == TGameEngineEventReset ) {
                TGameEngineReset(gameEngine);
                TGameEngineTick(gameEngine, TGameEngineEventStartGame);
                return TGameEngineUpdateNotificationAll;
            }
            break;
            
        case TGameEngineStateCheckHighScore:
            break;
            
    }

    gameEngine->tLastTick = t1;
    gameEngine->tickCount++;
    return updates;
}

