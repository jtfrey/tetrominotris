
#include "TTetrominos.h"
#include "TBitGrid.h"
#include "TSprite.h"
#include "TScoreboard.h"
#include "TGameEngine.h"
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
    mvwprintw(window_ptr, 2, 1, "%-20s", "Lines");
    wattroff(window_ptr, A_UNDERLINE);
    mvwprintw(window_ptr, 3, 2, "   Total%8u", SCOREBOARD.nLinesTotal);
    mvwprintw(window_ptr, 4, 2, "  Single%8u", SCOREBOARD.nLinesOfType[TScoreboardLineCountTypeSingle]);
    mvwprintw(window_ptr, 5, 2, "  Double%8u", SCOREBOARD.nLinesOfType[TScoreboardLineCountTypeDouble]);
    mvwprintw(window_ptr, 6, 2, "  Triple%8u", SCOREBOARD.nLinesOfType[TScoreboardLineCountTypeTriple]);
    mvwprintw(window_ptr, 7, 2, "    Quad%8u", SCOREBOARD.nLinesOfType[TScoreboardLineCountTypeQuadruple]);
    
    wattron(window_ptr, A_UNDERLINE);
    mvwprintw(window_ptr, 9, 1, "%-20s", "Tetrominos");
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
        mvwprintw(window_ptr, 2, 2, "%-18s", "Points");
    wattroff(window_ptr, A_UNDERLINE);
    mvwprintw(window_ptr, 4, 2, "%18u", SCOREBOARD.score);
    
    wattron(window_ptr, A_UNDERLINE);
        mvwprintw(window_ptr, 6, 2, "%-18s", "Level");
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

int
main()
{
    WINDOW              *mainWindow = NULL;
    tui_window_ref      gameBoardWindow = NULL, statsWindow = NULL, scoreWindow = NULL,
                        nextTetrominoWindow = NULL;
    int                 keyCh, screenHeight, screenWidth;
    TGameEngine         *gameEngine = NULL;
    
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
    
    // Determine screen size:
    getmaxyx(mainWindow, screenHeight, screenWidth);
    if ( screenHeight < 40 || screenWidth < 100 ) {
        delwin(mainWindow);
        endwin();
        refresh();
        printf("Please resize your terminal to at least 100 x 40.\n\n");
        exit(1);
    }
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
    gameEngine = TGameEngineCreate(1, 10, 20);
    
    // Create our windows:
    gameBoardWindow = tui_window_alloc(26, 7, 44, 42, 0, NULL, 0, gameBoardDraw, (const void*)gameEngine);
    statsWindow = tui_window_alloc(4, 7, 20, 32, 0, "STATS", 0, statsDraw, (const void*)gameEngine);
    scoreWindow = tui_window_alloc(72, 7, 22, 10, tui_window_opts_title_align_right, "SCORE", 0, scoreboardDraw, (const void*)gameEngine);
    nextTetrominoWindow = tui_window_alloc(72, 18, 22, 12, tui_window_opts_title_align_right, "NEXT UP", 0, nextTetrominoDraw, (const void*)gameEngine);

    if ( gameBoardWindow && statsWindow) {
        bool        triggerStatsNextMove = false;
        
        refresh();
        drawGameTitle(mainWindow);
        tui_window_refresh(statsWindow, 1);
        tui_window_refresh(scoreWindow, 1);
        tui_window_refresh(nextTetrominoWindow, 1);
        tui_window_refresh(gameBoardWindow, 1);
        doupdate();
    
        while ( (keyCh = getch()) != 'q' ) {
            switch ( keyCh ) {
                case 'c':
                case 'C':
                    TGameEngineCheckForCompleteRows(gameEngine);
                    break;
                case '1':
                case '2':
                case '3':
                case '4':
                case '5':
                case '6':
                case '7':
                    gameEngine->scoreboard.tetrominosOfType[keyCh - '1']++;
                    break;
                case 'p':
                    TScoreboardAddLinesOfType(&gameEngine->scoreboard, 1 + random() % 4);
                    break;
                case '\r':
                case '\n': {
                    // Test for ok:
                    TGridPos    newP = gameEngine->currentSprite.P;
                    uint16_t    board4x4, piece4x4 = TSpriteGet4x4(&gameEngine->currentSprite);
                    
                    newP.j++;
                    board4x4 = TBitGridExtract4x4AtPosition(gameEngine->gameBoard, 0, newP);
                    if ( (board4x4 & piece4x4) != 0 ) {
                        TBitGridSet4x4AtPosition(gameEngine->gameBoard, 0, gameEngine->currentSprite.P, TSpriteGet4x4(&gameEngine->currentSprite));
                        TGameEngineCheckForCompleteRows(gameEngine);
                        TGameEngineChooseNextPiece(gameEngine);
                    }
                    break;
                }
                case 'T':
                case 't':
                    triggerStatsNextMove = true;
                    break;
                case ' ': {
                    // Test for ok:
                    TSprite         newOrientation = TSpriteMakeRotated(&gameEngine->currentSprite);
                    uint16_t        board4x4 = TBitGridExtract4x4AtPosition(gameEngine->gameBoard, 0, newOrientation.P);
                    uint16_t        piece4x4 = TSpriteGet4x4(&newOrientation);
                    
                    if ( (board4x4 & piece4x4) == 0 ) {
                        gameEngine->currentSprite = newOrientation;
                    }
                    if ( triggerStatsNextMove ) {
                        FILE *fptr = fopen("next-move.txt", "w");
                        fprintf(fptr, "ROTATE   %04hX   %04hX   %04hX\n", piece4x4, board4x4, piece4x4 & board4x4);
                        fclose(fptr);
                        triggerStatsNextMove = false;
                    }
                    break;
                }
                case KEY_UP: {
                    // Test for ok:
                    TGridPos    newP = gameEngine->currentSprite.P;
                    uint16_t    board4x4, piece4x4 = TSpriteGet4x4(&gameEngine->currentSprite);
                    
                    newP.j--;
                    board4x4 = TBitGridExtract4x4AtPosition(gameEngine->gameBoard, 0, newP);
                    if ( (board4x4 & piece4x4) == 0 ) {
                        gameEngine->currentSprite.P.j--;
                    }
                    if ( triggerStatsNextMove ) {
                        FILE *fptr = fopen("next-move.txt", "w");
                        fprintf(fptr, "UP   %04hX   %04hX   %04hX\n", piece4x4, board4x4, piece4x4 & board4x4);
                        fclose(fptr);
                        triggerStatsNextMove = false;
                    }
                    break;
                }
                case KEY_DOWN: {
                    // Test for ok:
                    TGridPos    newP = gameEngine->currentSprite.P;
                    uint16_t    board4x4, piece4x4 = TSpriteGet4x4(&gameEngine->currentSprite);
                    
                    newP.j++;
                    board4x4 = TBitGridExtract4x4AtPosition(gameEngine->gameBoard, 0, newP);
                    if ( (board4x4 & piece4x4) == 0 ) {
                        gameEngine->currentSprite.P.j++;
                    }
                    if ( triggerStatsNextMove ) {
                        FILE *fptr = fopen("next-move.txt", "w");
                        fprintf(fptr, "DOWN   %04hX   %04hX   %04hX\n", piece4x4, board4x4, piece4x4 & board4x4);
                        fclose(fptr);
                        triggerStatsNextMove = false;
                    }
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
                    }
                    if ( triggerStatsNextMove ) {
                        FILE *fptr = fopen("next-move.txt", "w");
                        fprintf(fptr, "LEFT   %04hX   %04hX   %04hX\n", piece4x4, board4x4, piece4x4 & board4x4);
                        fclose(fptr);
                        triggerStatsNextMove = false;
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
                    }
                    if ( triggerStatsNextMove ) {
                        FILE *fptr = fopen("next-move.txt", "w");
                        fprintf(fptr, "RIGHT   %04hX   %04hX   %04hX\n", piece4x4, board4x4, piece4x4 & board4x4);
                        fclose(fptr);
                        triggerStatsNextMove = false;
                    }
                    break;
                }
                
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
