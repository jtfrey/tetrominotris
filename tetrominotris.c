/*	tetrominotris.h
	Copyright (c) 2024, J T Frey
*/

/*!
	Main program
	
	The source file contains the main program logic for the game.  In short, a
	TKeymap is setup, a TGameEngine is created, and a loop that monitors for
	keypresses calls the TGameEngineTick() function repeatedly to advance game
	state.
	
	All tui_window content-drawing callback functions are implemented herein,
	as well.  In some cases there are two variants -- with "_BW" and "_COLOR"
	suffixes to differentiate their mode.
	
	The color features are selectively compiled into the program.  The CMake
	option ENABLE_COLOR_DISPLAY controls their inclusion.
*/

#include "TTetrominos.h"
#include "TBitGrid.h"
#include "TSprite.h"
#include "TScoreboard.h"
#include "TGameEngine.h"
#include "TKeymap.h"
#include "THighScores.h"
#include "tui_window.h"

#include <locale.h>
#include <langinfo.h>

//

#include <getopt.h>

static struct option cliArgOpts[] = {
    { "help",           no_argument,        NULL,       'h' },
    { "width",          required_argument,  NULL,       'w' },
    { "height",         required_argument,  NULL,       'H' },
    { "level",          required_argument,  NULL,       'l' },
    { "keymap",         required_argument,  NULL,       'k' },
#ifdef ENABLE_COLOR_DISPLAY
    { "color",          no_argument,        NULL,       'C' },
    { "basic-colors",   no_argument,        NULL,       'B' },
#endif
    { "utf8",           no_argument,        NULL,       'U' },
    { NULL,             0,                  NULL,        0  }
};

/* Command line options descriptor string; if color is enabled, its additional
 * options are concatenated with the common options -- so don't end the next
 * line with a semicolon!
 */
static const char *cliArgOptsStr = "hw:H:l:k:U"
#ifdef ENABLE_COLOR_DISPLAY
            "CB"
#endif
            ;


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
        "    --height/-H <dimension>        choose the game board height\n"
#ifdef ENABLE_COLOR_DISPLAY
        "    --color/-C                     use a color game board\n"
        "    --basic-colors/-B              use basic curses colors rather\n"
        "                                   than attempting to use custom palettes\n"
#endif
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

/*
 * @function parseWidth
 *
 * Parse a width descriptor from the command line.  Returns
 * true if the string was parsed successfully and *width is
 * set to the desired value.
 *
 * Returns false on error and does not modify *width.
 */
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

/*
 * @function parseHeight
 *
 * Parse a height descriptor from the command line.  Returns
 * true if the string was parsed successfully and *height is
 * set to the desired value.
 *
 * Returns false on error and does not modify *height.
 */
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

/*
 * @var gAllowUTF8
 *
 * Set via a command line option, this determines whether or not the
 * doesSupportUTF8() function will bother checking the language info for
 * UTF-8 encoding.
 */
static bool gAllowUTF8 = false;

/*
 * @function doesSupportUTF8
 *
 * Returns true if the user has elected UTF-8 support and the language environment
 * appears to support it.  Otherwise returns false.
 */
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
////
//

#ifdef ENABLE_COLOR_DISPLAY

/*
 * @typedef TColorPair
 *
 * The two color indices that will form a curses color pair.
 */
typedef struct {
    int     fg, bg;
} TColorPair;

/*
 * @defined TBASICCOLORPALETTECOUNT
 *
 * File-scope arrays must be dimensioned with a compile-time constant,
 * not a variable even it it has const disposition.
 */
#define TBASICCOLORPALETTECOUNT 5

/*
 * @typedef TColorPalette
 *
 * The game associates a palette of three foreground-background pairs
 * with each level.  For basic curses color mode, we use the color
 * constants provided by the library.
 */
typedef TColorPair TColorPalette[3];

/*
 * @constant TBasicColorPaletteCount
 *
 * How many color palettes do we provide in basic curses color mode?
 */
static const int TBasicColorPaletteCount = TBASICCOLORPALETTECOUNT;

/*
 * @constant TBasicColorPalettes
 *
 * The five color palettes we manage to eke out of the basic curses
 * colors.
 */
static const TColorPalette TBasicColorPalettes[TBASICCOLORPALETTECOUNT] = {
                            {
                                { COLOR_WHITE, COLOR_CYAN },
                                { COLOR_BLUE, COLOR_WHITE },
                                { COLOR_CYAN, COLOR_WHITE }
                            },
                            {
                                { COLOR_WHITE, COLOR_GREEN },
                                { COLOR_GREEN, COLOR_YELLOW },
                                { COLOR_YELLOW, COLOR_WHITE }
                            },
                            {
                                { COLOR_WHITE, COLOR_BLUE },
                                { COLOR_BLUE, COLOR_MAGENTA },
                                { COLOR_MAGENTA, COLOR_WHITE }
                            },
                            {
                                { COLOR_WHITE, COLOR_BLUE },
                                { COLOR_BLUE, COLOR_GREEN },
                                { COLOR_GREEN, COLOR_WHITE }
                            },
                            {
                                { COLOR_WHITE, COLOR_RED },
                                { COLOR_RED, COLOR_MAGENTA },
                                { COLOR_MAGENTA, COLOR_WHITE }
                            }  
                        };

/*
 * @function __TBasicColorPalettesSetActivePalette
 *
 * Given the paletteid (modulus the number of palettes available), set color pairs
 * 1 through 4 that the (color) game display windows will utilize.  Color pair
 * 4 is exclusively used for the game title placard.
 */
static inline void
__TBasicColorPalettesSetActivePalette(
    int     paletteId
)
{
    paletteId %= TBasicColorPaletteCount;
    init_pair(1, TBasicColorPalettes[paletteId][0].fg, TBasicColorPalettes[paletteId][0].bg);
    init_pair(2, TBasicColorPalettes[paletteId][1].fg, TBasicColorPalettes[paletteId][1].bg);
    init_pair(3, TBasicColorPalettes[paletteId][2].fg, TBasicColorPalettes[paletteId][2].bg);
    init_pair(4, TBasicColorPalettes[paletteId][2].fg, COLOR_BLACK);
}

//

//
// The following color tables were found at
//    https://kirjava.xyz/tetris-level-colours/
//

/*
 * @constant TCustomColorIndices
 *
 * PLUT (Palette Look-Up Table) for the game levels.  Each level uses three colors, with
 * level 0 starting at index [1,3].  The integer is an index of an RGB value in the
 * TCustomColors table.
 */
static const int TCustomColorIndices[] = {  15, 48, 33, 18, 15, 48, 41, 26, 15, 48, 
                                            36, 20, 15, 48, 42, 18, 15, 48, 43, 21,
                                            15, 48, 34, 43, 15, 48,  0, 22, 15, 48,
                                             5, 19, 15, 48, 22, 18, 15, 48, 39, 22 };
/*
 * @typedef TRGBColor
 *
 * A 3 x 8-bit color in the RGB color space.
 */
typedef struct {
    uint8_t     r, g, b;
} TRGBColor;

/*
 * @constant TCustomColors
 *
 * The RGB color table from the NES version of the real variant of this game.
 */
static const TRGBColor TCustomColors[] = {
                    { .r = 0x66, .g = 0x66, .b = 0x66 },
                    { .r = 0x00, .g = 0x2A, .b = 0x88 },
                    { .r = 0x14, .g = 0x12, .b = 0xA7 },
                    { .r = 0x3B, .g = 0x00, .b = 0xA4 },
                    { .r = 0x5C, .g = 0x00, .b = 0x7E },
                    { .r = 0x6E, .g = 0x00, .b = 0x40 },
                    { .r = 0x6C, .g = 0x07, .b = 0x00 },
                    { .r = 0x56, .g = 0x1D, .b = 0x00 },
                    { .r = 0x33, .g = 0x35, .b = 0x00 },
                    { .r = 0x0C, .g = 0x48, .b = 0x00 },
                    { .r = 0x00, .g = 0x52, .b = 0x00 },
                    { .r = 0x00, .g = 0x4F, .b = 0x08 },
                    { .r = 0x00, .g = 0x40, .b = 0x4D },
                    { .r = 0x00, .g = 0x00, .b = 0x00 },
                    { .r = 0x00, .g = 0x00, .b = 0x00 },
                    { .r = 0x00, .g = 0x00, .b = 0x00 },
                    { .r = 0xAD, .g = 0xAD, .b = 0xAD },
                    { .r = 0x15, .g = 0x5F, .b = 0xD9 },
                    { .r = 0x42, .g = 0x40, .b = 0xFF },
                    { .r = 0x75, .g = 0x27, .b = 0xFE },
                    { .r = 0xA0, .g = 0x1A, .b = 0xCC },
                    { .r = 0xB7, .g = 0x1E, .b = 0x7B },
                    { .r = 0xB5, .g = 0x31, .b = 0x20 },
                    { .r = 0x99, .g = 0x4E, .b = 0x00 },
                    { .r = 0x6B, .g = 0x6D, .b = 0x00 },
                    { .r = 0x38, .g = 0x87, .b = 0x00 },
                    { .r = 0x0D, .g = 0x93, .b = 0x00 },
                    { .r = 0x00, .g = 0x8F, .b = 0x32 },
                    { .r = 0x00, .g = 0x7C, .b = 0x8D },
                    { .r = 0x00, .g = 0x00, .b = 0x00 },
                    { .r = 0x00, .g = 0x00, .b = 0x00 },
                    { .r = 0x00, .g = 0x00, .b = 0x00 },
                    { .r = 0xFF, .g = 0xFF, .b = 0xFF },
                    { .r = 0x64, .g = 0xB0, .b = 0xFF },
                    { .r = 0x92, .g = 0x90, .b = 0xFF },
                    { .r = 0xC6, .g = 0x76, .b = 0xFF },
                    { .r = 0xF2, .g = 0x6A, .b = 0xFF },
                    { .r = 0xFF, .g = 0x6E, .b = 0xCC },
                    { .r = 0xFF, .g = 0x81, .b = 0x70 },
                    { .r = 0xEA, .g = 0x9E, .b = 0x22 },
                    { .r = 0xBC, .g = 0xBE, .b = 0x00 },
                    { .r = 0x88, .g = 0xD8, .b = 0x00 },
                    { .r = 0x5C, .g = 0xE4, .b = 0x30 },
                    { .r = 0x45, .g = 0xE0, .b = 0x82 },
                    { .r = 0x48, .g = 0xCD, .b = 0xDE },
                    { .r = 0x4F, .g = 0x4F, .b = 0x4F },
                    { .r = 0x00, .g = 0x00, .b = 0x00 },
                    { .r = 0x00, .g = 0x00, .b = 0x00 },
                    { .r = 0xFF, .g = 0xFF, .b = 0xFF },
                    { .r = 0xC0, .g = 0xDF, .b = 0xFF },
                    { .r = 0xD3, .g = 0xD2, .b = 0xFF },
                    { .r = 0xE8, .g = 0xC8, .b = 0xFF },
                    { .r = 0xFA, .g = 0xC2, .b = 0xFF },
                    { .r = 0xFF, .g = 0xC4, .b = 0xEA },
                    { .r = 0xFF, .g = 0xCC, .b = 0xC5 },
                    { .r = 0xF7, .g = 0xD8, .b = 0xA5 },
                    { .r = 0xE4, .g = 0xE5, .b = 0x94 },
                    { .r = 0xCF, .g = 0xEF, .b = 0x96 },
                    { .r = 0xBD, .g = 0xF4, .b = 0xAB },
                    { .r = 0xB3, .g = 0xF3, .b = 0xCC },
                    { .r = 0xB5, .g = 0xEB, .b = 0xF2 },
                    { .r = 0xB8, .g = 0xB8, .b = 0xB8 },
                    { .r = 0x00, .g = 0x00, .b = 0x00 },
                    { .r = 0x00, .g = 0x00, .b = 0x00 },
                };

/*
 * @function TCustomColor8BitToCurses
 *
 * Converts an 8-bit color channel value to the [0,1000] short integer
 * format used by curses.
 */
static inline short
TCustomColor8BitToCurses(
    uint8_t     byte
)
{
    return ((short)1000 * (short)byte) / (short)255;
}

/*
 * @function __TCustomColorPalettesSetActivePalette
 *
 * Given the paletteId (which is really just the current game level), set the custom RGB
 * colors in curses color slots 11, 12, and 13.  Color pairs 1, 2, and 3 will have been
 * configured at the start of the program to use combinations of those three color slots,
 * so the pairs never need to be altered again, we just alter the color slots to change
 * the active palette.
 */
static inline void
__TCustomColorPalettesSetActivePalette(
    int     paletteId
)
{
    #define CUSTOM_COLOR_R(I)  TCustomColor8BitToCurses(TCustomColors[TCustomColorIndices[paletteId * 4 + 1 + (I)]].r)
    #define CUSTOM_COLOR_G(I)  TCustomColor8BitToCurses(TCustomColors[TCustomColorIndices[paletteId * 4 + 1 + (I)]].g)
    #define CUSTOM_COLOR_B(I)  TCustomColor8BitToCurses(TCustomColors[TCustomColorIndices[paletteId * 4 + 1 + (I)]].b)
    
    // Palettes repeat every 10 levels:
    paletteId %= 10;
    
    init_color(11, CUSTOM_COLOR_R(0), CUSTOM_COLOR_G(0), CUSTOM_COLOR_B(0));
    init_color(12, CUSTOM_COLOR_R(1), CUSTOM_COLOR_G(1), CUSTOM_COLOR_B(1));
    init_color(13, CUSTOM_COLOR_R(2), CUSTOM_COLOR_G(2), CUSTOM_COLOR_B(2));
    #undef CUSTOM_COLOR_B
    #undef CUSTOM_COLOR_G
    #undef CUSTOM_COLOR_R
}

/*
 * @var gForceBasicColors
 *
 * Set via a command-line flag to force the use of basic curses colors
 * even if the runtime supports changeable colors.
 */
static bool gForceBasicColors = false;

/*
 * @function TColorPaletteSelect
 *
 * Configure curses so that the colors associated with the desired palette
 * (by paletteId, modulus the number of palettes) are loaded into pairs
 * 1, 2, and 3.  Pair 4 is loaded with the colors used for the game
 * title.
 */
static inline void
TColorPaletteSelect(
    int         paletteId
)
{
    static bool isInited = false, useCustomColors = false;
    
    if ( ! isInited ) {
        if ( ! gForceBasicColors && can_change_color() ) {
            // We only do this once and then alter color slots 11, 12, and
            // 13 when changing palettes:
            init_pair(1, 11, 12);
            init_pair(2, 12, 13);
            init_pair(3, 13, 11);
            init_pair(4, 13, COLOR_BLACK);
            useCustomColors = true;
        }
        isInited = true;
    }
    if ( useCustomColors ) {
        __TCustomColorPalettesSetActivePalette(paletteId);
    } else {
        __TBasicColorPalettesSetActivePalette(paletteId);
    }
}

#endif /* ENABLE_COLOR_DISPLAY */

//
////
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
gameBoardDraw_BW(
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
gameTitleDraw_BW(
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
statsDraw_BW(
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
nextTetrominoDraw_BW(
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
////
//

#ifdef ENABLE_COLOR_DISPLAY

void
gameBoardDraw_COLOR(
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
    TBitGridIterator    *gridScanner = TBitGridIteratorCreate(THE_BOARD, 0x7);
    TGridPos            P;
    uint16_t            spriteBits = TSpriteGet4x4(&THE_PIECE);
    int                 spriteColorIdx = 1 + THE_PIECE.colorIdx;
    
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
                *line1Ptr++ = COLOR_PAIR(1 + (j %3)) | ' '; *line1Ptr++ = COLOR_PAIR(1 + (j %3)) | ' '; *line1Ptr++ = COLOR_PAIR(1 + (j %3)) | ' '; *line1Ptr++ = COLOR_PAIR(1 + (j %3)) | ' ';
                *line2Ptr++ = COLOR_PAIR(1 + (j %3)) | '_'; *line2Ptr++ = COLOR_PAIR(1 + (j %3)) | '_'; *line2Ptr++ = COLOR_PAIR(1 + (j %3)) | '_'; *line2Ptr++ = COLOR_PAIR(1 + (j %3)) | '_';
            }
            else if ( spriteBit ) {
                *line1Ptr++ = COLOR_PAIR(spriteColorIdx) | '|'; *line1Ptr++ = COLOR_PAIR(spriteColorIdx) | ' '; *line1Ptr++ = COLOR_PAIR(spriteColorIdx) | ' '; *line1Ptr++ = COLOR_PAIR(spriteColorIdx) | ' ';
                *line2Ptr++ = COLOR_PAIR(spriteColorIdx) | '|'; *line2Ptr++ = COLOR_PAIR(spriteColorIdx) | '_'; *line2Ptr++ = COLOR_PAIR(spriteColorIdx) | '_'; *line2Ptr++ = COLOR_PAIR(spriteColorIdx) | '_';
            }
            else if ( (gridBit && TCellGetIsOccupied(cellValue)) ) {
                int     colorIdx = 1 + TCellGetColorIndex(cellValue);
                
                *line1Ptr++ = COLOR_PAIR(colorIdx) | '|'; *line1Ptr++ = COLOR_PAIR(colorIdx) | ' '; *line1Ptr++ = COLOR_PAIR(colorIdx) | ' '; *line1Ptr++ = COLOR_PAIR(colorIdx) | ' ';
                *line2Ptr++ = COLOR_PAIR(colorIdx) | '|'; *line2Ptr++ = COLOR_PAIR(colorIdx) | '_'; *line2Ptr++ = COLOR_PAIR(colorIdx) | '_'; *line2Ptr++ = COLOR_PAIR(colorIdx) | '_';
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

void
gameTitleDraw_COLOR(
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
        
        wattron(inWindow, COLOR_PAIR(4));
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
        wattroff(inWindow, COLOR_PAIR(4));
    }
}

//

void
statsDraw_COLOR(
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
                waddch(window_ptr, '|' | A_REVERSE | COLOR_PAIR(1 + colorIdx));
                waddch(window_ptr, '_' | A_REVERSE | COLOR_PAIR(1 + colorIdx));
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
                waddch(window_ptr, '|' | A_REVERSE | COLOR_PAIR(1 + colorIdx));
                waddch(window_ptr, '_' | A_REVERSE | COLOR_PAIR(1 + colorIdx));
            } else {
                waddch(window_ptr, ' ');
                waddch(window_ptr, ' ');
            }
            mask <<= 1;
        }
        tIdx++;
        colorIdx = (colorIdx + 1) % 3;
    }
#undef SCOREBOARD
#undef GAME_ENGINE
}

//

void
nextTetrominoDraw_COLOR(
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
    int             colorIdx = 1 + GAME_ENGINE->nextSprite.colorIdx;
    
    wclear(window_ptr);

    rep = TSpriteGet4x4(&GAME_ENGINE->nextSprite);
    
    while ( j < 4 ) {
        line1Ptr = line1, line2Ptr = line2;
        count = 4; while ( count-- ) {
            if (rep & mask) {
                *line1Ptr++ = '|' | COLOR_PAIR(colorIdx); *line1Ptr++ = ' ' | COLOR_PAIR(colorIdx); *line1Ptr++ = ' ' | COLOR_PAIR(colorIdx); *line1Ptr++ = ' ' | COLOR_PAIR(colorIdx);
                *line2Ptr++ = '|' | COLOR_PAIR(colorIdx); *line2Ptr++ = '_' | COLOR_PAIR(colorIdx); *line2Ptr++ = '_' | COLOR_PAIR(colorIdx); *line2Ptr++ = '_' | COLOR_PAIR(colorIdx);
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

#endif /* ENABLE_COLOR_DISPLAY */

//
////
//

static const char *THighScoresFilePath = TETROMINOTRIS_HISCORES_FILEPATH;
static const char *THighScoresInitialsCharSet = "?ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789-_.";
static const unsigned int THighScoresInitialsCharSetLen = 66;

static inline int
__highScoreIdxForChar(
    char        c
)
{
    int         idx = 0;
    
    while ( idx < THighScoresInitialsCharSetLen ) {
        if ( THighScoresInitialsCharSet[idx] == c ) return idx;
        idx++;
    }
    return -1;
}

typedef enum {
    highScoreSelectedControlInitial0 = 0,
    highScoreSelectedControlInitial1,
    highScoreSelectedControlInitial2,
    highScoreSelectedControlCancel,
    highScoreSelectedControlSave,
    highScoreSelectedControlMax
} highScoreSelectedControl;

typedef struct {
    unsigned int                rank;
    bool                        updated[highScoreSelectedControlMax + 1];
    highScoreSelectedControl    selectedIdx;
    unsigned int                initialIdx[3];
    THighScoresRef              highScores;
} THighScoreWindowContext;

void
highScoreDraw(
    tui_window_ref  the_window,
    WINDOW          *window_ptr,
    const void      *context
)
{
#define UPDATED ((THighScoreWindowContext*)context)->updated  
#define SELECTEDIDX ((THighScoreWindowContext*)context)->selectedIdx  
#define INITIALIDX ((THighScoreWindowContext*)context)->initialIdx
#define RANK ((THighScoreWindowContext*)context)->rank
#define HIGHSCORES ((THighScoreWindowContext*)context)->highScores
    if ( RANK != 0xFFFFFFFF ) {
        if ( UPDATED[highScoreSelectedControlMax] ) {
            mvwprintw(window_ptr, 2, 2, "Congratulations, you've earned a high score!");
            mvwprintw(window_ptr, 4, 14, "Enter your initials:");
            UPDATED[highScoreSelectedControlMax] = false;
        }
        if ( UPDATED[highScoreSelectedControlInitial0] ) {
            mvwaddch(window_ptr, 6, 18, THighScoresInitialsCharSet[INITIALIDX[highScoreSelectedControlInitial0]] | ((SELECTEDIDX == highScoreSelectedControlInitial0) ? (A_REVERSE | A_BLINK) : 0));
            UPDATED[highScoreSelectedControlInitial0] = false;
        }
        if ( UPDATED[highScoreSelectedControlInitial1] ) {
            mvwaddch(window_ptr, 6, 23, THighScoresInitialsCharSet[INITIALIDX[highScoreSelectedControlInitial1]] | ((SELECTEDIDX == highScoreSelectedControlInitial1) ? (A_REVERSE | A_BLINK) : 0));
            UPDATED[highScoreSelectedControlInitial1] = false;
        }
        if ( UPDATED[highScoreSelectedControlInitial2] ) {
            mvwaddch(window_ptr, 6, 28, THighScoresInitialsCharSet[INITIALIDX[highScoreSelectedControlInitial2]] | ((SELECTEDIDX == highScoreSelectedControlInitial2) ? (A_REVERSE | A_BLINK) : 0));
            UPDATED[highScoreSelectedControlInitial2] = false;
        }
        if ( UPDATED[highScoreSelectedControlCancel] ) {
            if ( SELECTEDIDX == highScoreSelectedControlCancel ) wattron(window_ptr, A_REVERSE);
            mvwprintw(window_ptr, 8, 15, "CANCEL");
            if ( SELECTEDIDX == highScoreSelectedControlCancel ) wattroff(window_ptr, A_REVERSE);
            UPDATED[highScoreSelectedControlCancel] = false;
        }
        if ( UPDATED[highScoreSelectedControlSave] ) {
            if ( SELECTEDIDX == highScoreSelectedControlSave ) wattron(window_ptr, A_REVERSE);
            mvwprintw(window_ptr, 8, 27, " SAVE ");
            if ( SELECTEDIDX == highScoreSelectedControlSave ) wattroff(window_ptr, A_REVERSE);
            UPDATED[highScoreSelectedControlSave] = false;
        }
    } else {
        unsigned int        i = 0, iMax = THighScoresGetCount(HIGHSCORES);
        
        wclear(window_ptr);
        while ( i < iMax ) {
            unsigned int    score;
            char            initials[3];
            
            THighScoresGetRecord(HIGHSCORES, i, &score, initials);
            mvwprintw(window_ptr, 2 + 2 * i, 14, "%u.    %c %c %c    %9u", i + 1, initials[0], initials[1], initials[2], score);
            i++;
        }
        wattron(window_ptr, A_BLINK); mvwprintw(window_ptr, 8, 17, "Press any key."); wattroff(window_ptr, A_BLINK); 
    }
#undef UPDATED
#undef SELECTEDIDX
#undef INITIALIDX
#undef RANK
#undef HIGHSCORES
}

void
doHighScoreWindow(
    THighScoresRef          highScores,
    tui_window_rect_t       bounds,
    unsigned int            score,
    unsigned int            rank
)
{
    int                     keyCh;
    bool                    isModal = true, shouldSave = false;
    THighScoreWindowContext context = {
                                .rank = rank,
                                .updated = { true, true, true, true, true, true },
                                .selectedIdx = highScoreSelectedControlInitial0,
                                .initialIdx = { 0, 0, 0 },
                                .highScores = highScores
                            };
    tui_window_ref  highScoreWindow = tui_window_alloc(
                                                    bounds,
                                                    tui_window_opts_title_align_center,
                                                    "HIGH SCORES", 0,
                                                    highScoreDraw, (const void*)&context);
    tui_window_refresh(highScoreWindow, 0);
    timeout(-1);
    if ( rank != 0xFFFFFFFF ) {
        // Qualified for a high score, yay!
        while ( isModal ) {
            keyCh = getch();
            switch ( keyCh ) {
                case '\t': {
                    context.updated[context.selectedIdx] = true;
                    context.selectedIdx = (context.selectedIdx + 1) % highScoreSelectedControlMax;
                    context.updated[context.selectedIdx] = true;
                    break;
                }
                case '\r':
                case '\n': {
                    context.updated[context.selectedIdx] = true;
                    switch ( context.selectedIdx ) {

                        case highScoreSelectedControlInitial0:
                        case highScoreSelectedControlInitial1:
                        case highScoreSelectedControlInitial2:
                            context.updated[++context.selectedIdx] = true;
                            break;
                        case highScoreSelectedControlSave:
                            shouldSave = true;
                        case highScoreSelectedControlCancel:
                            isModal = false;
                            break;
                        case highScoreSelectedControlMax:
                            break;
                    }
                    break;
                }
                case KEY_UP: {
                    switch ( context.selectedIdx ) {
                        case highScoreSelectedControlInitial0:
                        case highScoreSelectedControlInitial1:
                        case highScoreSelectedControlInitial2:
                            if ( context.initialIdx[context.selectedIdx] == 0 )
                                context.initialIdx[context.selectedIdx] = THighScoresInitialsCharSetLen - 1;
                            else
                                context.initialIdx[context.selectedIdx] = (context.initialIdx[context.selectedIdx] - 1);
                            context.updated[context.selectedIdx] = true;
                            break;
                        case highScoreSelectedControlCancel:
                        case highScoreSelectedControlSave:
                        case highScoreSelectedControlMax:
                            break;
                    }
                    break;
                }
                case KEY_DOWN: {
                    switch ( context.selectedIdx ) {
                        case highScoreSelectedControlInitial0:
                        case highScoreSelectedControlInitial1:
                        case highScoreSelectedControlInitial2:
                            context.initialIdx[context.selectedIdx] = (context.initialIdx[context.selectedIdx] + 1) % THighScoresInitialsCharSetLen;
                            context.updated[context.selectedIdx] = true;
                            break;
                        case highScoreSelectedControlCancel:
                        case highScoreSelectedControlSave:
                        case highScoreSelectedControlMax:
                            break;
                    }
                    break;
                }
                default: {
                    switch ( context.selectedIdx ) {
                        case highScoreSelectedControlInitial0:
                        case highScoreSelectedControlInitial1:
                        case highScoreSelectedControlInitial2: {
                            int     idx = __highScoreIdxForChar(keyCh);
                            if ( idx >= 0 ) {
                                context.initialIdx[context.selectedIdx] = idx;
                                context.updated[context.selectedIdx] = true;
                            }
                            break;
                        }
                        case highScoreSelectedControlCancel:
                        case highScoreSelectedControlSave:
                        case highScoreSelectedControlMax:
                            break;
                    }
                    break;  
                }
            }
            tui_window_refresh(highScoreWindow, 0);
        }
        if ( shouldSave ) {
            char        initials[3] = {
                                        THighScoresInitialsCharSet[context.initialIdx[highScoreSelectedControlInitial0]],
                                        THighScoresInitialsCharSet[context.initialIdx[highScoreSelectedControlInitial1]],
                                        THighScoresInitialsCharSet[context.initialIdx[highScoreSelectedControlInitial2]]
                                    };
            THighScoresRegister(highScores, score, initials);
            THighScoresSave(highScores, THighScoresFilePath);
        }
        context.rank = 0xFFFFFFFF;
    }
    
    // Show the high score list:
    tui_window_refresh(highScoreWindow, 0);
    
    // Wait for a keypress...
    getch();
    
    tui_window_free(highScoreWindow);
    clear();
    
    timeout(0);
}

//
////
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

#ifdef ENABLE_COLOR_DISPLAY
    bool                wantsColor = false;
#endif

    unsigned int        startingLevel = 0, savedLevel;
    
    setlocale(LC_ALL, "");
    
    // Disable tab-based screen movement:
    setenv("NCURSES_NO_HARD_TABS", "1", 0);
    
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

#ifdef ENABLE_COLOR_DISPLAY
            case 'C':
                wantsColor = true;
                break;
            
            case 'B':
                gForceBasicColors = true;
                break;
#endif
            
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

#ifdef ENABLE_COLOR_DISPLAY
    // If the user wants color, make sure we can do it:
    if ( wantsColor ) {
        if ( ! has_colors() ) {
            delwin(mainWindow);
            endwin();
            refresh();
            printf("Your terminal does not support color display.\n\n");
            exit(1);
        }
        start_color();
    }
#endif
    
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
        printf("Please resize your terminal to at least 77 columns wide and 44 columns high.\n\n");
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
        printf("Please resize your terminal to at least 44 columns high.\n\n");
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

#ifdef ENABLE_COLOR_DISPLAY
    if ( wantsColor )
        gameEngine = TGameEngineCreate(3, gameBoardWidth, gameBoardHeight, startingLevel);
    else
#endif
    // Create the game engine:
    gameEngine = TGameEngineCreate(1, gameBoardWidth, gameBoardHeight, startingLevel);
    
    //
    // Initialize game windows:
    //
    memset(gameWindows, 0, sizeof(gameWindows));
    
#ifdef ENABLE_COLOR_DISPLAY
    if ( wantsColor )
        gameWindows[TWindowIndexGameBoard] = tui_window_alloc(
                                                    gameWindowsBounds[TWindowIndexGameBoard], 0,
                                                    NULL, 0,
                                                    gameBoardDraw_COLOR, (const void*)gameEngine);
    else
#endif
    gameWindows[TWindowIndexGameBoard] = tui_window_alloc(
                                                gameWindowsBounds[TWindowIndexGameBoard], 0,
                                                NULL, 0,
                                                gameBoardDraw_BW, (const void*)gameEngine);
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
    
#ifdef ENABLE_COLOR_DISPLAY
    if ( wantsColor )
        gameWindows[TWindowIndexNextTetromino] = tui_window_alloc(
                                                    gameWindowsBounds[TWindowIndexNextTetromino], tui_window_opts_title_align_right,
                                                    "NEXT UP", 0,
                                                    nextTetrominoDraw_COLOR, (const void*)gameEngine);
    else
#endif
    gameWindows[TWindowIndexNextTetromino] = tui_window_alloc(
                                                gameWindowsBounds[TWindowIndexNextTetromino], tui_window_opts_title_align_right,
                                                "NEXT UP", 0,
                                                nextTetrominoDraw_BW, (const void*)gameEngine);
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
#ifdef ENABLE_COLOR_DISPLAY
        gameWindows[TWindowIndexStats] = tui_window_alloc(
                                                    gameWindowsBounds[TWindowIndexStats], 0,
                                                    "STATS", 0,
                                                    wantsColor ? statsDraw_COLOR : statsDraw_BW, (const void*)gameEngine);
#else
        gameWindows[TWindowIndexStats] = tui_window_alloc(
                                                    gameWindowsBounds[TWindowIndexStats], 0,
                                                    "STATS", 0,
                                                    statsDraw_BW, (const void*)gameEngine);
#endif
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

#ifdef ENABLE_COLOR_DISPLAY
    // Load the initial color palette:
    TColorPaletteSelect(gameEngine->scoreboard.level);
#endif

    // Initial draw of all windows:
    refresh();
    if ( isGameTitleDisplayed ) {
#ifdef ENABLE_COLOR_DISPLAY
        if ( wantsColor ) 
            gameTitleDraw_COLOR(mainWindow, screenWidth);
        else
#endif
        gameTitleDraw_BW(mainWindow, screenWidth);
    }
    for ( idx = 0; idx < TWindowIndexMax; idx++ ) if (gameWindowsEnabled & (1 << idx)) tui_window_refresh(gameWindows[idx], 1);
    doupdate();
    
    // Key checks should be non-blocking:
    timeout(0);
    
    savedLevel = gameEngine->scoreboard.level;
    TGameEngineTick(gameEngine, TGameEngineEventStartGame);
    
    while ( 1 ) {
        TGameEngineUpdateNotification   updateNotifications;
        TGameEngineEvent                gameEngineEvent = TGameEngineEventNoOp;
        
        keyCh = getch();
        if ( (keyCh == 'Q') || (keyCh == 'q') ) {
            break;
        }
        
        // Check for game over:
        if ( gameEngine->hasEnded && ! gameEngine->didDoHighScores ) {
            THighScoresRef      highScores = THighScoresLoad(THighScoresFilePath);
            unsigned int        highScoreRank;
    
            if ( ! highScores ) highScores = THighScoresCreate();
            if ( THighScoresDoesQualify(highScores, gameEngine->scoreboard.score, &highScoreRank) ) {
                doHighScoreWindow(
                        highScores,
                        tui_window_rect_make(screenWidth / 2 - 48 / 2,
                                             screenHeight / 2 - 10/ 2,
                                             48,
                                             10
                                    ),
                        gameEngine->scoreboard.score,
                        highScoreRank
                    );
            } else {
                doHighScoreWindow(
                        highScores,
                        tui_window_rect_make(screenWidth / 2 - 48 / 2,
                                             screenHeight / 2 - 10/ 2,
                                             48,
                                             10
                                    ),
                        gameEngine->scoreboard.score,
                        0xFFFFFFFF
                    );
            }
            gameEngine->didDoHighScores = true;
            
            refresh();
            if ( isGameTitleDisplayed ) {
#ifdef ENABLE_COLOR_DISPLAY
                if ( wantsColor ) 
                    gameTitleDraw_COLOR(mainWindow, screenWidth);
                else
#endif
                gameTitleDraw_BW(mainWindow, screenWidth);
            }
            for ( idx = 0; idx < TWindowIndexMax; idx++ ) if (gameWindowsEnabled & (1 << idx)) tui_window_refresh(gameWindows[idx], 1);
            doupdate();
            updateNotifications = 0;
        } else {
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
        }        
        if ( updateNotifications ) {
            refresh();
            
            if ( updateNotifications & TGameEngineUpdateNotificationGameBoard ) {
                tui_window_refresh(gameWindows[TWindowIndexGameBoard], 1);
            }
            if ( updateNotifications & TGameEngineUpdateNotificationScoreboard ) {
#ifdef ENABLE_COLOR_DISPLAY
                // Update the color palette if the level changed:
                if ( savedLevel != gameEngine->scoreboard.level ) {
                    TColorPaletteSelect((savedLevel = gameEngine->scoreboard.level));
                }
#endif
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
    refresh();    
    endwin();
    refresh();
    
    printf("%lg seconds, %lu ticks = %12.3lg ticks/sec\n", dt_sec, gameEngine->tickCount, (double)gameEngine->tickCount / dt_sec);
    
    return 0;
}
