
#ifndef __TKEYMAP_H__
#define __TKEYMAP_H__

#include "tetrominotris_config.h"
#include "TGameEngine.h"

/*
 * @typedef TKeymap
 *
 * A keymap associates TGameEngine event codes with the printable
 * ASCII characters in the range 0x20 (space) to 0x7d (tilde).
 *
 * Beyond the default keymap, a function is provided to initialize
 * a TKeymap instance from a descriptive file format.
 */
typedef struct {
    TGameEngineEvent    ascii_20_7d[0x7e - 0x20 + 1];
} TKeymap;

/*
 * @constant TKeymapDefault
 *
 * The default key mapping for the game.
 */
extern const TKeymap TKeymapDefault;

/*
 * @function TKeymapInit
 *
 * Initialize the TKeymap at pointer keymap to the default game
 * key mapping.
 */
static inline TKeymap*
TKeymapInit(
    TKeymap     *keymap
)
{
    memcpy(keymap, &TKeymapDefault, sizeof(TKeymap));
    return keymap;
}

/*
 * @function TKeymapInitWithFile
 *
 * Clear any mapping present in keymap and replace it with the
 * key-event mappings described in the text file at filepath.
 * See the README.md for documentation of the format, or just
 * look in the example-keymap.txt file.
 *
 * If any error occurs while parsing the information in
 * filepath, an error message is displayed and the program
 * exists with EINVAL.
 *
 * If not every mandatory game engine event is mapped by the
 * information in filepath, an error message is displayed and
 * the program exits with EINVAL.
 */
TKeymap* TKeymapInitWithFile(TKeymap *keymap, const char *filepath);

/*
 * @function TKeymapEventForKey
 *
 * Translate an ASCII key value to its TGameEngine event code.
 */
static inline TGameEngineEvent
TKeymapEventForKey(
    TKeymap     *keymap,
    char        key
)
{
    if ( key >= 0x20 && key <= 0x7d ) return keymap->ascii_20_7d[key - 0x20];
    return TGameEngineEventNoOp;
}

/*
 * @function TKeymapKeySummaryForEvent
 *
 * Summarizes the one or two keys that map to the given game
 * engine event.
 *
 * If the event is not mapped, the returned character buffer
 * contains the empty string.
 *
 * If the event maps to a single key, that key symbol (or SPC
 * for character 0x20) is returned in the string.
 *
 * If the event maps to multiple keys, that key symbols (or SPC
 * for character 0x20) are returned in the string.
 *
 * Keys are searched first across the capital letters; then the
 * lowercase letters; and then sequentially across the remainder
 * of the ASCII range 32 - 126.
 */
const char* TKeymapKeySummaryForEvent(TKeymap *keymap, TGameEngineEvent event);

#endif /* __TKEYMAP_H__ */
