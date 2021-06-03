#include "field.h"
#include "define.h"
#include "stdlib.h"

/* create \ destroy functions */

game_field* game_field_create(char create_ships_array)
{
	game_field* field = malloc(sizeof(game_field));
	memset(field->field, CELL_STATE_VOID, sizeof(field->field));
	field->ships_cells_count = 0;

	if (create_ships_array)
		field->ships_info = malloc(SHIPS_COUNT * sizeof(ship));
	else
		field->ships_info = NULL;

	return field;
}

game_field* game_field_destroy(game_field* field)
{
	if (field->ships_info)
		free(field->ships_info);
	free(field);
	return NULL;
}

/* game functions */

void game_field_set_shot(game_field* field, client_shot* shot)
{
	switch (shot->state)
	{
		case CELL_STATE_HIT: field->field[shot->pos_x][shot->pos_y] = CELL_STATE_HIT; break;
		case CELL_STATE_SHIP_DESTROY:
		case CELL_STATE_SHIP_HIT: field->field[shot->pos_x][shot->pos_y] = CELL_STATE_SHIP_HIT; break;
		default: break;
	}
}

void game_field_ship_destroy(game_field* field, ship* shp)
{
	int min_x = get_max(0, shp->pos_x - 1);
	int min_y = get_max(0, shp->pos_y - 1);

	int max_x = get_min(FIELD_SIZE - 1, shp->pos_x + ((shp->ship_dir == SHIP_DIRECTION_VERTICAL) ? 1 : shp->ship_ctg));
	int max_y = get_min(FIELD_SIZE - 1, shp->pos_y + ((shp->ship_dir == SHIP_DIRECTION_VERTICAL) ? shp->ship_ctg : 1));

	for (int i = min_x; i <= max_x; i++)
		for (int j = min_y; j <= max_y; j++)
			field->field[i][j] = CELL_STATE_HIT;
	
	for (int i = 0; i < shp->ship_ctg; i++)
	{
		if (shp->ship_dir == SHIP_DIRECTION_VERTICAL)
			field->field[shp->pos_x][shp->pos_y + i] = CELL_STATE_SHIP_DESTROY;
		else
			field->field[shp->pos_x + i][shp->pos_y] = CELL_STATE_SHIP_DESTROY;
	}
}

void game_field_place_ship_to_field(game_field* field, ship* shp)
{
	for (int i = 0; i < shp->ship_ctg; i++)
	{
		if (shp->ship_dir == SHIP_DIRECTION_VERTICAL)
			field->field[shp->pos_x][shp->pos_y + i] = CELL_STATE_SHIP;
		else
			field->field[shp->pos_x + i][shp->pos_y] = CELL_STATE_SHIP;
	}	
}

char game_field_check_coordinates_to_hit(game_field* field, char x, char y)
{
	if ((x >= 0 && x <= 9) && (y >= 0 && y <= 9))
		if (field->field[x][y] != CELL_STATE_HIT &&
			field->field[x][y] != CELL_STATE_SHIP_HIT &&
			field->field[x][y] != CELL_STATE_SHIP_DESTROY)
			return 0;
	return 1;
}

char game_field_is_can_place_ship(game_field* field, ship* shp)
{
	if (field->field[shp->pos_x][shp->pos_y] == CELL_STATE_SHIP)
		return 0;

	if (shp->ship_dir == SHIP_DIRECTION_VERTICAL && shp->pos_y + shp->ship_ctg >= FIELD_SIZE + 1)
		return 0;
	
	if (shp->ship_dir == SHIP_DIRECTION_HORIZONTAL && shp->pos_x + shp->ship_ctg >= FIELD_SIZE + 1)
		return 0;

	int min_x = get_max(0, shp->pos_x - 1);
	int min_y = get_max(0, shp->pos_y - 1);

	int max_x = get_min(FIELD_SIZE - 1, shp->pos_x + ((shp->ship_dir == SHIP_DIRECTION_VERTICAL) ? 1 : shp->ship_ctg));
	int max_y = get_min(FIELD_SIZE - 1, shp->pos_y + ((shp->ship_dir == SHIP_DIRECTION_VERTICAL) ? shp->ship_ctg : 1));

	for (int i = min_x; i <= max_x; i++)
		for (int j = min_y; j <= max_y; j++)
			if (field->field[i][j] == CELL_STATE_SHIP)
				return 0;
	
	return 1;	
}

cell_state game_field_get_hit_result(game_field* field, client_shot* shot)
{
	cell_state result = (field->field[shot->pos_x][shot->pos_y] == CELL_STATE_SHIP) ? CELL_STATE_SHIP_HIT : CELL_STATE_HIT;
	field->field[shot->pos_x][shot->pos_y] = result;
	return result;
}

char game_field_get_hit_ship_num_by_coord(ship* ships, client_shot* shot)
{
	for (int i = 0; i < SHIPS_COUNT; i++)
	{
		if (ships[i].ship_dir == SHIP_DIRECTION_VERTICAL)
		{
			if (shot->pos_x == ships[i].pos_x && shot->pos_y >= ships[i].pos_y && shot->pos_y <= ships[i].pos_y + ships[i].ship_ctg)
				return i;
		}
		else
		{
			if (shot->pos_y == ships[i].pos_y && shot->pos_x >= ships[i].pos_x && shot->pos_x <= ships[i].pos_x + ships[i].ship_ctg)
				return i;
		}
	}

	return -1;
}

char game_field_check_ship_destroy(game_field* field, ship* check_ship)
{
	for (int i = 0; i < check_ship->ship_ctg; i++)
	{
		if (check_ship->ship_dir == SHIP_DIRECTION_VERTICAL)
		{
			if (field->field[check_ship->pos_x][check_ship->pos_y + i] == CELL_STATE_SHIP)
				return 0;
		}
		else
		{
			if (field->field[check_ship->pos_x + i][check_ship->pos_y] == CELL_STATE_SHIP)
				return 0;
		}
	}

	return 1;
}