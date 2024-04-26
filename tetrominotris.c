
#include "TTetrominos.h"
#include "TBitGrid.h"
#include "TSprite.h"
#include "TScoreboard.h"
#include "TGameEngine.h"
#include "TKeymap.h"
#include "tui_window.h"

#include <locale.h>
#include <langinfo.h>

//

#include <getopt.h>

static struct option cliArgOpts[] = {
    { "help",       no_argument,        NULL,       'h' },
    { "width",      required_argument,  NULL,       'w' },
    { "height",     required_argument,  NULL,       'H' },
    { "level",      required_argument,  NULL,       'l' },
    { "keymap",     required_argument,  NULL,       'k' },
    { "utf8",       no_argument,        NULL,       'U' },
    { NULL,         0,                  NULL,        0  }
};
static const char *cliArgOptsStr = "hw:H:l:k:U";

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
        "    --level/-l #                   start the game at this level (0 and\n"
        "                                   up)\n"
        "    --keymap/-k <filepath>         initialize the key mapping from the\n"
        "                                   given file\n"
        "    --utf8/-U                      allow UTF-8 characters to be displayed\n"
        "\n"
        "    <dimension> = # | default | fit\n"
        "              # = a positive integer value\n"
        "        default = 10 wide or 20 high\n"
        "            fit = adjust the width to fit the terminal\n"
        "\n"
        "version: " TETROMINOTRIS_VERSION "\n"
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
        fprintf(stdout, "ERROR:  %s is lower than the minimum game board width of 10\n", optstr);
    } else {
        fprintf(stdout, "ERROR:  invalid game board width provided: %s\n", optstr);
    }
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
        if ( v >= 18 ) {
            *height = v;
            return true;
        }
        fprintf(stdout, "ERROR:  %s is lower than the minimum game board height of 18\n", optstr);
    } else {
        fprintf(stdout, "ERROR:  invalid game board height provided: %s\n", optstr);
    }
    return false;
}

//

static bool gAllowUTF8 = false;

static bool
doesSupportUTF8(void)
{
    static bool hasBeenChecked = false;
    static bool __doesSupportUTF8;
    
    if ( ! hasBeenChecked ) {
        if ( gAllowUTF8 ) {
            const char  *encoding = nl_langinfo(CODESET);
            __doesSupportUTF8 = ( ! strcasecmp(encoding, "utf8") || ! strcasecmp(encoding, "utf-8") );
        } else {
            __doesSupportUTF8 = false;
        }
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

static const char* gameOverStrings[5] = {
                        "                     ",
                        "  G A M E   O V E R  ",
                        "                     ",
                        " press R/r  to reset ",
                        "                     "
                    };
static unsigned int gameOverStringsLen = 21;

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
            if ( GAME_ENGINE->hasEnded ) {
                *line1Ptr++ = A_REVERSE | ' '; *line1Ptr++ = A_REVERSE | ' '; *line1Ptr++ = A_REVERSE | ' '; *line1Ptr++ = A_REVERSE | ' ';
                *line2Ptr++ = A_REVERSE | '_'; *line2Ptr++ = A_REVERSE | '_'; *line2Ptr++ = A_REVERSE | '_'; *line2Ptr++ = A_REVERSE | '_';
            }
            else if ( spriteBit || (gridBit && cellValue) ) {
                *line1Ptr++ = A_REVERSE | '|'; *line1Ptr++ = A_REVERSE | ' '; *line1Ptr++ = A_REVERSE | ' '; *line1Ptr++ = A_REVERSE | ' ';
                *line2Ptr++ = A_REVERSE | '|'; *line2Ptr++ = A_REVERSE | '_'; *line2Ptr++ = A_REVERSE | '_'; *line2Ptr++ = A_REVERSE | '_';
            }
            else {
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
    
    if ( GAME_ENGINE->hasEnded ) {
        int     x = 2 + (4 * THE_BOARD->dimensions.w - gameOverStringsLen) / 2;
        int     y = THE_BOARD->dimensions.h - 4;
        
        mvwprintw(window_ptr, y++, x, "%s", gameOverStrings[0]);
        wattron(window_ptr, A_BLINK);
        mvwprintw(window_ptr, y++, x, "%s", gameOverStrings[1]);
        wattroff(window_ptr, A_BLINK);
        mvwprintw(window_ptr, y++, x, "%s", gameOverStrings[2]);
        mvwprintw(window_ptr, y++, x, "%s", gameOverStrings[3]);
        mvwprintw(window_ptr, y, x, "%s", gameOverStrings[4]);
    }
    else if ( GAME_ENGINE->isPaused ) {
        int     x = 2 + (4 * THE_BOARD->dimensions.w - gamePausedStringsLen) / 2;
        int     y = THE_BOARD->dimensions.h - 4;
        
        mvwprintw(window_ptr, y++, x, "%s", gamePausedStrings[0]);
        wattron(window_ptr, A_BLINK);
        mvwprintw(window_ptr, y++, x, "%s", gamePausedStrings[1]);
        wattroff(window_ptr, A_BLINK);
        mvwprintw(window_ptr, y, x, "%s", gamePausedStrings[2]);
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
#define THE_KEYMAP  ((TKeymap*)context)

    wclear(window_ptr);
    
    mvwprintw(window_ptr, 2, 2, "%-13.13spause", TKeymapKeySummaryForEvent(THE_KEYMAP, TGameEngineEventTogglePause));
    mvwprintw(window_ptr, 3, 2, "Q,q           quit");
    mvwprintw(window_ptr, 4, 2, "%-13.13sreset", TKeymapKeySummaryForEvent(THE_KEYMAP, TGameEngineEventReset));
    wattron(window_ptr, A_UNDERLINE);
        mvwprintw(window_ptr, 6, 1, "             rotate ");
    wattroff(window_ptr, A_UNDERLINE);
    mvwprintw(window_ptr, 7, 2, "%-8.8s-clockwise", TKeymapKeySummaryForEvent(THE_KEYMAP, TGameEngineEventRotateAntiClockwise));
    mvwprintw(window_ptr, 8, 2, "%-8.8s+clockwise", TKeymapKeySummaryForEvent(THE_KEYMAP, TGameEngineEventRotateClockwise));
    wattron(window_ptr, A_UNDERLINE);
        mvwprintw(window_ptr, 10, 1, "              shift ");
    wattroff(window_ptr, A_UNDERLINE);
    
    if ( doesSupportUTF8() ) {
        mvwprintw(window_ptr, 11, 2, "← →    left, right");
        mvwprintw(window_ptr, 12, 2, " ↓       soft drop");
    } else {
        mvwaddch(window_ptr, 11, 2, ACS_LARROW);
        mvwaddch(window_ptr, 11 , 4, ACS_RARROW);
        mvwprintw(window_ptr, 11, 5, "    left, right");
        mvwaddch(window_ptr, 12, 3, ACS_DARROW);
        mvwprintw(window_ptr, 12, 4, "       soft drop");
    }
    
    mvwprintw(window_ptr, 13, 2, "%-9.9shard drop", TKeymapKeySummaryForEvent(THE_KEYMAP, TGameEngineEventHardDrop));
    
#undef THE_KEYMAP
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
    unsigned int        idx;
    
    TGameEngine         *gameEngine = NULL;
    TKeymap             gameKeymap;
    
    WINDOW              *mainWindow = NULL;
    
    bool                areStatsDisplayed = true, isGameTitleDisplayed = true;
    tui_window_ref      gameWindows[TWindowIndexMax];
    tui_window_rect_t   gameWindowsBounds[TWindowIndexMax];
    int                 keyCh, screenHeight, screenWidth, availScreenWidth, availScreenHeight,
                        gameBoardLeadMargin, gameBoardWidth, gameBoardHeight;
    int                 wantGameBoardWidth = 10, wantGameBoardHeight = 20;
    unsigned int        gameWindowsEnabled = 0;
    bool                haveRetriedWidth = false, haveRetriedHeight = false, doDimensionRetry = false;
    
    unsigned int        startingLevel = 0;
    
    setlocale(LC_ALL, "");
    
    TKeymapInit(&gameKeymap);
    
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
            
            case 'l': {
                char    *endptr = NULL;
                long    v = strtol(optarg, &endptr, 0);
                
                if ( endptr > optarg ) {
                    if ( v < 0 || v > 9 ) {
                        fprintf(stderr, "ERROR:  level number must be between 0 and 9: %ld\n", v);
                        exit(EINVAL);
                    }
                    startingLevel = v;
                } else {
                    fprintf(stderr, "ERROR:  invalid level number: %s\n", optarg);
                    exit(EINVAL);
                }
                break;
            }
                
            case 'k':
                TKeymapInitWithFile(&gameKeymap, optarg);
                break;
            
            case 'U':
                gAllowUTF8 = true;
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

retry_board_dims:

    // Given what's going to be displayed, figure out how much space is left:
    availScreenWidth = screenWidth - (1 + 22 + 1) - ((areStatsDisplayed) ? 1 + 20 + 1 : 0);
    availScreenHeight = screenHeight - (isGameTitleDisplayed ? 6 : 0);
    gameBoardWidth = availScreenWidth / 4 - 2;
    gameBoardLeadMargin = 0;
    gameBoardHeight = availScreenHeight / 2 - 2;
    
    // Check to ensure the desired dimensions work:
    if ( wantGameBoardWidth <= gameBoardWidth ) {
        if ( wantGameBoardWidth > 0 ) gameBoardWidth = wantGameBoardWidth;
    } else {
        if ( ! haveRetriedWidth && areStatsDisplayed ) {
            areStatsDisplayed = false;
            haveRetriedWidth = true;
            doDimensionRetry = true;
        } else {
            delwin(mainWindow);
            endwin();
            refresh();
            printf("Your terminal window is not wide enough to display the game board.\n\n");
            exit(1);
        }
    }
    if ( wantGameBoardHeight <= gameBoardHeight ) {
        if ( wantGameBoardHeight > 0 ) gameBoardHeight = wantGameBoardHeight;
    } else {
        if ( ! haveRetriedHeight && isGameTitleDisplayed ) {
            isGameTitleDisplayed = false;
            haveRetriedHeight = true;
            doDimensionRetry = true;
        } else {
            delwin(mainWindow);
            endwin();
            refresh();
            printf("Your terminal window lacks enough rows to display the game board.\n\n");
            exit(1);
        }
    }
    if ( doDimensionRetry ) {
        doDimensionRetry = false;
        goto retry_board_dims;
    }
    gameBoardLeadMargin = (availScreenWidth - (4 * gameBoardWidth + 2)) / 2 - 1;
    
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
                                                    1 + (areStatsDisplayed ? (20 + 1) : 0) + gameBoardLeadMargin,
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
                                                    15
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
    gameEngine = TGameEngineCreate(1, gameBoardWidth, gameBoardHeight, startingLevel);
    
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
    gameWindowsEnabled |= 1 << TWindowIndexGameBoard;
    
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
                                                helpDraw, (const void*)&gameKeymap);
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
    
    timeout(0);
    
    TGameEngineTick(gameEngine, TGameEngineEventStartGame);
    
    while ( 1 ) {
        TGameEngineUpdateNotification   updateNotifications;
        TGameEngineEvent                gameEngineEvent = TGameEngineEventNoOp;
        
        keyCh = getch();
        if ( (keyCh == 'Q') || (keyCh == 'q') ) {
            break;
        }
        switch ( keyCh ) {
            case '\r':
            case '\n':
                gameEngineEvent = TGameEngineEventTogglePause;
                break;
            case KEY_DOWN:
                gameEngineEvent = TGameEngineEventSoftDrop;
                break;
                gameEngineEvent = TGameEngineEventHardDrop;
                break;
            case KEY_LEFT:
                gameEngineEvent = TGameEngineEventMoveLeft;
                break;
            case KEY_RIGHT:
                gameEngineEvent = TGameEngineEventMoveRight;
                break;
            default:
                gameEngineEvent = TKeymapEventForKey(&gameKeymap, keyCh);
                break;
        }
        
        updateNotifications = TGameEngineTick(gameEngine, gameEngineEvent);
        
        if ( updateNotifications ) {
            refresh();
            
            if ( updateNotifications & TGameEngineUpdateNotificationGameBoard ) {
                tui_window_refresh(gameWindows[TWindowIndexGameBoard], 1);
            }
            if ( updateNotifications & TGameEngineUpdateNotificationScoreboard ) {
                tui_window_refresh(gameWindows[TWindowIndexScoreboard], 1);
                if ( gameWindowsEnabled & (1 << TWindowIndexStats) ) {
                    tui_window_refresh(gameWindows[TWindowIndexStats], 1);
                }
            }
            if ( updateNotifications & TGameEngineUpdateNotificationNextTetromino ) {
                tui_window_refresh(gameWindows[TWindowIndexNextTetromino], 1);
            }
            doupdate();
        }
            
        double      dt_sec = timespec_to_double(&gameEngine->tElapsed);
    
        mvprintw(screenHeight - 1, 0, "%12.3lg ticks/sec", (double)gameEngine->tickCount / dt_sec);
    }
    
    double      dt_sec = timespec_to_double(&gameEngine->tElapsed);
    
    // Dispose of all windows:
    for ( idx = 0; idx < TWindowIndexMax; idx++ ) if (gameWindowsEnabled & (1 << idx)) tui_window_free(gameWindows[idx]);
    delwin(mainWindow);
    endwin();
    refresh();
    
    printf("%lg seconds, %lu ticks = %12.3lg ticks/sec\n", dt_sec, gameEngine->tickCount, (double)gameEngine->tickCount / dt_sec);
    
    return 0;
}
