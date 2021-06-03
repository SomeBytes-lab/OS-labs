#ifndef UTILITY_H
#define UTILITY_H

#include "stdio.h"
#include "stdlib.h"
#include "unistd.h"
#include "define.h"
#include "renderer.h"

int get_enemy_num(char player_num);

int get_max(int first, int second);

int get_min(int first, int second);

char is_coordinates_input_correct(char x, char y);

char is_ship_direction_input_correct(ship_direction pos);

char is_game_end(client_states state);

void throw_exeption_and_exit(u_int16_t tcp_socket_fd, char thread_num);

void ship_struct_assignment_operation(ship* first_ship, ship* second_ship);

void shot_struct_assignment_operation(client_shot* first_shot, client_shot* second_shot);

char get_random_char_from_range(char min, char max);

#endif