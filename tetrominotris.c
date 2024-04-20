
#include "TTetrominos.h"
#include "TBoard.h"
#include "TScoreboard.h"
#include "tui_window.h"

#include <locale.h>

//

void
gameBoardDraw(
    tui_window_ref  the_window,
    WINDOW          *window_ptr,
    const void      *context
)
{
    static uint8_t      tBlockTop[10] = { 0xE2, 0x94, 0x8F, 0xE2, 0x94, 0x81, 0xE2, 0x94, 0x93, '\0' };
    static uint8_t      tBlockBot[10] = { 0xE2, 0x94, 0x97, 0xE2, 0x94, 0x81, 0xE2, 0x94, 0x9B, '\0' };
    
#define THE_BOARD   ((TBoard*)context)
    
    int                 i, j;
    chtype              line1[THE_BOARD->w * 4 + 1];
    chtype              line2[THE_BOARD->w * 4 + 1];
    TBoardIterator      gridScanner = TBoardIteratorMake(THE_BOARD);
    
    j = 0;
    while ( j < THE_BOARD->h ) {
        chtype          *line1Ptr = line1, *line2Ptr = line2;
        
        i = THE_BOARD->w;
        while ( i-- ) {
            bool        cellValue;
            
            if ( TBoardIteratorNext(&gridScanner, &cellValue) && cellValue ) {
                *line1Ptr++ = A_REVERSE | '|'; *line1Ptr++ = A_REVERSE | ' '; *line1Ptr++ = A_REVERSE | ' '; *line1Ptr++ = A_REVERSE | ' ';
                *line2Ptr++ = A_REVERSE | '|'; *line2Ptr++ = A_REVERSE | '_'; *line2Ptr++ = A_REVERSE | '_'; *line2Ptr++ = A_REVERSE | '_';
            } else {
                *line1Ptr++ = ' '; *line1Ptr++ = ' '; *line1Ptr++ = ' '; *line1Ptr++ = ' ';
                *line2Ptr++ = ' '; *line2Ptr++ = ' '; *line2Ptr++ = ' '; *line2Ptr++ = ' ';
            }
        }
        wmove(window_ptr, 1 + 2 * j, 2);
        i = (line1Ptr - line1); line1Ptr = line1;
        while ( i-- ) waddch(window_ptr, *line1Ptr++);
        wmove(window_ptr, 2 + 2 * j, 2);
        i = (line2Ptr - line2); line2Ptr = line2;
        while ( i-- ) waddch(window_ptr, *line2Ptr++);
        j++;
    }

#undef THE_BOARD
}

//

static const char* gameTitleStrings[5] = {
    "   ////// ////// ////// //////   ////  //| //|   //// //| //  ////  ////// //////  ////  ////",
    "    //   //       //   //   // //  // //||//||   //  //||// //  //   //   //   //  //  //    ",
    "   //   //////   //   //////  //  // // ||/ //  //  // ||/ //  //   //   //////   //   ///   ",
    "  //   //       //   //  ||  //  // //     //  //  //  // //  //   //   //  ||   //      //  ",
    " //   //////   //   //   ||  ////  //     // //// //  //  ////    //   //   || ////  ////    "
};

void
drawGameTitle(
    WINDOW      *inWindow
)
{
    wattron(inWindow, A_BOLD);
    mvwprintw(inWindow, 1, 2, "%s", gameTitleStrings[0]);
    mvwprintw(inWindow, 2, 2, "%s", gameTitleStrings[1]);
    mvwprintw(inWindow, 3, 2, "%s", gameTitleStrings[2]);
    mvwprintw(inWindow, 4, 2, "%s", gameTitleStrings[3]);
    mvwprintw(inWindow, 5, 2, "%s", gameTitleStrings[4]);
    wattroff(inWindow, A_BOLD);
}

//

typedef struct {
    TScoreboard *scoreboard;
    uint16_t    tetrominoReps[TTetrominosCount];
} TStatsData;

static inline TStatsData
TStatsDataMake(
    TScoreboard *scoreboard
)
{
    TStatsData  newStatsData = {
                    .scoreboard = scoreboard,
                    .tetrominoReps = {
                        TTetrominoOrientationShiftUp(TTetrominosExtractOrientation(2, 3)),
                        TTetrominoOrientationShiftLeft(TTetrominoOrientationShiftUp(TTetrominosExtractOrientation(6, 3))),
                        TTetrominoOrientationShiftLeft(TTetrominoOrientationShiftUp(TTetrominosExtractOrientation(4, 1))),
                        TTetrominoOrientationShiftUp(TTetrominosExtractOrientation(0, 0)),
                        TTetrominoOrientationShiftLeft(TTetrominoOrientationShiftUp(TTetrominosExtractOrientation(3, 1))),
                        TTetrominoOrientationShiftLeft(TTetrominoOrientationShiftUp(TTetrominosExtractOrientation(5, 1))),
                        TTetrominoOrientationShiftUp(TTetrominosExtractOrientation(1, 3))
                    }
                };
    return newStatsData;
}

void
statsDraw(
    tui_window_ref  the_window,
    WINDOW          *window_ptr,
    const void      *context
)
{
#define THE_STATS_DATA      ((TStatsData*)context)
#define THE_SCOREBOARD      THE_STATS_DATA->scoreboard
    unsigned int            tIdx;
    
    wclear(window_ptr);
    wattron(window_ptr, A_UNDERLINE);
    mvwprintw(window_ptr, 2, 1, "%-20s", "Lines");
    wattroff(window_ptr, A_UNDERLINE);
    mvwprintw(window_ptr, 3, 2, "   Total%8u", THE_SCOREBOARD->nLinesTotal);
    mvwprintw(window_ptr, 4, 2, "  Single%8u", THE_SCOREBOARD->nLinesOfType[TScoreboardLineCountTypeSingle]);
    mvwprintw(window_ptr, 5, 2, "  Double%8u", THE_SCOREBOARD->nLinesOfType[TScoreboardLineCountTypeDouble]);
    mvwprintw(window_ptr, 6, 2, "  Triple%8u", THE_SCOREBOARD->nLinesOfType[TScoreboardLineCountTypeTriple]);
    mvwprintw(window_ptr, 7, 2, "    Quad%8u", THE_SCOREBOARD->nLinesOfType[TScoreboardLineCountTypeQuadruple]);
    
    wattron(window_ptr, A_UNDERLINE);
    mvwprintw(window_ptr, 9, 1, "%-20s", "Tetrominos");
    wattroff(window_ptr, A_UNDERLINE);
    
    tIdx = 0;
    while ( tIdx < TTetrominosCount ) {
        uint16_t        rep, mask = 0x8000;
        unsigned int    count;
        
        rep = THE_STATS_DATA->tetrominoReps[tIdx];
        wmove(window_ptr, 11 + 3 * tIdx, 2);
        count = 4; while ( count-- ) {
            if (rep & mask) {
                waddch(window_ptr, A_REVERSE | '|');
                waddch(window_ptr, A_REVERSE | '_');
            } else {
                waddch(window_ptr, ' ');
                waddch(window_ptr, ' ');
            }
            mask >>= 1;
        }
        wprintw(window_ptr, "%8u", THE_SCOREBOARD->tetrominosOfType[tIdx]);
        wmove(window_ptr, 12 + 3 * tIdx, 2);
        count = 4; while ( count-- ) {
            if (rep & mask) {
                waddch(window_ptr, A_REVERSE | '|');
                waddch(window_ptr, A_REVERSE | '_');
            } else {
                waddch(window_ptr, ' ');
                waddch(window_ptr, ' ');
            }
            mask >>= 1;
        }
        tIdx++;
    }
#undef THE_SCOREBOARD
#undef THE_STATS_DATA
}

//

void
scoreboardDraw(
    tui_window_ref  the_window,
    WINDOW          *window_ptr,
    const void      *context
)
{
#define THE_SCOREBOARD      ((TScoreboard*)context)
    
    wclear(window_ptr);
    
    wattron(window_ptr, A_UNDERLINE);
        mvwprintw(window_ptr, 2, 2, "%-18s", "Points");
    wattroff(window_ptr, A_UNDERLINE);
    mvwprintw(window_ptr, 4, 2, "%18u", THE_SCOREBOARD->score);
    
    wattron(window_ptr, A_UNDERLINE);
        mvwprintw(window_ptr, 6, 2, "%-18s", "Level");
    wattroff(window_ptr, A_UNDERLINE);
    mvwprintw(window_ptr, 8, 2, "%18u", THE_SCOREBOARD->level);

#undef THE_SCOREBOARD
}

//

typedef struct {
    uint16_t        tetromino;
} TNextUp;

void
nextTetrominoDraw(
    tui_window_ref  the_window,
    WINDOW          *window_ptr,
    const void      *context
)
{
#define NEXT_UP     ((TNextUp*)context)
    chtype          line1[4 * 4], *line1Ptr = line1;
    chtype          line2[4 * 4], *line2Ptr = line2;
    unsigned int    count, j = 0;
    uint16_t        rep, mask = 0x8000;
    
    wclear(window_ptr);

    rep = NEXT_UP->tetromino;
    
    while ( j < 4 ) {
        line1Ptr = line1, line2Ptr = line2;
        count = 4; while ( count-- ) {
            if (rep & mask) {
                *line1Ptr++ = A_REVERSE | '|'; *line1Ptr++ = A_REVERSE | ' '; *line1Ptr++ = A_REVERSE | ' '; *line1Ptr++ = A_REVERSE | ' ';
                *line2Ptr++ = A_REVERSE | '|'; *line2Ptr++ = A_REVERSE | '_'; *line2Ptr++ = A_REVERSE | '_'; *line2Ptr++ = A_REVERSE | '_';
            } else {
                *line1Ptr++ = ' '; *line1Ptr++ = ' '; *line1Ptr++ = ' '; *line1Ptr++ = ' ';
                *line2Ptr++ = ' '; *line2Ptr++ = ' '; *line2Ptr++ = ' '; *line2Ptr++ = ' ';
            }
            mask >>= 1;
        }
        wmove(window_ptr, 2 + 2 * j, 3);
        count = line1Ptr - line1; line1Ptr = line1;
        while ( count-- ) waddch(window_ptr, *line1Ptr++);
        wmove(window_ptr, 3 + 2 * j, 3);
        count = line2Ptr - line2; line2Ptr = line2;
        while ( count-- ) waddch(window_ptr, *line2Ptr++);
        
        j++;
    }
#undef NEXT_UP
}

//

int
main()
{
    WINDOW          *mainWindow = NULL;
    tui_window_ref  gameBoardWindow = NULL, statsWindow = NULL, scoreWindow = NULL,
                    nextTetrominoWindow = NULL;
    int             keyCh;
    TBoard          *gameBoard = NULL;
    TScoreboard     gameScoreboard;
    TStatsData      statsData = TStatsDataMake(&gameScoreboard);
    TNextUp         nextUp = {
                        .tetromino = TTetrominosExtractOrientation(1, 3)
                    };
    
    setlocale(LC_ALL, "");
    
    // Initialize the curses screen:
    mainWindow = initscr();
    if ( ! mainWindow ) {
        fprintf(stdout, "ERROR:  unable to initialize screen\n");
        exit(EINVAL);
    }

    // Turn off key echoing — we don't want what we type showing
    // up on the screen:
    noecho();
    
    // Enable the keypad:
    keypad(mainWindow, TRUE);
    
    // Create the game board:
    gameBoard = TBoardCreate(10, 15);
    
    // Create the scoreboard:
    gameScoreboard = TScoreboardMake();

    // Fake-up the initial layout
    TBoardFill(gameBoard, false);
    
    TBoardSetValueAtGridIndex(gameBoard, TBoardGridIndexMakeWithPos(gameBoard, 2, 4), true);
    TBoardSetValueAtGridIndex(gameBoard, TBoardGridIndexMakeWithPos(gameBoard, 3, 4), true);
    TBoardSetValueAtGridIndex(gameBoard, TBoardGridIndexMakeWithPos(gameBoard, 4, 4), true);
    TBoardSetValueAtGridIndex(gameBoard, TBoardGridIndexMakeWithPos(gameBoard, 3, 5), true);
    
    TBoardSetValueAtGridIndex(gameBoard, TBoardGridIndexMakeWithPos(gameBoard, 8, 12), true);
    
    TBoardSetValueAtGridIndex(gameBoard, TBoardGridIndexMakeWithPos(gameBoard, 0, 13), true);
    TBoardSetValueAtGridIndex(gameBoard, TBoardGridIndexMakeWithPos(gameBoard, 1, 13), true);
    TBoardSetValueAtGridIndex(gameBoard, TBoardGridIndexMakeWithPos(gameBoard, 4, 13), true);
    TBoardSetValueAtGridIndex(gameBoard, TBoardGridIndexMakeWithPos(gameBoard, 5, 13), true);
    TBoardSetValueAtGridIndex(gameBoard, TBoardGridIndexMakeWithPos(gameBoard, 6, 13), true);
    TBoardSetValueAtGridIndex(gameBoard, TBoardGridIndexMakeWithPos(gameBoard, 7, 13), true);
    TBoardSetValueAtGridIndex(gameBoard, TBoardGridIndexMakeWithPos(gameBoard, 8, 13), true);
    TBoardSetValueAtGridIndex(gameBoard, TBoardGridIndexMakeWithPos(gameBoard, 9, 13), true);
    
    TBoardSetValueAtGridIndex(gameBoard, TBoardGridIndexMakeWithPos(gameBoard, 0, 14), true);
    TBoardSetValueAtGridIndex(gameBoard, TBoardGridIndexMakeWithPos(gameBoard, 1, 14), true);
    TBoardSetValueAtGridIndex(gameBoard, TBoardGridIndexMakeWithPos(gameBoard, 2, 14), true);
    TBoardSetValueAtGridIndex(gameBoard, TBoardGridIndexMakeWithPos(gameBoard, 3, 14), true);
    TBoardSetValueAtGridIndex(gameBoard, TBoardGridIndexMakeWithPos(gameBoard, 4, 14), true);
    TBoardSetValueAtGridIndex(gameBoard, TBoardGridIndexMakeWithPos(gameBoard, 5, 14), true);
    TBoardSetValueAtGridIndex(gameBoard, TBoardGridIndexMakeWithPos(gameBoard, 6, 14), true);
    TBoardSetValueAtGridIndex(gameBoard, TBoardGridIndexMakeWithPos(gameBoard, 7, 14), true);
    TBoardSetValueAtGridIndex(gameBoard, TBoardGridIndexMakeWithPos(gameBoard, 8, 14), true);
    TBoardSetValueAtGridIndex(gameBoard, TBoardGridIndexMakeWithPos(gameBoard, 9, 14), true);
    
    // Create our windows:
    gameBoardWindow = tui_window_alloc(26, 7, 44, 32, 0, NULL, 0, gameBoardDraw, (const void*)gameBoard);
    statsWindow = tui_window_alloc(4, 7, 20, 32, 0, "STATS", 0, statsDraw, (const void*)&statsData);
    scoreWindow = tui_window_alloc(72, 7, 22, 10, tui_window_opts_title_align_right, "SCORE", 0, scoreboardDraw, (const void*)&gameScoreboard);
    nextTetrominoWindow = tui_window_alloc(72, 18, 22, 12, tui_window_opts_title_align_right, "NEXT UP", 0, nextTetrominoDraw, (const void*)&nextUp);
    
    if ( gameBoardWindow && statsWindow) {
        refresh();
        drawGameTitle(mainWindow);
        tui_window_refresh(gameBoardWindow, 1);
        tui_window_refresh(statsWindow, 1);
        tui_window_refresh(scoreWindow, 1);
        tui_window_refresh(nextTetrominoWindow, 1);
        doupdate();
    
        while ( (keyCh = getch()) != 'q' ) {
            switch ( keyCh ) {
                case 's':
                case 'S':
                    TBoardScroll(gameBoard);
                    break;
                case 'c':
                case 'C':
                    TBoardClearLines(gameBoard, 13, 13);
                    break;
                case 'n':
                case 'N':
                    nextUp.tetromino = TTetrominosExtractOrientation(random() % 7, random() % 4);
                    break;
                case '1':
                case '2':
                case '3':
                case '4':
                case '5':
                case '6':
                case '7':
                    gameScoreboard.tetrominosOfType[keyCh - '1']++;
                    break;
                case 'p':
                    TScoreboardAddLinesOfType(&gameScoreboard, 1 + random() % 4);
                    break;
            }
            refresh();
            drawGameTitle(mainWindow);
            tui_window_refresh(gameBoardWindow, 1);
            tui_window_refresh(statsWindow, 1);
            tui_window_refresh(scoreWindow, 1);
            tui_window_refresh(nextTetrominoWindow, 1);
            doupdate();
        }
        tui_window_free(statsWindow);
        tui_window_free(gameBoardWindow);
        tui_window_free(scoreWindow);
    }
    delwin(mainWindow);
    endwin();
    refresh();
    
    return 0;
}
