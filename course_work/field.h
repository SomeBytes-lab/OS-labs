#ifndef FIELD_H
#define FIELD_H

#include "stdio.h"
#include "stdlib.h"
#include "string.h"
#include "define.h"
#include "utility.h"

/* create \ destroy functions */

game_field* game_field_create(char create_ships_array);

game_field* game_field_destroy(game_field* field);

/* game functions */

void game_field_set_shot(game_field* field, client_shot* shot);

void game_field_ship_destroy(game_field* field, ship* shp);

void game_field_place_ship_to_field(game_field* field, ship* ship_to_place);

char game_field_check_coordinates_to_hit(game_field* field, char x, char y);

char game_field_is_can_place_ship(game_field* field, ship* ship_to_place);

cell_state game_field_get_hit_result(game_field* field, client_shot* shot);

char game_field_get_hit_ship_num_by_coord(ship* ships, client_shot* shot);

char game_field_check_ship_destroy(game_field* field, ship* check_ship);

#endif