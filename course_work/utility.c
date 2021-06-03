#include "utility.h"

inline int get_enemy_num(char player_num)
{
	return (!player_num) ? 1 : 0;
}

inline int get_max(int first, int second)
{
	return (first > second) ? first : second;
}

inline int get_min(int first, int second)
{
	return (first < second) ? first : second;
}

// return true if bad coordinates input
inline char is_coordinates_input_correct(char x, char y)
{
	return !((x < 0 || x > 9) || (y < 0 || y > 9));
}

// return true if bad direction input
inline char is_ship_direction_input_correct(ship_direction pos)
{
	return !(pos != SHIP_DIRECTION_VERTICAL && pos != SHIP_DIRECTION_HORIZONTAL);
}

char is_game_end(client_states state)
{
	return (state == CLIENT_STATE_GAME_WIN || state == CLIENT_STATE_GAME_LOSE) ? 1 : 0;
}

void throw_exeption_and_exit(u_int16_t tcp_socket_fd, char thread_num)
{
	printf("Cant read or send data. Thread number: %i. Shutting down...\n", thread_num);
	perror(NULL);
	close(tcp_socket_fd);
	endwin();
	exit(1);
}

void ship_struct_assignment_operation(ship* first_ship, ship* second_ship)
{
	first_ship->pos_x = second_ship->pos_x;
	first_ship->pos_y = second_ship->pos_y;

	first_ship->ship_dir = second_ship->ship_dir;
	first_ship->ship_ctg = second_ship->ship_ctg;
}

void shot_struct_assignment_operation(client_shot* first_shot, client_shot* second_shot)
{
	first_shot->pos_x = second_shot->pos_x;
	first_shot->pos_y = second_shot->pos_y;
	first_shot->state = second_shot->state;
}

char get_random_char_from_range(char min, char max)
{
	if (!min)
		return rand() % (max + 1);
	return min + rand() + (max - min + 1);
}