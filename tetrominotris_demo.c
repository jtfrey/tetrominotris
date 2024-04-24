
#include "TTetrominos.h"
#include "TBitGrid.h"
#include "TCell.h"

int
main()
{
    TBitGrid    *ourBoard = TBitGridCreate(TBitGridWordSizeDefault, 1, 10, 20);
    TGridPos    P;
    TGridIndex  I;
    
    P = TGridPosMake(0,0); I = TBitGridPosToIndex(ourBoard, P);
    printf("P = { %u, %u } => I = { %u, %u }\n", P.i, P.j, I.W, I.b);
    
    P = TBitGridIndexToPos(ourBoard, I);
    printf("I = { %u, %u } => P = { %u, %u }\n", I.W, I.b, P.i, P.j);
    
    P = TGridPosMake(5,10); I = TBitGridPosToIndex(ourBoard, P);
    printf("P = { %u, %u } => I = { %u, %u }\n", P.i, P.j, I.W, I.b);
    P = TGridPosMake(6,10); I = TBitGridPosToIndex(ourBoard, P);
    printf("P = { %u, %u } => I = { %u, %u }\n", P.i, P.j, I.W, I.b);
    P = TGridPosMake(6,11); I = TBitGridPosToIndex(ourBoard, P);
    printf("P = { %u, %u } => I = { %u, %u }\n", P.i, P.j, I.W, I.b);
    
    TBitGridFill(ourBoard, false);
    
    TBitGridSetValueAtIndex(ourBoard, TBitGridMakeGridIndexWithPos(ourBoard, 2, 10), true);
    TBitGridSetValueAtIndex(ourBoard, TBitGridMakeGridIndexWithPos(ourBoard, 3, 10), true);
    TBitGridSetValueAtIndex(ourBoard, TBitGridMakeGridIndexWithPos(ourBoard, 4, 10), true);
    TBitGridSetValueAtIndex(ourBoard, TBitGridMakeGridIndexWithPos(ourBoard, 3, 11), true);
    
    TBitGridSetValueAtIndex(ourBoard, TBitGridMakeGridIndexWithPos(ourBoard, 8, 17), true);
    
    TBitGridSetValueAtIndex(ourBoard, TBitGridMakeGridIndexWithPos(ourBoard, 0, 18), true);
    TBitGridSetValueAtIndex(ourBoard, TBitGridMakeGridIndexWithPos(ourBoard, 1, 18), true);
    TBitGridSetValueAtIndex(ourBoard, TBitGridMakeGridIndexWithPos(ourBoard, 4, 18), true);
    TBitGridSetValueAtIndex(ourBoard, TBitGridMakeGridIndexWithPos(ourBoard, 6, 18), true);
    TBitGridSetValueAtIndex(ourBoard, TBitGridMakeGridIndexWithPos(ourBoard, 7, 18), true);
    TBitGridSetValueAtIndex(ourBoard, TBitGridMakeGridIndexWithPos(ourBoard, 8, 18), true);
    TBitGridSetValueAtIndex(ourBoard, TBitGridMakeGridIndexWithPos(ourBoard, 9, 18), true);
    
    TBitGridSetValueAtIndex(ourBoard, TBitGridMakeGridIndexWithPos(ourBoard, 0, 19), true);
    TBitGridSetValueAtIndex(ourBoard, TBitGridMakeGridIndexWithPos(ourBoard, 1, 19), true);
    TBitGridSetValueAtIndex(ourBoard, TBitGridMakeGridIndexWithPos(ourBoard, 2, 19), true);
    TBitGridSetValueAtIndex(ourBoard, TBitGridMakeGridIndexWithPos(ourBoard, 3, 19), true);
    TBitGridSetValueAtIndex(ourBoard, TBitGridMakeGridIndexWithPos(ourBoard, 4, 19), true);
    TBitGridSetValueAtIndex(ourBoard, TBitGridMakeGridIndexWithPos(ourBoard, 5, 19), true);
    TBitGridSetValueAtIndex(ourBoard, TBitGridMakeGridIndexWithPos(ourBoard, 6, 19), true);
    TBitGridSetValueAtIndex(ourBoard, TBitGridMakeGridIndexWithPos(ourBoard, 7, 19), true);
    TBitGridSetValueAtIndex(ourBoard, TBitGridMakeGridIndexWithPos(ourBoard, 8, 19), true);
    TBitGridSetValueAtIndex(ourBoard, TBitGridMakeGridIndexWithPos(ourBoard, 9, 19), true);
    
    TBitGridChannelSummary(ourBoard, 0, TBitGridChannelSummaryKindTechnical);
    
    TBitGridIterator    *ourScanner = TBitGridIteratorCreate(ourBoard, 1);
    bool                foundCell;
    TCell               cellVal;
    unsigned int        rowVal;
    
    printf("TBitGridIterator - fast scanner initialized\n");
    while ( (foundCell = TBitGridIteratorNext(ourScanner, &P, &cellVal)) && ! cellVal ) {}
    if ( foundCell ) {
        printf("TBitGridIterator - fast scanner next true at position (%u,%u)\n", P.i, P.j);
    }
    TBitGridIteratorDestroy(ourScanner);
    
    ourScanner = TBitGridIteratorCreate(ourBoard, 1);
    printf("TBitGridIterator - fast scanner reinitialized\n");
    if ( TBitGridIteratorNextFullRow(ourScanner, &rowVal) ) {
        printf("TBitGridIterator - fast scanner found full row at position %u\n", rowVal);
    }
    TBitGridIteratorDestroy(ourScanner);
    
    printf("\n");
    
    unsigned int        tetrominoId = 0;
    uint16_t            tetromino;
    
    //
    // Show all of the tetrominos in each orietation:
    //
    while ( tetrominoId < TTetrominosCount ) {
        unsigned int    orientation = 0;
        
        printf("TTetromino[%u]   = 0x%016llX\n", tetrominoId, TTetrominos[tetrominoId]);
        TTetrominoSummary(TTetrominos[tetrominoId]);
        printf("\n");
        tetrominoId++;
    }
    
    printf("\n");
    TTetrominoSummary(TTetrominoShiftUp(TTetrominos[5]));
    printf("\n");
    TTetrominoSummary(TTetrominoShiftDown(TTetrominos[5]));
    printf("\n");
    TTetrominoSummary(TTetrominoShiftLeft(TTetrominos[5]));
    printf("\n");
    TTetrominoSummary(TTetrominoShiftRight(TTetrominos[5]));
    printf("\n");
    
    //
    // Demonstrate shifting of the square tetromino around its unit cell
    //
    tetrominoId = 0;
    tetromino = TTetrominosExtractOrientation(0, 0);
    printf("TTetromino[0,0] Base position:\n");
    TTetrominoOrientationSummary(tetromino);
    tetromino = TTetrominoOrientationShiftUp(tetromino);
    printf("TTetromino[0,0] Shifted up:\n");
    TTetrominoOrientationSummary(tetromino);
    tetromino = TTetrominoOrientationShiftUp(tetromino);
    printf("TTetromino[0,0] Shifted up again:\n");
    TTetrominoOrientationSummary(tetromino);
    tetromino = TTetrominoOrientationShiftLeft(tetromino);
    printf("TTetromino[0,0] Shifted left:\n");
    TTetrominoOrientationSummary(tetromino);
    tetromino = TTetrominoOrientationShiftLeft(tetromino);
    printf("TTetromino[0,0] Shifted left again:\n");
    TTetrominoOrientationSummary(tetromino);
    tetromino = TTetrominoOrientationShiftDown(tetromino);
    printf("TTetromino[0,0] Shifted down:\n");
    TTetrominoOrientationSummary(tetromino);
    tetromino = TTetrominoOrientationShiftRight(tetromino);
    printf("TTetromino[0,0] Shifted right:\n");
    TTetrominoOrientationSummary(tetromino);
    tetromino = TTetrominoOrientationShiftDown(tetromino);
    printf("TTetromino[0,0] Shifted down again:\n");
    TTetrominoOrientationSummary(tetromino);
    tetromino = TTetrominoOrientationShiftRight(tetromino);
    printf("TTetromino[0,0] Shifted right again:\n");
    TTetrominoOrientationSummary(tetromino);
    
    // Load blocks of the grid:
    printf("\n-----\n");
    tetromino = TBitGridExtract4x4AtPosition(ourBoard, 0, TGridPosMake(0, 16));
    TTetrominoOrientationSummary(tetromino);
    printf("\n");
    tetromino = TBitGridExtract4x4AtPosition(ourBoard, 0, TGridPosMake(4, 17));
    TTetrominoOrientationSummary(tetromino);
    printf("\n");
    tetromino = TBitGridExtract4x4AtPosition(ourBoard, 0, TGridPosMake(5, 16));
    TTetrominoOrientationSummary(tetromino);
    
    
    TBitGridDestroy(ourBoard);
}
