
#ifndef __TUI_WINDOW_H__
#define __TUI_WINDOW_H__

#include "tetrominotris_config.h"

/*
 * @enum TUI window options
 *
 * Options that control the construction and presentation
 * of a TUI window.
 *
 * - tui_window_opts_title_align_left: horizontal title alignment, left
 * - tui_window_opts_title_align_center: horizontal title alignment, center
 * - tui_window_opts_title_align_right: horizontal title alignment, right
 * - tui_window_opts_title_align_top: vertical title alignment, top of window
 * - tui_window_opts_title_align_bottom: vertical title alignment, bottom of window
 * - tui_window_opts_enable_scroll: allow scrolling behavior for window
 * - tui_window_opts_disable_box: do not display a frame around the window
 */
enum {
    tui_window_opts_title_align_left = 0,
    tui_window_opts_title_align_center = 1,
    tui_window_opts_title_align_right = 2,
    tui_window_opts_title_align_horiz = 0x3,
    //
    tui_window_opts_title_align_top = 0,
    tui_window_opts_title_align_bottom = 1 << 2,
    tui_window_opts_title_align_vert = 0x4,
    //
    tui_window_opts_enable_scroll = 1 << 3,
    tui_window_opts_disable_box = 1 << 4
};

/*
 * @typedef tui_window_opts_t
 *
 * The type of a bit vector of TUI window options.
 */
typedef unsigned int tui_window_opts_t;

/*
 * @typedef tui_window_ref
 *
 * Opaque reference to a TUI window object.
 */
typedef struct tui_window * tui_window_ref;

/*
 * @typedef tui_window_refresh_callback_t
 *
 * The type of a function that can automatically be called to
 * refresh the contents of a TUI window.
 *
 * The context is an opaque pointer provided when the refresh
 * function was registered with a TUI window.  It allows external
 * data to be communicated to the refresh function.
 */
typedef void (*tui_window_refresh_callback_t)(tui_window_ref the_window, WINDOW *window_ptr, const void *context);

/*
 * @function tui_window_alloc
 *
 * Create a new TUI window of dimensions [w]idth by [h]eight with its
 * top-left corner at (x,y).  The window will have an optional title and
 * presentation controlled by opts.
 *
 * If title_len is zero (0), the strlen() function is used to determine
 * the length of the title string to display.  Regardless, if the title
 * length exceeds the width of the window, the text will be truncated at
 * the appropriate number of characters.
 *
 * The refresh_fn will be called to draw the window contents.  The
 * refresh_context is an opaque pointer that will be passed to the
 * refresh_fn; it can be used to communicate external data to the
 * function.
 */
tui_window_ref tui_window_alloc(int x, int y, int w, int h,
                    tui_window_opts_t opts,
                    const char *title, int title_len,
                    tui_window_refresh_callback_t refresh_fn, const void *refresh_context
                );

/*
 * @function tui_window_free
 *
 * Destroy the TUI window associated with the_window and dispose of
 * all memory used by it.
 */
void tui_window_free(tui_window_ref the_window);

/*
 * @function tui_window_refresh
 *
 * Refresh the TUI window associated with the_window by calling its
 * refresh callback and then drawing applicable window structures on
 * top (e.g. frame, title).
 *
 * If should_defer_update is true, then the update of the on-screen
 * display of the window will wait until doupdate() is called by the
 * consumer.
 */
void tui_window_refresh(tui_window_ref the_window, int should_defer_update);

#endif /* __TUI_WINDOW_H__ */
