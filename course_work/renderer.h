#ifndef RENDERER_H
#define RENDERER_H

#include "define.h"
#include "stdlib.h"
#include "ncurses.h"

typedef WINDOW window;

void ncurses_renderer_init();

void ncurses_renderer_destroy();

void ncurses_update_window_positions_and_colors();

void ncurses_render_field(game_field* field, window* field_window);

void ncurses_render_bottom_bar(client_states state);

void ncurses_render_update(game_field* first_field, game_field* second_field, client_states state);

void ncurses_resize_windows();

int ncurses_get_user_input(char input_stage);

void ncurses_update_player_state(client_states state);

void ncurses_incorrect_coord_input_msg();

/* =========================================================================== */

void native_renderer_update(game_field* first_field, game_field* second_field);

void native_renderer_print_field_row(game_field* field, int row);

void native_renderer_clear();

void native_renderer_print_row_nums();

char native_renderer_get_user_input();

#endif