
#include "TKeymap.h"

#include <ctype.h>

const TKeymap TKeymapDefault = {
                    .ascii_20_7d = {
                        TGameEngineEventRotateClockwise,        // 0x20 = 32 ' '
                        TGameEngineEventNoOp, TGameEngineEventNoOp, TGameEngineEventNoOp,
                        TGameEngineEventNoOp, TGameEngineEventNoOp, TGameEngineEventNoOp,
                        TGameEngineEventNoOp, TGameEngineEventNoOp, TGameEngineEventNoOp,
                        TGameEngineEventNoOp, TGameEngineEventNoOp, 
                        TGameEngineEventMoveLeft,               // 0x2c = 44 ','
                        TGameEngineEventNoOp, 
                        TGameEngineEventMoveRight,              // 0x2e = 46 '.'
                        TGameEngineEventNoOp, TGameEngineEventNoOp, TGameEngineEventNoOp,
                        TGameEngineEventNoOp, TGameEngineEventNoOp, TGameEngineEventNoOp,
                        TGameEngineEventNoOp, TGameEngineEventNoOp, TGameEngineEventNoOp,
                        TGameEngineEventNoOp, TGameEngineEventNoOp, TGameEngineEventNoOp,
                        TGameEngineEventNoOp,                   // 0x3b = 59
                        TGameEngineEventMoveLeft,               // 0x3c = 60 '<'
                        TGameEngineEventNoOp,
                        TGameEngineEventMoveRight,              // 0x3e = 62 '>'
                        TGameEngineEventNoOp, TGameEngineEventNoOp,
                        TGameEngineEventRotateAntiClockwise,    // 0x41 = 65 'A'
                        TGameEngineEventNoOp, TGameEngineEventNoOp,
                        TGameEngineEventHardDrop,               // 0x44 = 68 'D'
                        TGameEngineEventNoOp, TGameEngineEventNoOp, TGameEngineEventNoOp,
                        TGameEngineEventNoOp, TGameEngineEventNoOp, TGameEngineEventNoOp,
                        TGameEngineEventNoOp, TGameEngineEventNoOp, TGameEngineEventNoOp,
                        TGameEngineEventNoOp, TGameEngineEventNoOp,
                        TGameEngineEventTogglePause,            // 0x50 = 80 'P'
                        TGameEngineEventNoOp,
                        TGameEngineEventReset,                  // 0x52 = 82 'R'
                        TGameEngineEventRotateClockwise,        // 0x53 = 83 'S'
                        TGameEngineEventNoOp, TGameEngineEventNoOp,
                        TGameEngineEventHardDrop,               // 0x56 = 86 'V'
                        TGameEngineEventNoOp, TGameEngineEventNoOp, TGameEngineEventNoOp,
                        TGameEngineEventNoOp, TGameEngineEventNoOp, TGameEngineEventNoOp,
                        TGameEngineEventNoOp, TGameEngineEventNoOp, TGameEngineEventNoOp,
                        TGameEngineEventNoOp,
                        TGameEngineEventRotateAntiClockwise,    // 0x61 = 97 'a'
                        TGameEngineEventNoOp, TGameEngineEventNoOp,
                        TGameEngineEventHardDrop,               // 0x64 = 100 'd'
                        TGameEngineEventNoOp, TGameEngineEventNoOp, TGameEngineEventNoOp,
                        TGameEngineEventNoOp, TGameEngineEventNoOp, TGameEngineEventNoOp,
                        TGameEngineEventNoOp, TGameEngineEventNoOp, TGameEngineEventNoOp,
                        TGameEngineEventNoOp, TGameEngineEventNoOp,
                        TGameEngineEventTogglePause,            // 0x70 = 112 'p'
                        TGameEngineEventNoOp,
                        TGameEngineEventReset,                  // 0x72 = 114 'r'
                        TGameEngineEventRotateClockwise,        // 0x73 = 115 's'
                        TGameEngineEventNoOp, TGameEngineEventNoOp,
                        TGameEngineEventSoftDrop,               // 0x76 = 118 'v'
                        TGameEngineEventNoOp, TGameEngineEventNoOp, TGameEngineEventNoOp,
                        TGameEngineEventNoOp
                    }
                };

//

#define CONFIGURABLE_EVENT_BITS ((1 << TGameEngineEventMoveLeft) | (1 << TGameEngineEventMoveRight) | (1 << TGameEngineEventRotateClockwise) | (1 << TGameEngineEventRotateAntiClockwise) | (1 << TGameEngineEventSoftDrop) | (1 << TGameEngineEventHardDrop) | (1 << TGameEngineEventTogglePause) | (1 << TGameEngineEventReset))
#define MANDATORY_EVENT_BITS ((1 << TGameEngineEventRotateClockwise) | (1 << TGameEngineEventRotateAntiClockwise) | (1 << TGameEngineEventHardDrop) | (1 << TGameEngineEventTogglePause) | (1 << TGameEngineEventReset))

TKeymap*
TKeymapInitWithFile(
    TKeymap     *keymap,
    const char  *filepath
)
{
    FILE            *fptr = fopen(filepath, "r");
    unsigned int    haveEvents = 0;
    
    if ( fptr ) {
        char        *buffer = malloc(256);
        int         bufferLen = 256;
        int         ch;
        
        // Reset the map to all no-ops:
        memset(keymap->ascii_20_7d, TGameEngineEventNoOp, sizeof(keymap->ascii_20_7d));
        
        // Loop through the file:
        while ( true ) {
            // Skip any whitespace:
            while ( (ch = fgetc(fptr)) && isspace(ch) );
            
            // End of file?
            if ( ch == EOF ) break;
            
            // Comment?
            if ( ch == '#' ) {
                while ( (ch = fgetc(fptr)) && (ch != '\r' && ch != '\n') );
            } else {
                int                 i = 0, iEquals = -1;
                TGameEngineEvent    event = TGameEngineEventNoOp;
                char                *key, *value;
                
                // Directive line:
                buffer[i++] = ch;
                while ( true ) {
                    ch = fgetc(fptr);
                    if ( ch == EOF || ch == '\r' || ch == '\n' ) break;
                    if ( i + 2 == bufferLen ) {
                        char    *newBuffer = realloc(buffer, bufferLen + 256);
                        
                        if ( ! newBuffer ) {
                            free(buffer);
                            exit(ENOMEM);
                        }
                        buffer = newBuffer;
                        bufferLen += 256;
                    }
                    if ( (iEquals < 0) && (ch == '=') ) iEquals = i;
                    buffer[i++] = ch;
                }
                buffer[i++] = '\0';
                
                if ( iEquals < 0 ) {
                    // Every key is a NoOp:
                    key = buffer;
                } else {
                    // We have a list of keys = event:
                    char        *nextToken, *sepstate;
                    
                    key = buffer;
                    value = buffer + iEquals + 1;
                    buffer[iEquals] = '\0';
                    
                    // Parse the value to get the event:
                    while ( isspace(*value) ) value++;
                    
                    if ( ! strncasecmp(value, "NO-OP", 5) || ! strncasecmp(value, "NO OP", 5) || ! strncasecmp(value, "NO_OP", 5) || ! strncasecmp(value, "NOOP", 4) ) {
                        event = TGameEngineEventNoOp;
                    }
                    else if ( ! strncasecmp(value, "RESET", 5) ) {
                        event = TGameEngineEventReset;
                    }
                    else if ( ! strncasecmp(value, "PAUSE", 5) ) {
                        event = TGameEngineEventTogglePause;
                    }
                    else if ( ! strncasecmp(value, "HARD-DROP", 9) || ! strncasecmp(value, "HARD DROP", 9) || ! strncasecmp(value, "HARD_DROP", 9) ) {
                        event = TGameEngineEventHardDrop;
                    }
                    else if ( ! strncasecmp(value, "SOFT-DROP", 9) || ! strncasecmp(value, "SOFT DROP", 9) || ! strncasecmp(value, "SOFT_DROP", 9) ) {
                        event = TGameEngineEventSoftDrop;
                    }
                    else if ( ! strncasecmp(value, "MOVE-LEFT", 9) || ! strncasecmp(value, "MOVE LEFT", 9) || ! strncasecmp(value, "MOVE_LEFT", 9) ) {
                        event = TGameEngineEventMoveLeft;
                    }
                    else if ( ! strncasecmp(value, "MOVE-RIGHT", 10) || ! strncasecmp(value, "MOVE RIGHT", 10) || ! strncasecmp(value, "MOVE_RIGHT", 10) ) {
                        event = TGameEngineEventMoveRight;
                    }
                    else if ( ! strncasecmp(value, "ROTATE-CLOCKWISE", 16) || ! strncasecmp(value, "ROTATE CLOCKWISE", 16) || ! strncasecmp(value, "ROTATE_CLOCKWISE", 16) ) {
                        event = TGameEngineEventRotateClockwise;
                    }
                    else if ( ! strncasecmp(value, "ROTATE-ANTICLOCKWISE", 20) || ! strncasecmp(value, "ROTATE ANTICLOCKWISE", 20) || ! strncasecmp(value, "ROTATE_ANTICLOCKWISE", 20) ) {
                        event = TGameEngineEventRotateAntiClockwise;
                    }
                    else {
                        fprintf(stderr, "ERROR:  invalid event '%s' for key(s) '%s'\n", value, key);
                        exit(EINVAL);
                    }
                    
                    // We have the target event, now determine what keys to associate
                    // it with.  Tokenize words by punctuation:
                    sepstate = key;
                    while ( (nextToken = strsep(&sepstate, " \t")) ) {
                        char        c;
                        
                        if ( *nextToken ) {
                            if ( ! strcasecmp(nextToken, "space") || ! strcasecmp(nextToken, "spacebar") ) {
                                c = ' ';
                            }
                            else if ( *nextToken && ! *(nextToken + 1) ) {
                                c = *nextToken;
                                if ( c < 0x20 || c > 0x7d ) {
                                    fprintf(stderr, "ERROR:  invalid key name: 0x%1$02hhX (%1$hhu)\n", c);
                                    exit(EINVAL);
                                }
                                if ( keymap->ascii_20_7d[c - ' '] != TGameEngineEventNoOp &&
                                     keymap->ascii_20_7d[c - ' '] != event )
                                {
                                    fprintf(stderr, "ERROR:  key '%c' already assigned an event %u\n", c, keymap->ascii_20_7d[c] );
                                    exit(EINVAL);
                                }
                            }
                            else {
                                fprintf(stderr, "ERROR:  invalid key name: %s\n", nextToken);
                                exit(EINVAL);
                            }
                            keymap->ascii_20_7d[c - ' '] = event;
                            haveEvents |= (1 << event);
                        }
                    }
                }
            }
        }
        fclose(fptr);
        
        // Ensure every mandatory control is present -- the left/right/down arrow keys
        // are always enabled for left/right/soft-drop events:
        if ( (haveEvents & MANDATORY_EVENT_BITS) != MANDATORY_EVENT_BITS ) {
            fprintf(stderr, "ERROR:  not all events have been assigned to keys in '%s' (%08X vs. %08X)\n", filepath, haveEvents, MANDATORY_EVENT_BITS);
            exit(EINVAL);
        }
        free(buffer);
    }
    return NULL;
}

//

const char*
TKeymapKeySummaryForEvent(
    TKeymap             *keymap,
    TGameEngineEvent    event
)
{
    static char         outBuffer[16];
    char                c1 = 0, c2 = 0, *p = outBuffer;
    int                 i = 0, plen = sizeof(outBuffer), n;
    
    i = 'A' - ' ';
    while ( (! c1 || ! c2) && (i <= ('Z' - ' ')) ) {
        if ( keymap->ascii_20_7d[i] == event ) {
            if ( ! c1 ) c1 = i + 1;
            else if ( ! c2 ) c2 = i + 1;
        }
        i++;
    }
    if ( ! c1 || ! c2 ) {
        i = 'a' - ' ';
        while ( (! c1 || ! c2) && (i <= ('z' - ' ')) ) {
            if ( keymap->ascii_20_7d[i] == event ) {
                if ( ! c1 ) c1 = i + 1;
                else if ( ! c2 ) c2 = i + 1;
            }
            i++;
        }
    }
    if ( ! c1 || ! c2 ) {
        i = 0;
        while ( (! c1 || ! c2) && (i < ('A' - ' ')) ) {
            if ( keymap->ascii_20_7d[i] == event ) {
                if ( ! c1 ) c1 = i + 1;
                else if ( ! c2 ) c2 = i + 1;
            }
            i++;
        }
    }
    if ( ! c1 || ! c2 ) {
        i = 91;
        while ( (! c1 || ! c2) && (i < ('a' - ' ')) ) {
            if ( keymap->ascii_20_7d[i] == event ) {
                if ( ! c1 ) c1 = i + 1;
                else if ( ! c2 ) c2 = i + 1;
            }
            i++;
        }
    }
    if ( ! c1 || ! c2 ) {
        i = 123;
        while ( (! c1 || ! c2) && (i < 127) ) {
            if ( keymap->ascii_20_7d[i] == event ) {
                if ( ! c1 ) c1 = i + 1;
                else if ( ! c2 ) c2 = i + 1;
            }
            i++;
        }
    }
    if ( c1-- ) {
        if ( c1 == 0 ) {
            n = snprintf(p, plen, "SPC");
            p += n, plen -= n;
        } else {
            n = snprintf(p, plen, "%c", c1 + ' ');
            p += n, plen -= n;
        }
    } else {
        outBuffer[0] = '\0';
    }
    if ( c2-- ) {
        if ( c2 == 0 ) {
            n = snprintf(p, plen, " SPC");
            p += n, plen -= n;
        } else {
            n = snprintf(p, plen, " %c", c2 + ' ');
            p += n, plen -= n;
        }
    }
    return outBuffer;
}
