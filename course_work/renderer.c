#include "sys/types.h"
#include "renderer.h"
#include "define.h"
#include "signal.h"
#include <curses.h>

#define WINDOW_GAP_HORIZONTAL 2
#define WINDOW_GAP_VERTICAL 2

#define FIELD_WINDOW_WIDTH 27
#define FIELD_WINDOW_HEIGHT 13

#define BOTTOM_WINDOW_WIDTH 56
#define BOTTOM_WINDOW_HEIGHT 6

/* NCURSES RENDERER */

typedef struct ncurses_window_info_struct
{
	int window_pos_x;
	int window_pos_y;

	window* window_ptr;	
} ncurses_window_info;

ncurses_window_info first_field_window;
ncurses_window_info second_field_window;
ncurses_window_info bottom_window_bar;

void ncurses_update_window_positions_and_colors()
{
	first_field_window.window_pos_x = COLS / 2 - FIELD_WINDOW_WIDTH;
	first_field_window.window_pos_y = LINES / 2 - WINDOW_GAP_VERTICAL - 1 - FIELD_WINDOW_HEIGHT / 2;
	bottom_window_bar.window_pos_x = first_field_window.window_pos_x;

	second_field_window.window_pos_x = COLS / 2 + WINDOW_GAP_HORIZONTAL;
	second_field_window.window_pos_y = first_field_window.window_pos_y;
	bottom_window_bar.window_pos_y = first_field_window.window_pos_y + FIELD_WINDOW_HEIGHT + 1;

	first_field_window.window_ptr = newwin(FIELD_WINDOW_HEIGHT, FIELD_WINDOW_WIDTH, first_field_window.window_pos_y, first_field_window.window_pos_x);
	second_field_window.window_ptr = newwin(FIELD_WINDOW_HEIGHT, FIELD_WINDOW_WIDTH, second_field_window.window_pos_y, second_field_window.window_pos_x);
	bottom_window_bar.window_ptr = newwin(BOTTOM_WINDOW_HEIGHT, BOTTOM_WINDOW_WIDTH, bottom_window_bar.window_pos_y, bottom_window_bar.window_pos_x);

	wbkgd(first_field_window.window_ptr, COLOR_PAIR(1));
	wbkgd(second_field_window.window_ptr, COLOR_PAIR(1));
	wbkgd(bottom_window_bar.window_ptr, COLOR_PAIR(1));

	wclear(first_field_window.window_ptr);
	wclear(second_field_window.window_ptr);
	wclear(bottom_window_bar.window_ptr);

	bkgd(COLOR_PAIR(2));
}

void ncurses_renderer_init()
{
	initscr();
	refresh();
	start_color();

	init_pair(1, COLOR_BLACK, COLOR_WHITE);
	init_pair(2, COLOR_WHITE, COLOR_BLUE);

	ncurses_update_window_positions_and_colors();

	wrefresh(first_field_window.window_ptr);
	wrefresh(first_field_window.window_ptr);
	wrefresh(bottom_window_bar.window_ptr);
}

void ncurses_render_field(game_field* field, window* field_window)
{
	attron(COLOR_PAIR(1));
	wmove(field_window, 1, 4);
	for (int i = 0; i < FIELD_SIZE; i++)
		wprintw(field_window, "|%i", i);
	wprintw(field_window, "|");

	for (int i = 0; i < FIELD_SIZE; i++)
	{
		wmove(field_window, i + 2, 2);
		wprintw(field_window, "%c ", (i + 'a'));
		for (int j = 0; j < FIELD_SIZE; j++)
			wprintw(field_window, "|%c", field->field[j][i]);
		wprintw(field_window, "|\n");	
	}

	box(field_window, 0, 0);
	attroff(COLOR_PAIR(1));
}

void ncurses_render_bottom_bar(client_states state)
{
	box(bottom_window_bar.window_ptr, 0, 0);
	
	wmove(bottom_window_bar.window_ptr, 1, 1);
	wprintw(bottom_window_bar.window_ptr, "Input bars block");

	wmove(bottom_window_bar.window_ptr, 2, 1);
	wprintw(bottom_window_bar.window_ptr, "Position x: ");
	
	wmove(bottom_window_bar.window_ptr, 3, 1);
	wprintw(bottom_window_bar.window_ptr, "Position y: ");

	if (state == CLIENT_STATE_PLACING_SHIPS)
	{
		wmove(bottom_window_bar.window_ptr, 4, 1);
		wprintw(bottom_window_bar.window_ptr, "Ship direc: ");
	}

	wmove(bottom_window_bar.window_ptr, 0, 17);
	waddch(bottom_window_bar.window_ptr, ACS_TTEE);

	wmove(bottom_window_bar.window_ptr, BOTTOM_WINDOW_HEIGHT - 1, 17);
	waddch(bottom_window_bar.window_ptr, ACS_BTEE);

	for (int i = 1; i < BOTTOM_WINDOW_HEIGHT - 1; i++)
	{
		wmove(bottom_window_bar.window_ptr, i, 17);
		waddch(bottom_window_bar.window_ptr, ACS_VLINE);
	}

	wmove(bottom_window_bar.window_ptr, 1, (BOTTOM_WINDOW_WIDTH) / 2 + 1);
	wprintw(bottom_window_bar.window_ptr, "Game information");
}

void ncurses_incorrect_coord_input_msg()
{
	wmove(bottom_window_bar.window_ptr, 3, 20);
	wprintw(bottom_window_bar.window_ptr, "Incorrect coordinates! Try again!");
	wrefresh(bottom_window_bar.window_ptr);
	getch();
}

void ncurses_render_update(game_field* first_field, game_field* second_field, client_states state)
{
	clear();
	refresh();

	wclear(bottom_window_bar.window_ptr);

	ncurses_render_field(first_field, first_field_window.window_ptr);
	ncurses_render_field(second_field, second_field_window.window_ptr);
	ncurses_render_bottom_bar(state);

	ncurses_update_player_state(state);

	wrefresh(first_field_window.window_ptr);
	wrefresh(second_field_window.window_ptr);
	wrefresh(bottom_window_bar.window_ptr);
}

void ncurses_resize_windows()
{
	delwin(first_field_window.window_ptr);
	delwin(second_field_window.window_ptr);

	ncurses_update_window_positions_and_colors();
}

int ncurses_get_user_input(char input_stage)
{
	if (input_stage == 0)
		wmove(bottom_window_bar.window_ptr, 2, 14);
	else if (input_stage == 1)
		wmove(bottom_window_bar.window_ptr, 3, 14);
	else if (input_stage == 2)
		wmove(bottom_window_bar.window_ptr, 4, 14);

	int key_input = wgetch(bottom_window_bar.window_ptr);
	if (key_input == KEY_RESIZE)
		ncurses_resize_windows();
	else if (key_input == 27)
		kill(0, SIGINT);
	
	return key_input;
}

void ncurses_update_player_state(client_states state)
{
	wmove(bottom_window_bar.window_ptr, 2, 20);
	if (state == CLIENT_STATE_MAKE_TURN)
		wprintw(bottom_window_bar.window_ptr, "Your turn");
	else if (state == CLIENT_STATE_WAIT_TURN)
		wprintw(bottom_window_bar.window_ptr, "Wait enemy turn");
	else if (state == CLIENT_STATE_PLACING_SHIPS)
		wprintw(bottom_window_bar.window_ptr, "Placing ships");
	else if (state == CLIENT_STATE_WAITING_ANOTHER_PLAYER)
		wprintw(bottom_window_bar.window_ptr, "Waiting another player");
}

void ncurses_renderer_destroy()
{
	delwin(first_field_window.window_ptr);
	delwin(second_field_window.window_ptr);
	endwin();
}

/* NCURSES RENDERER END */

/* render functions */

void native_renderer_update(game_field* first_field, game_field* second_field)
{
	native_renderer_print_row_nums();

	for (int i = 0; i < FIELD_SIZE; i++)
	{
		native_renderer_print_field_row(first_field, i);
		printf("  +  ");
		native_renderer_print_field_row(second_field, i);
		printf("\n");
	}
}

void native_renderer_print_field_row(game_field* field, int row)
{
	printf("%c ", (row + 'a'));
	for (int i = 0; i < FIELD_SIZE; i++)
		printf("|%c", field->field[i][row]);
	printf("|");
}

void native_renderer_print_row_nums()
{
	printf("  ");
	
	for (int i = 0; i < FIELD_SIZE; i++)
		printf("|%i", i);
	
	printf("|  +    ");

	for (int i = 0; i < FIELD_SIZE; i++)
		printf("|%i", i);
	
	printf("|\n");
}

char native_renderer_get_user_input()
{
	char pressed_key;
	scanf(" %c", &pressed_key);
	return pressed_key;
}

void native_renderer_clear()
{
	system("clear");
}