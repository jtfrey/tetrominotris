/*	tui_window.c
	Copyright (c) 2024, J T Frey
*/

#include "tui_window.h"

typedef struct tui_window {
    WINDOW                          *window_ptr;
    tui_window_rect_t               bounds;
    tui_window_opts_t               opts;
    
    tui_window_refresh_callback_t   refresh_fn;
    const void                      *refresh_context;
    
    char                            *title;
    int                             title_len;
} tui_window_t;

//

tui_window_ref
tui_window_alloc(
    tui_window_rect_t bounds,
    tui_window_opts_t opts,
    const char *title, int title_len,
    tui_window_refresh_callback_t refresh_fn, const void *refresh_context
)
{
    tui_window_t    *new_window = NULL;
    int             actual_title_len = title ? (title_len ? title_len : strlen(title)) : 0;
    
    if ( actual_title_len > (bounds.w - 6) ) actual_title_len = bounds.w - 6;
    
    new_window = (tui_window_t*)malloc(sizeof(tui_window_t) + (actual_title_len + 3));
    
    if ( new_window ) {
        new_window->bounds = bounds;
        new_window->opts = opts;
        
        new_window->window_ptr = newwin(bounds.h, bounds.w, bounds.y, bounds.x);
        
        new_window->refresh_fn = refresh_fn;
        new_window->refresh_context = refresh_context;
        
        if ( opts & tui_window_opts_enable_scroll ) scrollok(new_window->window_ptr, TRUE);
        
        if ( actual_title_len ) {
            new_window->title = (void*)new_window + sizeof(tui_window_t);
            new_window->title_len = actual_title_len + 2;
            new_window->title[0] = '[';
            memcpy(new_window->title + 1, title, actual_title_len);
            new_window->title[actual_title_len + 1] = ']';
            new_window->title[actual_title_len + 2] = '\0';
        } else {
            new_window->title = NULL;
        }
    }
    return new_window;
}

//

void
tui_window_free(
    tui_window_ref  the_window
)
{
    werase(the_window->window_ptr);
    delwin(the_window->window_ptr);
    free((void*)the_window);
}

//

void
tui_window_refresh(
    tui_window_ref  the_window,
    int             should_defer_update
)
{
    int             should_not_show_frame = (the_window->opts & tui_window_opts_disable_box) ? TRUE : FALSE;
    
    if ( the_window->refresh_fn ) the_window->refresh_fn(the_window, the_window->window_ptr, the_window->refresh_context);
    box(the_window->window_ptr, should_not_show_frame, should_not_show_frame);
    if ( the_window->title_len ) {
        int             x, y;
        
        switch ( the_window->opts & tui_window_opts_title_align_horiz ) {
            case tui_window_opts_title_align_left:
                x = 2;
                break;
            case tui_window_opts_title_align_center:
                x = the_window->bounds.w / 2 - (1 + the_window->title_len / 2);
                break;
                break;
            case tui_window_opts_title_align_right:
                x = the_window->bounds.w - 2 - the_window->title_len;
                break;
        }
        y = ( the_window->opts & tui_window_opts_title_align_vert ) ? (the_window->bounds.h - 1) : 0;
        mvwprintw(the_window->window_ptr, y, x, the_window->title);
        mvwaddch(the_window->window_ptr, y, x, ACS_RTEE);
        mvwaddch(the_window->window_ptr, y, x + the_window->title_len - 1, ACS_LTEE);
    }
    if ( should_defer_update )
        wnoutrefresh(the_window->window_ptr);
    else
        wrefresh(the_window->window_ptr);
}
