
#include "TTetrominos.h"
#include "TBoard.h"
#include "TGridScanner.h"
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
    char                line1[THE_BOARD->w * 10 + 1];
    char                line2[THE_BOARD->w * 10 + 1];
    TGridScanner        gridScanner = TGridScannerMake(THE_BOARD);
    
    //wclear(window_ptr);
    j = 0;
    while ( j < THE_BOARD->h ) {
        char            *line1Ptr = line1, *line2Ptr = line2;
        int             line1Len = sizeof(line1), line2Len = sizeof(line2);
        
        i = 0;
        while ( i < THE_BOARD->w ) {
            bool        cellValue;
            int         l;
            
            if ( TGridScannerNext(&gridScanner, &cellValue) && cellValue ) {
                l = snprintf(line1Ptr, line1Len, "%s", (const char*)tBlockTop); line1Len -= l; line1Ptr += l;
                l = snprintf(line2Ptr, line2Len, "%s", (const char*)tBlockBot); line2Len -= l; line2Ptr += l;
            } else {
                l = snprintf(line1Ptr, line1Len, "   "); line1Len -= l; line1Ptr += l;
                l = snprintf(line2Ptr, line2Len, "   "); line2Len -= l; line2Ptr += l;
            }
            i++;
        }
        wmove(window_ptr, 1 + 2 * j, 2);
        attron(COLOR_PAIR(1));
        waddstr(window_ptr, line1);
        wmove(window_ptr, 2 + 2 * j, 2);
        waddstr(window_ptr, line2);
        attroff(COLOR_PAIR(1));
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
    static bool is_dim = true;
    
    attron(is_dim ? A_DIM : A_BOLD);
    mvwprintw(inWindow, 1, 2, "%s", gameTitleStrings[0]);
    mvwprintw(inWindow, 2, 2, "%s", gameTitleStrings[1]);
    mvwprintw(inWindow, 3, 2, "%s", gameTitleStrings[2]);
    mvwprintw(inWindow, 4, 2, "%s", gameTitleStrings[3]);
    mvwprintw(inWindow, 5, 2, "%s", gameTitleStrings[4]);
    attroff(is_dim ? A_DIM : A_BOLD);
    is_dim = ! is_dim;
}

//

int
main()
{
    WINDOW          *mainWindow = NULL;
    tui_window_ref  gameBoardWindow = NULL;
    int             keyCh;
    TBoard          *gameBoard = NULL;
    
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
    
    // Create the game board window:
    gameBoard = TBoardCreate(10, 15);

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
    
    
    gameBoardWindow = tui_window_alloc(4, 7, 34, 32, 0, NULL, 0, gameBoardDraw, (const void*)gameBoard);
    if ( gameBoardWindow ) {
        refresh();
        drawGameTitle(mainWindow);
        tui_window_refresh(gameBoardWindow, 0);
        while ( (keyCh = getch()) != 'q' ) {
            if ( keyCh == 's' ) TBoardScroll(gameBoard);
            drawGameTitle(mainWindow);
            tui_window_refresh(gameBoardWindow, 0);
        }

        tui_window_free(gameBoardWindow);
    }
    delwin(mainWindow);
    endwin();
    refresh();
    
    return 0;
}
