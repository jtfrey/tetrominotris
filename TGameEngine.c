
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
    unsigned int        nChannels,
    unsigned int        w,
    unsigned int        h
)
{
    TGameEngine         *newEngine = NULL;
    TBitGrid            *gameBoard = TBitGridCreate(TBitGridWordSizeForce16Bit, nChannels, w, h);
    
    if ( gameBoard ) {
        newEngine = (TGameEngine*)malloc(sizeof(TGameEngine));    
        if ( newEngine ) {
            // Initialize the PRNG:
            initstate(time(NULL), newEngine->randomState, TGAMEENGINE_RANDOM_NSTATE);
            
            // Ensure an empty game board to start:
            TBitGridFill(gameBoard, 0);
            
            //  Fill-in the rest of the game engine fields:
            newEngine->gameBoard = gameBoard;
            newEngine->isPaused = false;
            newEngine->startingPos = TGridPosMake(w / 2, 0);
            
            // Shift two pieces into the engine, current and next:
            TGameEngineChooseNextPiece(newEngine);
            TGameEngineChooseNextPiece(newEngine);
            
            // Now fill-in the scoreboard:
            newEngine->scoreboard = TScoreboardMake();
            
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
            
            // Timings:
            newEngine->tElapsed = TGameEngineZeroTime;
            newEngine->tPerLine = timespec_tpl_with_level(newEngine->scoreboard.level);
        } else {
            TBitGridDestroy(gameBoard);   
        }
    }
    return newEngine;
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
