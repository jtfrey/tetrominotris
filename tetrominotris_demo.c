
#include "TTetrominos.h"
#include "TBoard.h"

int
main()
{
    TBoard      *ourBoard = TBoardCreate(10, 20);
    TGridPos    P;
    TGridIndex  I;
    
    P = TGridPosMake(0,0); I = TBoardGridPosToIndex(ourBoard, P);
    printf("P = { %u, %u } => I = { %u, %u }\n", P.i, P.j, I.W, I.b);
    
    P = TBoardGridIndexToPos(ourBoard, I);
    printf("I = { %u, %u } => P = { %u, %u }\n", I.W, I.b, P.i, P.j);
    
    P = TGridPosMake(5,10); I = TBoardGridPosToIndex(ourBoard, P);
    printf("P = { %u, %u } => I = { %u, %u }\n", P.i, P.j, I.W, I.b);
    P = TGridPosMake(6,10); I = TBoardGridPosToIndex(ourBoard, P);
    printf("P = { %u, %u } => I = { %u, %u }\n", P.i, P.j, I.W, I.b);
    P = TGridPosMake(6,11); I = TBoardGridPosToIndex(ourBoard, P);
    printf("P = { %u, %u } => I = { %u, %u }\n", P.i, P.j, I.W, I.b);
    
    TBoardFill(ourBoard, false);
    
    TBoardSetValueAtGridIndex(ourBoard, TBoardGridIndexMakeWithPos(ourBoard, 2, 10), true);
    TBoardSetValueAtGridIndex(ourBoard, TBoardGridIndexMakeWithPos(ourBoard, 3, 10), true);
    TBoardSetValueAtGridIndex(ourBoard, TBoardGridIndexMakeWithPos(ourBoard, 4, 10), true);
    TBoardSetValueAtGridIndex(ourBoard, TBoardGridIndexMakeWithPos(ourBoard, 3, 11), true);
    
    TBoardSetValueAtGridIndex(ourBoard, TBoardGridIndexMakeWithPos(ourBoard, 8, 17), true);
    
    TBoardSetValueAtGridIndex(ourBoard, TBoardGridIndexMakeWithPos(ourBoard, 0, 18), true);
    TBoardSetValueAtGridIndex(ourBoard, TBoardGridIndexMakeWithPos(ourBoard, 1, 18), true);
    TBoardSetValueAtGridIndex(ourBoard, TBoardGridIndexMakeWithPos(ourBoard, 2, 18), true);
    TBoardSetValueAtGridIndex(ourBoard, TBoardGridIndexMakeWithPos(ourBoard, 3, 18), true);
    TBoardSetValueAtGridIndex(ourBoard, TBoardGridIndexMakeWithPos(ourBoard, 4, 18), true);
    TBoardSetValueAtGridIndex(ourBoard, TBoardGridIndexMakeWithPos(ourBoard, 5, 18), true);
    TBoardSetValueAtGridIndex(ourBoard, TBoardGridIndexMakeWithPos(ourBoard, 6, 18), true);
    TBoardSetValueAtGridIndex(ourBoard, TBoardGridIndexMakeWithPos(ourBoard, 7, 18), true);
    TBoardSetValueAtGridIndex(ourBoard, TBoardGridIndexMakeWithPos(ourBoard, 8, 18), true);
    TBoardSetValueAtGridIndex(ourBoard, TBoardGridIndexMakeWithPos(ourBoard, 9, 18), true);
    
    TBoardSetValueAtGridIndex(ourBoard, TBoardGridIndexMakeWithPos(ourBoard, 0, 19), true);
    TBoardSetValueAtGridIndex(ourBoard, TBoardGridIndexMakeWithPos(ourBoard, 1, 19), true);
    TBoardSetValueAtGridIndex(ourBoard, TBoardGridIndexMakeWithPos(ourBoard, 2, 19), true);
    TBoardSetValueAtGridIndex(ourBoard, TBoardGridIndexMakeWithPos(ourBoard, 3, 19), true);
    TBoardSetValueAtGridIndex(ourBoard, TBoardGridIndexMakeWithPos(ourBoard, 4, 19), true);
    TBoardSetValueAtGridIndex(ourBoard, TBoardGridIndexMakeWithPos(ourBoard, 5, 19), true);
    TBoardSetValueAtGridIndex(ourBoard, TBoardGridIndexMakeWithPos(ourBoard, 6, 19), true);
    TBoardSetValueAtGridIndex(ourBoard, TBoardGridIndexMakeWithPos(ourBoard, 7, 19), true);
    TBoardSetValueAtGridIndex(ourBoard, TBoardGridIndexMakeWithPos(ourBoard, 8, 19), true);
    TBoardSetValueAtGridIndex(ourBoard, TBoardGridIndexMakeWithPos(ourBoard, 9, 19), true);
    TBoardSummary(ourBoard);
    
    TBoardIterator  ourScanner = TBoardIteratorMake(ourBoard);
    bool            foundCell, cellVal;
    unsigned int    rowVal;
    
    printf("TBoardIterator - fast scanner initialized\n");
    while ( (foundCell = TBoardIteratorNext(&ourScanner, &cellVal)) && ! cellVal ) {}
    if ( foundCell ) {
        printf("TBoardIterator - fast scanner next true at position (%u,%u)\n", ourScanner.i, ourScanner.j);
    }
    
    ourScanner = TBoardIteratorMake(ourBoard);
    printf("TBoardIterator - fast scanner reinitialized\n");
    if ( TBoardIteratorNextFullRow(&ourScanner, &rowVal) ) {
        printf("TBoardIterator - fast scanner found full row at position %u\n", rowVal);
    }
    
    TBoardDestroy(ourBoard);
    
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
    exit(0);
    
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
}
