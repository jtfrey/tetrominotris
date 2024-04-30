/*	high-scores.c
	Copyright (c) 2024, J T Frey
*/

#include "THighScores.h"

#include <getopt.h>

static struct option cliArgOpts[] = {
    { "help",           no_argument,        NULL,       'h' },
    { "reset",          no_argument,        NULL,       'r' },
    { NULL,             0,                  NULL,        0  }
};

static const char *cliArgOptsStr = "hr";

void
usage(
    const char      *exe
)
{
    printf(
        "\n"
        "usage:\n"
        "\n"
        "    %s {options} <filepath>\n"
        "\n"
        "  options:\n"
        "\n"
        "    --help/-h                      show this information\n"
        "    --reset/-r                     clear the high score data\n"
        "\n"
        "version: " TETROMINOTRIS_VERSION "\n"
        "\n",
        exe
    );
}

//
////
//

typedef enum {
    hiscoresModePrint = 0,
    hiscoresModeReset
} highScoresMode;

int
main(
    int             argc,
    char * const    argv[]
)
{
    highScoresMode  mode = hiscoresModePrint;
    int             keyCh, argn = 0;

    // Parse CLI arguments:
    while ( (keyCh = getopt_long(argc, argv, cliArgOptsStr, cliArgOpts, NULL)) != -1 ) {
        switch ( keyCh ) {
            case 'h':
                usage(argv[0]);
                exit(0);
            case 'r':
                mode = hiscoresModeReset;
                break;
        }
    }
    argc -= optind;
    argv += optind;

    if ( argc > 0 ) {
        switch ( mode ) {
        
            case hiscoresModePrint: {
                THighScoresRef  highScores = THighScoresLoad(argv[argn]);
                unsigned int    i = 0, iMax;
        
                if ( ! highScores ) {
                    fprintf(stderr, "ERROR:  could not open/read the file: %s\n", argv[argn]);
                    exit(1);
                }
                iMax = THighScoresGetCount(highScores);
                while ( i < iMax ) {
                    unsigned int    score, level;
                    char            initials[3], timestamp[24];
        
                    if ( THighScoresGetRecord(highScores, i++, &score, &level, initials, timestamp, sizeof(timestamp)) ) {
                        printf("%2u.    %c %c %c      %8u (Lv %2u)  %s\n",
                            i, initials[0], initials[1], initials[2], score, level, timestamp);
                    }
                }
                THighScoresDestroy(highScores);
                break;
            }
            
            case hiscoresModeReset: {
                THighScoresRef  highScores = THighScoresCreate();
                
                if ( ! highScores ) {
                    fprintf(stderr, "ERROR:  could not create new high scores data set\n");
                    exit(1);
                }
                if ( ! THighScoresSave(highScores, argv[argn]) ) {
                    fprintf(stderr, "ERROR:  could not save high scores data to file: %s\n", argv[argn]);
                    exit(1);
                }
                THighScoresDestroy(highScores);
                break;
            }
            
        }
    }
    return 0;
}
