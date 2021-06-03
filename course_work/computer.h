#ifndef COMPUTER_H
#define COMPUTER_H

#include "field.h"
#include "define.h"
#include "utility.h"

extern char computer_array_set;

typedef enum hit_direction_enum
{
	HIT_DIRECTION_UNDERFINED = 1,
	HIT_DIRECTION_UP,
	HIT_DIRECTION_DOWN,
	HIT_DIRECTION_LEFT,
	HIT_DIRECTION_RIGHT
} hit_direction;

hit_direction computer_get_opposite_hit_direction(hit_direction dir);

hit_direction computer_get_new_hit_direction(game_field* field, computer_shot* first_shot);

ship_category computer_update_higher_ship_tier_stand(ship_category current, ship_category ctg);

char computer_get_next_shot_by_direction(computer_shot* current, hit_direction hit_dir);

void computer_gen_new_shots(game_field* field, computer_shot* shots, char gen_shots_count);

#endif