
#include "TTetrominos.h"
#include "TBitGrid.h"
#include "TSprite.h"
#include "TScoreboard.h"
#include "TGameEngine.h"
#include "tui_window.h"

#include <locale.h>
#include <langinfo.h>

//

#include <getopt.h>

static struct option cliArgOpts[] = {
    { "help",       no_argument,        NULL,       'h' },
    { "width",      required_argument,  NULL,       'w' },
    { "height",     required_argument,  NULL,       'H' },
    { NULL,         0,                  NULL,        0  }
};
static const char *cliArgOptsStr = "hw:H:";

//

void
usage(
    const char  *exe
)
{
    printf(
        "\n"
        "usage:\n"
        "\n"
        "    %s {options}\n"
        "\n"
        "  options:\n"
        "\n"
        "    --help/-h                      show this information\n"
        "    --width/-w <dimension>         choose the game board width\n"
        "    --height/-h <dimension>        choose the game board height\n"
        "\n"
        "    <dimension> = # | default | fit\n"
        "              # = a positive integer value\n"
        "        default = 10 wide or 20 high\n"
        "            fit = adjust the width to fit the terminal\n"
        "\n",
        exe
    );
}

//

bool
parseWidth(
    const char      *optstr,
    int             *width
)
{
    long            v;
    char            *endptr;
    
    if ( ! strcasecmp(optstr, "default") ) {
        *width = 10;
        return true;
    }
    if ( ! strcasecmp(optstr, "fit") ) {
        *width = -1;
        return true;
    }
    v = strtol(optstr, &endptr, 0);
    if ( endptr > optstr ) {
        if ( v >= 10 ) {
            *width = v;
            return true;
        }
    }
    fprintf(stdout, "ERROR:  invalid width provided: %s\n", optstr);
    return false;
}

//

bool
parseHeight(
    const char      *optstr,
    int             *height
)
{
    long            v;
    char            *endptr;
    
    if ( ! strcasecmp(optstr, "default") ) {
        *height = 20;
        return true;
    }
    if ( ! strcasecmp(optstr, "fit") ) {
        *height = -1;
        return true;
    }
    v = strtol(optstr, &endptr, 0);
    if ( endptr > optstr ) {
        if ( v >= 20 ) {
            *height = v;
            return true;
        }
    }
    fprintf(stdout, "ERROR:  invalid height provided: %s\n", optstr);
    return false;
}

//

static bool
doesSupportUTF8(void)
{
    static bool hasBeenChecked = false;
    static bool __doesSupportUTF8;
    
    if ( ! hasBeenChecked ) {
        const char  *encoding = nl_langinfo(CODESET);
        __doesSupportUTF8 = ( ! strcasecmp(encoding, "utf8") || ! strcasecmp(encoding, "utf-8") );
        hasBeenChecked = true;
    }
    return __doesSupportUTF8;
}

//

static const char* gamePausedStrings[3] = {
                        "             ",
                        "  P A U S E  ",
                        "             "
                    };
static unsigned int gamePausedStringsLen = 13;

void
gameBoardDraw(
    tui_window_ref  the_window,
    WINDOW          *window_ptr,
    const void      *context
)
{ 
#define GAME_ENGINE ((TGameEngine*)context)
#define THE_BOARD   GAME_ENGINE->gameBoard
#define THE_PIECE   GAME_ENGINE->currentSprite
    
    int                 i, j, spriteILo, spriteIHi, spriteJLo, spriteJHi, extraIShift = 0;
    chtype              line1[THE_BOARD->dimensions.w * 4 + 1];
    chtype              line2[THE_BOARD->dimensions.w * 4 + 1];
    TBitGridIterator    *gridScanner = TBitGridIteratorCreate(THE_BOARD, 1);
    TGridPos            P;
    uint16_t            spriteBits = TSpriteGet4x4(&THE_PIECE);
    
    // Skip any lines that are off-screen above the board:
    if ( THE_PIECE.P.j < 0 ) {
        spriteBits >>= 4 * (-THE_PIECE.P.j);
        spriteJLo = 0;
        spriteJHi = 4 + THE_PIECE.P.j;
    } else {
        spriteJLo = THE_PIECE.P.j;
        spriteJHi = 4 + spriteJLo;
    }
    
    // If the piece is off-screen to the left then force a
    // left-shift:
    if ( THE_PIECE.P.i < 0 ) {
        int     steps = THE_PIECE.P.i;
        
        while ( steps++ < 0 )
            spriteBits = (spriteBits & 0xEEEE) >> 1;
        spriteILo = 0;
        spriteIHi = 4 + THE_PIECE.P.i;
        extraIShift = -THE_PIECE.P.i;
    } else {
        spriteILo = THE_PIECE.P.i;
        spriteIHi = 4 + spriteILo;
        extraIShift = (spriteIHi > THE_BOARD->dimensions.w) ? (spriteIHi - THE_BOARD->dimensions.w) : 0;
    }
    
    j = 0;
    while ( j < THE_BOARD->dimensions.h ) {
        chtype          *line1Ptr = line1, *line2Ptr = line2;
        
        i = 0;
        while ( i < THE_BOARD->dimensions.w ) {
            TCell       cellValue;
            bool        spriteBit = false, gridBit = TBitGridIteratorNext(gridScanner, &P, &cellValue);
            
            // Sprite?
            if ((j >= spriteJLo) && (j < spriteJHi) && (i >= spriteILo) && (i < spriteIHi)) {
                spriteBit = spriteBits & 0x1;
                spriteBits >>= 1;
            }
            
            if ( spriteBit || (gridBit && cellValue) ) {
                *line1Ptr++ = A_REVERSE | '|'; *line1Ptr++ = A_REVERSE | ' '; *line1Ptr++ = A_REVERSE | ' '; *line1Ptr++ = A_REVERSE | ' ';
                *line2Ptr++ = A_REVERSE | '|'; *line2Ptr++ = A_REVERSE | '_'; *line2Ptr++ = A_REVERSE | '_'; *line2Ptr++ = A_REVERSE | '_';
            } else {
                *line1Ptr++ = ' '; *line1Ptr++ = ' '; *line1Ptr++ = ' '; *line1Ptr++ = ' ';
                *line2Ptr++ = ' '; *line2Ptr++ = ' '; *line2Ptr++ = ' '; *line2Ptr++ = ' ';
            }
            i++;
        }
        if ( (j >= spriteJLo) && (j < spriteJHi) && extraIShift ) spriteBits >>= extraIShift;
        wmove(window_ptr, 1 + 2 * j, 2);
        i = (line1Ptr - line1); line1Ptr = line1;
        while ( i-- ) waddch(window_ptr, *line1Ptr++);
        wmove(window_ptr, 2 + 2 * j, 2);
        i = (line2Ptr - line2); line2Ptr = line2;
        while ( i-- ) waddch(window_ptr, *line2Ptr++);
        j++;
    }
    TBitGridIteratorDestroy(gridScanner);
    
    if ( GAME_ENGINE->isPaused ) {
        int     x = 2 + (4 * THE_BOARD->dimensions.w - gamePausedStringsLen) / 2;
        int     y = THE_BOARD->dimensions.h - 4;
        
        wattron(window_ptr, A_BLINK);
        mvwprintw(window_ptr, y++, x, "%s", gamePausedStrings[0]);
        mvwprintw(window_ptr, y++, x, "%s", gamePausedStrings[1]);
        mvwprintw(window_ptr, y, x, "%s", gamePausedStrings[2]);
        wattroff(window_ptr, A_BLINK);
    }

#undef THE_PIECE
#undef THE_BOARD
#undef GAME_ENGINE
}

//

static const char* gameTitleStrings[5] = {
    "   ////// ////// ////// //////   ////  //| //|   //// //| //  ////  ////// //////  ////  ////",
    "    //   //       //   //   // //  // //||//||   //  //||// //  //   //   //   //  //  //    ",
    "   //   //////   //   //////  //  // // ||/ //  //  // ||/ //  //   //   //////   //   ///   ",
    "  //   //       //   //  ||  //  // //     //  //  //  // //  //   //   //  ||   //      //  ",
    " //   //////   //   //   ||  ////  //     // //// //  //  ////    //   //   || ////  ////    "
};
static const int gameTitleStringsLen = 93;

void
drawGameTitle(
    WINDOW          *inWindow,
    unsigned int    w
)
{
    int             extraSpace = w - gameTitleStringsLen - 4;
    
    if ( extraSpace >= 0 ) {
        char        slashes[extraSpace / 2 + 1], whitespace[extraSpace / 2 + 1];
        int         leadSlashCount = extraSpace / 2, leadWhitespaceCount = 0;
        int         trailWhitespaceCount = 4, trailSlashCount = leadSlashCount - 4;
        
        memset(slashes, '/', sizeof(slashes)); slashes[extraSpace / 2] = '\0';
        memset(whitespace, ' ', sizeof(whitespace)); whitespace[extraSpace / 2] = '\0';
        
        wattron(inWindow, A_BOLD);
        mvwprintw(inWindow, 1, 2, "%.*s%.*s%s%.*s%.*s", leadSlashCount, slashes, leadWhitespaceCount, whitespace,
                                                    gameTitleStrings[0],
                                                    trailWhitespaceCount, whitespace, trailSlashCount, slashes);
        leadSlashCount--, leadWhitespaceCount++, trailWhitespaceCount--, trailSlashCount++;
        mvwprintw(inWindow, 2, 2, "%.*s%.*s%s%.*s%.*s", leadSlashCount, slashes, leadWhitespaceCount, whitespace,
                                                    gameTitleStrings[1],
                                                    trailWhitespaceCount, whitespace, trailSlashCount, slashes);
        leadSlashCount--, leadWhitespaceCount++, trailWhitespaceCount--, trailSlashCount++;
        mvwprintw(inWindow, 3, 2, "%.*s%.*s%s%.*s%.*s", leadSlashCount, slashes, leadWhitespaceCount, whitespace,
                                                    gameTitleStrings[2],
                                                    trailWhitespaceCount, whitespace, trailSlashCount, slashes);
        leadSlashCount--, leadWhitespaceCount++, trailWhitespaceCount--, trailSlashCount++;
        mvwprintw(inWindow, 4, 2, "%.*s%.*s%s%.*s%.*s", leadSlashCount, slashes, leadWhitespaceCount, whitespace,
                                                    gameTitleStrings[3],
                                                    trailWhitespaceCount, whitespace, trailSlashCount, slashes);
        leadSlashCount--, leadWhitespaceCount++, trailWhitespaceCount--, trailSlashCount++;
        mvwprintw(inWindow, 5, 2, "%.*s%.*s%s%.*s%.*s", leadSlashCount, slashes, leadWhitespaceCount, whitespace,
                                                    gameTitleStrings[4],
                                                    trailWhitespaceCount, whitespace, trailSlashCount, slashes);
        wattroff(inWindow, A_BOLD);
    }
}

//

void
statsDraw(
    tui_window_ref  the_window,
    WINDOW          *window_ptr,
    const void      *context
)
{
#define GAME_ENGINE ((TGameEngine*)context)
#define SCOREBOARD  GAME_ENGINE->scoreboard
    unsigned int    tIdx, colorIdx = 0;
    
    wclear(window_ptr);
    wattron(window_ptr, A_UNDERLINE);
    mvwprintw(window_ptr, 2, 1, " %-18s ", "Lines");
    wattroff(window_ptr, A_UNDERLINE);
    mvwprintw(window_ptr, 3, 2, "   Total%8u", SCOREBOARD.nLinesTotal);
    mvwprintw(window_ptr, 4, 2, "  Single%8u", SCOREBOARD.nLinesOfType[TScoreboardLineCountTypeSingle]);
    mvwprintw(window_ptr, 5, 2, "  Double%8u", SCOREBOARD.nLinesOfType[TScoreboardLineCountTypeDouble]);
    mvwprintw(window_ptr, 6, 2, "  Triple%8u", SCOREBOARD.nLinesOfType[TScoreboardLineCountTypeTriple]);
    mvwprintw(window_ptr, 7, 2, "    Quad%8u", SCOREBOARD.nLinesOfType[TScoreboardLineCountTypeQuadruple]);
    
    wattron(window_ptr, A_UNDERLINE);
    mvwprintw(window_ptr, 9, 1, " %-18s ", "Tetrominos");
    wattroff(window_ptr, A_UNDERLINE);
    
    tIdx = 0;
    while ( tIdx < TTetrominosCount ) {
        uint16_t        rep, mask = 0x0001;
        unsigned int    count;
        
        rep = GAME_ENGINE->terominoRepsForStats[tIdx];
        wmove(window_ptr, 11 + 3 * tIdx, 2);
        count = 4; while ( count-- ) {
            if (rep & mask) {
                waddch(window_ptr, '|' | A_REVERSE);
                waddch(window_ptr, '_' | A_REVERSE);
            } else {
                waddch(window_ptr, ' ');
                waddch(window_ptr, ' ');
            }
            mask <<= 1;
        }
        wprintw(window_ptr, "%8u", SCOREBOARD.tetrominosOfType[tIdx]);
        wmove(window_ptr, 12 + 3 * tIdx, 2);
        count = 4; while ( count-- ) {
            if (rep & mask) {
                waddch(window_ptr, '|' | A_REVERSE);
                waddch(window_ptr, '_' | A_REVERSE);
            } else {
                waddch(window_ptr, ' ');
                waddch(window_ptr, ' ');
            }
            mask <<= 1;
        }
        tIdx++;
        colorIdx = (colorIdx + 1) % 4;
    }
#undef SCOREBOARD
#undef GAME_ENGINE
}

//

void
scoreboardDraw(
    tui_window_ref  the_window,
    WINDOW          *window_ptr,
    const void      *context
)
{
#define GAME_ENGINE ((TGameEngine*)context)
#define SCOREBOARD  GAME_ENGINE->scoreboard
    
    wclear(window_ptr);
    
    wattron(window_ptr, A_UNDERLINE);
        mvwprintw(window_ptr, 2, 1, " %-18s ", "Points");
    wattroff(window_ptr, A_UNDERLINE);
    mvwprintw(window_ptr, 4, 2, "%18u", SCOREBOARD.score);
    
    wattron(window_ptr, A_UNDERLINE);
        mvwprintw(window_ptr, 6, 1, " %-18s ", "Level");
    wattroff(window_ptr, A_UNDERLINE);
    mvwprintw(window_ptr, 8, 2, "%18u", SCOREBOARD.level);

#undef SCOREBOARD
#undef GAME_ENGINE
}

//

void
nextTetrominoDraw(
    tui_window_ref  the_window,
    WINDOW          *window_ptr,
    const void      *context
)
{
#define GAME_ENGINE ((TGameEngine*)context)
    chtype          line1[4 * 4], *line1Ptr = line1;
    chtype          line2[4 * 4], *line2Ptr = line2;
    unsigned int    count, j = 0;
    uint16_t        rep, mask = 0x0001;
    
    wclear(window_ptr);

    rep = TSpriteGet4x4(&GAME_ENGINE->nextSprite);
    
    while ( j < 4 ) {
        line1Ptr = line1, line2Ptr = line2;
        count = 4; while ( count-- ) {
            if (rep & mask) {
                *line1Ptr++ = '|' | A_REVERSE; *line1Ptr++ = ' ' | A_REVERSE; *line1Ptr++ = ' ' | A_REVERSE; *line1Ptr++ = ' ' | A_REVERSE;
                *line2Ptr++ = '|' | A_REVERSE; *line2Ptr++ = '_' | A_REVERSE; *line2Ptr++ = '_' | A_REVERSE; *line2Ptr++ = '_' | A_REVERSE;
            } else {
                *line1Ptr++ = ' '; *line1Ptr++ = ' '; *line1Ptr++ = ' '; *line1Ptr++ = ' ';
                *line2Ptr++ = ' '; *line2Ptr++ = ' '; *line2Ptr++ = ' '; *line2Ptr++ = ' ';
            }
            mask <<= 1;
        }
        wmove(window_ptr, 2 + 2 * j, 3);
        count = line1Ptr - line1; line1Ptr = line1;
        while ( count-- ) waddch(window_ptr, *line1Ptr++);
        wmove(window_ptr, 3 + 2 * j, 3);
        count = line2Ptr - line2; line2Ptr = line2;
        while ( count-- ) waddch(window_ptr, *line2Ptr++);
        
        j++;
    }
#define GAME_ENGINE ((TGameEngine*)context)
}

//

void
helpDraw(
    tui_window_ref  the_window,
    WINDOW          *window_ptr,
    const void      *context
)
{
    wclear(window_ptr);
    
    mvwprintw(window_ptr, 2, 2, "P,p          pause");
    mvwprintw(window_ptr, 3, 2, "Q,q           quit");
    wattron(window_ptr, A_UNDERLINE);
        mvwprintw(window_ptr, 5, 1, "             rotate ");
    wattroff(window_ptr, A_UNDERLINE);
    mvwprintw(window_ptr, 6, 2, "A,a     -clockwise");
    mvwprintw(window_ptr, 7, 2, "S,s     +clockwise");
    wattron(window_ptr, A_UNDERLINE);
        mvwprintw(window_ptr, 9, 1, "              shift ");
    wattroff(window_ptr, A_UNDERLINE);
    
    if ( doesSupportUTF8() ) {
        mvwprintw(window_ptr, 10, 2, "← →    left, right");
        mvwprintw(window_ptr, 11, 2, " ↓       soft drop");
    } else {
        mvwaddch(window_ptr, 10, 2, ACS_LARROW);
        mvwaddch(window_ptr, 10, 4, ACS_RARROW);
        mvwprintw(window_ptr, 10, 5, "    left, right");
        mvwaddch(window_ptr, 11, 3, ACS_DARROW);
        mvwprintw(window_ptr, 11, 4, "       soft drop");
    }
    
    mvwprintw(window_ptr, 12, 2, "D,d     hard drop");
}

//

enum {
    TWindowIndexGameBoard = 0,
    TWindowIndexStats,
    TWindowIndexScoreboard,
    TWindowIndexNextTetromino,
    TWindowIndexHelp,
    TWindowIndexMax
};

int
main(
    int                 argc,
    char * const        argv[]
)
{
    WINDOW              *mainWindow = NULL;
    int                 keyCh, screenHeight, screenWidth;
    int                 wantGameBoardWidth = 10, wantGameBoardHeight = 20;
    unsigned int        idx;
    
    TGameEngine         *gameEngine = NULL;
    tui_window_ref      gameWindows[TWindowIndexMax];
    tui_window_rect_t   gameWindowsBounds[TWindowIndexMax];
    unsigned int        gameWindowsUpdate = 0, gameWindowsEnabled = 0;
    bool                areStatsDisplayed = true, isGameTitleDisplayed = true;
    bool                noAlternateWidth = false;
    
    setlocale(LC_ALL, "");
    
    // Parse CLI arguments:
    while ( (keyCh = getopt_long(argc, argv, cliArgOptsStr, cliArgOpts, NULL)) != -1 ) {
        switch ( keyCh ) {
            case 'h':
                usage(argv[0]);
                exit(0);
            
            case 'w':
                if ( ! parseWidth(optarg, &wantGameBoardWidth) ) exit(EINVAL);
                break;
            
            case 'H':
                if ( ! parseHeight(optarg, &wantGameBoardHeight) ) exit(EINVAL);
                break;
        }
    }
    
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
    
    // Determine screen size:
    getmaxyx(mainWindow, screenHeight, screenWidth);
    
    if ( screenWidth < 77 ) {
        delwin(mainWindow);
        endwin();
        refresh();
        printf("Please resize your terminal to at least 77 columns wide and 40 columns high.\n\n");
        exit(1);
    }
    if ( screenWidth < 97 ) {
        areStatsDisplayed = false;
        isGameTitleDisplayed = false;
    }
    if ( screenHeight < 44 ) {
        delwin(mainWindow);
        endwin();
        refresh();
        printf("Please resize your terminal to at least 40 columns high.\n\n");
        exit(1);
    }
    if ( screenHeight < 50 ) {
        isGameTitleDisplayed = false;
    }
    
    // Given what's going to be displayed, figure out how much space is left:
    int     dW = screenWidth - (1 + 22 + 1) - ((areStatsDisplayed) ? 1 + 20 + 1 : 0) - 2;
    int     dH = screenHeight - (isGameTitleDisplayed ? 6 : 0) - 4;
    int     gameBoardWidth = dW / 4, gameBoardLeadMargin = 0;
    int     gameBoardHeight = dH / 2;
    
    // Check to ensure the desired dimensions work:
    if ( wantGameBoardWidth <= gameBoardWidth ) {
        if ( wantGameBoardWidth > 0 ) gameBoardWidth = wantGameBoardWidth;
    } else {
        delwin(mainWindow);
        endwin();
        refresh();
        printf("Your terminal window is not wide enough to display the game board.\n\n");
        exit(1);
    }
    if ( wantGameBoardHeight <= gameBoardHeight ) {
        if ( wantGameBoardHeight > 0 ) gameBoardHeight = wantGameBoardHeight;
    } else {
        delwin(mainWindow);
        endwin();
        refresh();
        printf("Your terminal window lacks enough rows to display the game board.\n\n");
        exit(1);
    }
    
    if ( noAlternateWidth ) {
        gameBoardLeadMargin = 4 * ((gameBoardWidth - 10) / 2);
        gameBoardWidth = 10;
    }
    
    // Setup window bounds rects:
    if ( areStatsDisplayed ) {
        gameWindowsBounds[TWindowIndexStats] = tui_window_rect_make(
                                                        1,
                                                        isGameTitleDisplayed ? 7 : 1,
                                                        20,
                                                        32
                                                    );
    }
    gameWindowsBounds[TWindowIndexGameBoard] = tui_window_rect_make(
                                                    1 + (areStatsDisplayed ? (1 + 20) : 0) + gameBoardLeadMargin,
                                                    isGameTitleDisplayed ? 7 : 1,
                                                    gameBoardWidth * 4 + 4,
                                                    gameBoardHeight * 2 + 2
                                                );
    gameWindowsBounds[TWindowIndexScoreboard] = tui_window_rect_make(
                                                    screenWidth - 22 - 1,
                                                    isGameTitleDisplayed ? 7 : 1,
                                                    22,
                                                    10
                                                );
    gameWindowsBounds[TWindowIndexNextTetromino] = tui_window_rect_make(
                                                    screenWidth - 22 - 1,
                                                    isGameTitleDisplayed ? 18 : 12,
                                                    22,
                                                    12
                                                );
    gameWindowsBounds[TWindowIndexHelp] = tui_window_rect_make(
                                                    screenWidth - 22 - 1,
                                                    isGameTitleDisplayed ? 31 : 25,
                                                    22,
                                                    14
                                                );
    
    
    /*if ( ! has_colors() ) {
        delwin(mainWindow);
        endwin();
        refresh();
        printf("No color.\n\n");
        exit(1);
    }
    
    start_color();
    init_pair(1, COLOR_WHITE, COLOR_RED);
    init_pair(2, COLOR_WHITE, COLOR_GREEN);
    init_pair(3, COLOR_WHITE, COLOR_BLUE);
    init_pair(4, COLOR_WHITE, COLOR_YELLOW);*/
    
    // Create the game engine:
    gameEngine = TGameEngineCreate(1, gameBoardWidth, gameBoardHeight);
    
    //
    // Initialize game windows:
    //
    memset(gameWindows, 0, sizeof(gameWindows));
    gameWindows[TWindowIndexGameBoard] = tui_window_alloc(
                                                gameWindowsBounds[TWindowIndexGameBoard], 0,
                                                NULL, 0,
                                                gameBoardDraw, (const void*)gameEngine);
    if ( ! gameWindows[TWindowIndexGameBoard] ) {
        delwin(mainWindow);
        endwin();
        refresh();
        fprintf(stderr, "ERROR:  unable to create game board window\n");
        exit(1);
    }
    gameWindowsEnabled = 1 << TWindowIndexGameBoard;
    
    gameWindows[TWindowIndexScoreboard] = tui_window_alloc(
                                                gameWindowsBounds[TWindowIndexScoreboard], tui_window_opts_title_align_right,
                                                "SCORE", 0,
                                                scoreboardDraw, (const void*)gameEngine);
    if ( ! gameWindows[TWindowIndexScoreboard] ) {
        tui_window_free(gameWindows[TWindowIndexGameBoard]);
        delwin(mainWindow);
        endwin();
        refresh();
        fprintf(stderr, "ERROR:  unable to create game scoreboard window\n");
        exit(1);
    }
    gameWindowsEnabled |= 1 << TWindowIndexScoreboard;
    
    gameWindows[TWindowIndexNextTetromino] = tui_window_alloc(
                                                gameWindowsBounds[TWindowIndexNextTetromino], tui_window_opts_title_align_right,
                                                "NEXT UP", 0,
                                                nextTetrominoDraw, (const void*)gameEngine);
    if ( ! gameWindows[TWindowIndexNextTetromino] ) {
        tui_window_free(gameWindows[TWindowIndexScoreboard]);
        tui_window_free(gameWindows[TWindowIndexGameBoard]);
        delwin(mainWindow);
        endwin();
        refresh();
        fprintf(stderr, "ERROR:  unable to create next tetromino window\n");
        exit(1);
    }
    gameWindowsEnabled |= 1 << TWindowIndexNextTetromino;
    
    gameWindows[TWindowIndexHelp] = tui_window_alloc(
                                                gameWindowsBounds[TWindowIndexHelp], tui_window_opts_title_align_right,
                                                "HELP", 0,
                                                helpDraw, NULL);
    if ( ! gameWindows[TWindowIndexHelp] ) {
        tui_window_free(gameWindows[TWindowIndexNextTetromino]);
        tui_window_free(gameWindows[TWindowIndexScoreboard]);
        tui_window_free(gameWindows[TWindowIndexGameBoard]);
        delwin(mainWindow);
        endwin();
        refresh();
        fprintf(stderr, "ERROR:  unable to create help window\n");
        exit(1);
    }
    gameWindowsEnabled |= 1 << TWindowIndexHelp;
    
    if ( areStatsDisplayed ) {
        gameWindows[TWindowIndexStats] = tui_window_alloc(
                                                    gameWindowsBounds[TWindowIndexStats], 0,
                                                    "STATS", 0,
                                                    statsDraw, (const void*)gameEngine);
        if ( ! gameWindows[TWindowIndexStats] ) {
        tui_window_free(gameWindows[TWindowIndexHelp]);
        tui_window_free(gameWindows[TWindowIndexNextTetromino]);
        tui_window_free(gameWindows[TWindowIndexScoreboard]);
        tui_window_free(gameWindows[TWindowIndexGameBoard]);
            delwin(mainWindow);
            endwin();
            refresh();
            fprintf(stderr, "ERROR:  unable to create game stats window\n");
            exit(1);
        }
        gameWindowsEnabled |= 1 << TWindowIndexStats;
    }

    // Initial draw of all windows:
    refresh();
    if ( isGameTitleDisplayed ) drawGameTitle(mainWindow, screenWidth);
    for ( idx = 0; idx < TWindowIndexMax; idx++ ) if (gameWindowsEnabled & (1 << idx)) tui_window_refresh(gameWindows[idx], 1);
    doupdate();
    
    struct timespec     t0, t1, dt, nextDrop;
    unsigned long       frameCount = 0;
    unsigned int        extraPoints = 0;
    bool                isSoftDrop = false;
    
    // Start timer:
    t0.tv_sec = t0.tv_nsec = 0;
    clock_gettime(CLOCK_REALTIME, &t1);
    
    timespec_add(&nextDrop, &t1, &gameEngine->tPerLine);
    
    timeout(0);
    
    while ( 1 ) {
        keyCh = getch();
        if ( (keyCh == 'Q') || (keyCh == 'q') ) {
            break;
        }
        if ( gameEngine->isPaused ) {
            if ( (keyCh == 'P') || (keyCh == 'p') ) gameEngine->isPaused = false;
        } else {
            t0 = t1;
            clock_gettime(CLOCK_REALTIME, &t1);
        
            if ( timespec_is_ordered_asc(&nextDrop, &t1) ) {
                // Drop the piece:
                TGridPos    newP = gameEngine->currentSprite.P;
                uint16_t    board4x4, piece4x4 = TSpriteGet4x4(&gameEngine->currentSprite);
            
                newP.j++;
                board4x4 = TBitGridExtract4x4AtPosition(gameEngine->gameBoard, 0, newP);
                if ( (board4x4 & piece4x4) == 0 ) {
                    gameEngine->currentSprite.P.j++;
                    gameWindowsUpdate |= (1 << TWindowIndexGameBoard);
                    extraPoints = 0;
                    isSoftDrop = false;
                } else {
                    TBitGridSet4x4AtPosition(gameEngine->gameBoard, 0, gameEngine->currentSprite.P, TSpriteGet4x4(&gameEngine->currentSprite));
                    TGameEngineCheckForCompleteRows(gameEngine);
                    gameEngine->scoreboard.score += extraPoints;
                    extraPoints = 0;
                    isSoftDrop = false;
                    TGameEngineChooseNextPiece(gameEngine);
                    gameWindowsUpdate |= (1 << TWindowIndexGameBoard) | (1 << TWindowIndexStats) | (1 << TWindowIndexScoreboard) | (1 << TWindowIndexNextTetromino);
                }
                timespec_add(&nextDrop, &t1, &gameEngine->tPerLine);
            }
        
            switch ( keyCh ) {
                case 'p':
                case 'P':
                    gameEngine->isPaused = true;
                    gameWindowsUpdate |= (1 << TWindowIndexGameBoard);
                    break;
                case ' ':
                case 's':
                case 'S': {
                    // Test for ok:
                    TSprite         newOrientation = TSpriteMakeRotated(&gameEngine->currentSprite);
                    uint16_t        board4x4 = TBitGridExtract4x4AtPosition(gameEngine->gameBoard, 0, newOrientation.P);
                    uint16_t        piece4x4 = TSpriteGet4x4(&newOrientation);
                
                    if ( (board4x4 & piece4x4) == 0 ) {
                        gameEngine->currentSprite = newOrientation;
                        gameWindowsUpdate |= (1 << TWindowIndexGameBoard);
                    }
                    break;
                }
                case 'a':
                case 'A': {
                    // Test for ok:
                    TSprite         newOrientation = TSpriteMakeRotatedAnti(&gameEngine->currentSprite);
                    uint16_t        board4x4 = TBitGridExtract4x4AtPosition(gameEngine->gameBoard, 0, newOrientation.P);
                    uint16_t        piece4x4 = TSpriteGet4x4(&newOrientation);
                
                    if ( (board4x4 & piece4x4) == 0 ) {
                        gameEngine->currentSprite = newOrientation;
                        gameWindowsUpdate |= (1 << TWindowIndexGameBoard);
                    }
                    break;
                }
                /*case KEY_UP: {
                    // Test for ok:
                    TGridPos    newP = gameEngine->currentSprite.P;
                    uint16_t    board4x4, piece4x4 = TSpriteGet4x4(&gameEngine->currentSprite);
                
                    newP.j--;
                    board4x4 = TBitGridExtract4x4AtPosition(gameEngine->gameBoard, 0, newP);
                    if ( (board4x4 & piece4x4) == 0 ) {
                        gameEngine->currentSprite.P.j--;
                        gameWindowsUpdate |= (1 << TWindowIndexGameBoard);
                    }
                    break;
                }*/
                case KEY_DOWN: {
                    // Test for ok:
                    TGridPos    newP = gameEngine->currentSprite.P;
                    uint16_t    board4x4, piece4x4 = TSpriteGet4x4(&gameEngine->currentSprite);
                
                    newP.j++;
                    board4x4 = TBitGridExtract4x4AtPosition(gameEngine->gameBoard, 0, newP);
                    if ( (board4x4 & piece4x4) == 0 ) {
                        gameEngine->currentSprite.P.j++;
                        gameWindowsUpdate |= (1 << TWindowIndexGameBoard);
                        isSoftDrop = true;
                        extraPoints++;
                    } else {
                        TBitGridSet4x4AtPosition(gameEngine->gameBoard, 0, gameEngine->currentSprite.P, TSpriteGet4x4(&gameEngine->currentSprite));
                        TGameEngineCheckForCompleteRows(gameEngine);
                        gameEngine->scoreboard.score += extraPoints;
                        extraPoints = 0;
                        isSoftDrop = false;
                        TGameEngineChooseNextPiece(gameEngine);
                        gameWindowsUpdate |= (1 << TWindowIndexGameBoard) | (1 << TWindowIndexStats) | (1 << TWindowIndexScoreboard) | (1 << TWindowIndexNextTetromino);
                    }
                    timespec_add(&nextDrop, &t1, &gameEngine->tPerLine);
                    break;
                }
                case 'D':
                case 'd': {
                    extraPoints = 0;
                    isSoftDrop = false;
                    
                    while ( 1 ) {
                        // Test for ok:
                        TGridPos    newP = gameEngine->currentSprite.P;
                        uint16_t    board4x4, piece4x4 = TSpriteGet4x4(&gameEngine->currentSprite);
                
                        newP.j++;
                        board4x4 = TBitGridExtract4x4AtPosition(gameEngine->gameBoard, 0, newP);
                        if ( (board4x4 & piece4x4) == 0 ) {
                            gameEngine->currentSprite.P.j++;
                            gameWindowsUpdate |= (1 << TWindowIndexGameBoard);
                            extraPoints += 2;
                        } else {
                            break;
                        }
                    }
                    TBitGridSet4x4AtPosition(gameEngine->gameBoard, 0, gameEngine->currentSprite.P, TSpriteGet4x4(&gameEngine->currentSprite));
                    TGameEngineCheckForCompleteRows(gameEngine);
                    gameEngine->scoreboard.score += extraPoints;
                    TGameEngineChooseNextPiece(gameEngine);
                    gameWindowsUpdate |= (1 << TWindowIndexGameBoard) | (1 << TWindowIndexStats) | (1 << TWindowIndexScoreboard) | (1 << TWindowIndexNextTetromino);
                    break;
                }
                case KEY_LEFT: {
                    // Test for ok:
                    TGridPos    newP = gameEngine->currentSprite.P;
                    uint16_t    board4x4, piece4x4 = TSpriteGet4x4(&gameEngine->currentSprite);
                
                    newP.i--;
                    board4x4 = TBitGridExtract4x4AtPosition(gameEngine->gameBoard, 0, newP);
                    if ( (board4x4 & piece4x4) == 0 ) {
                        gameEngine->currentSprite.P.i--;
                        gameWindowsUpdate |= (1 << TWindowIndexGameBoard);
                    }
                    break;
                }
                case KEY_RIGHT: {
                    // Test for ok:
                    TGridPos    newP = gameEngine->currentSprite.P;
                    uint16_t    board4x4, piece4x4 = TSpriteGet4x4(&gameEngine->currentSprite);
                
                    newP.i++;
                    board4x4 = TBitGridExtract4x4AtPosition(gameEngine->gameBoard, 0, newP);
                    if ( (board4x4 & piece4x4) == 0 ) {
                        gameEngine->currentSprite.P.i++;
                        gameWindowsUpdate |= (1 << TWindowIndexGameBoard);
                    }
                    break;
                }
            
            }
        
            // Only check those windows we're displaying:
            gameWindowsUpdate &= gameWindowsEnabled;
            if ( gameWindowsUpdate ) {
                refresh();
                idx = 0;
                while ( gameWindowsUpdate && (idx < TWindowIndexMax) ) {
                    if ( gameWindowsUpdate & 0x1 ) tui_window_refresh(gameWindows[idx], 1);
                    gameWindowsUpdate >>= 1;
                    idx++;
                }
                doupdate();
            }
        
            timespec_add(&gameEngine->tElapsed, &gameEngine->tElapsed, timespec_subtract(&dt, &t1, &t0));
            frameCount++;
        
        
            double      dt_sec = timespec_to_double(&gameEngine->tElapsed);
        
            mvprintw(screenHeight - 1, 0, "%12.3lg frames/sec", (double)frameCount / dt_sec);
        }
    }
    
    // Dispose of all windows:
    for ( idx = 0; idx < TWindowIndexMax; idx++ ) if (gameWindowsEnabled & (1 << idx)) tui_window_free(gameWindows[idx]);
    delwin(mainWindow);
    endwin();
    refresh();
    
    double      dt_sec = timespec_to_double(&gameEngine->tElapsed);
    
    printf("%lg seconds, %lu frames = %lg frames/sec\n", dt_sec, frameCount, (double)frameCount / dt_sec);
    
    return 0;
}
