#include "computer.h"
#include "define.h"
#include "field.h"
#include "utility.h"
#include <stdlib.h>

char alg_step = 3;
char alg_pos_x = 0;
char alg_pos_y = 0;

hit_direction computer_get_new_hit_direction(game_field* field, computer_shot* first_shot)
{
	if (first_shot->pos_x - 1 >= 0 && field->field[first_shot->pos_x - 1][first_shot->pos_y] != CELL_STATE_HIT)
		return HIT_DIRECTION_LEFT;
	if (first_shot->pos_y - 1 >= 0 && field->field[first_shot->pos_x][first_shot->pos_y - 1] != CELL_STATE_HIT)
		return HIT_DIRECTION_UP;
	
	if (first_shot->pos_x + 1 <= FIELD_SIZE - 1 && field->field[first_shot->pos_x + 1][first_shot->pos_y] != CELL_STATE_HIT)
		return HIT_DIRECTION_RIGHT;
	if (first_shot->pos_y + 1 <= FIELD_SIZE - 1 && field->field[first_shot->pos_x][first_shot->pos_y + 1] != CELL_STATE_HIT)
		return HIT_DIRECTION_DOWN;
	
	return HIT_DIRECTION_UNDERFINED; 
}

char computer_get_next_shot_by_direction(computer_shot* current, hit_direction hit_dir)
{
	if (hit_dir == HIT_DIRECTION_LEFT && current->pos_x - 1 >= 0)
	{
		current->pos_x = current->pos_x - 1;
		return 0;
	}
	
	if (hit_dir == HIT_DIRECTION_UP && current->pos_y - 1 >= 0)
	{
		current->pos_y = current->pos_y - 1;
		return 0;
	}

	if (hit_dir == HIT_DIRECTION_RIGHT && current->pos_x + 1 <= FIELD_SIZE - 1)
	{
		current->pos_x = current->pos_x + 1;
		return 0;
	}

	if (hit_dir == HIT_DIRECTION_DOWN && current->pos_y + 1 <= FIELD_SIZE - 1)
	{
		current->pos_y = current->pos_y + 1;
		return 0;
	}

	return 1;
}

hit_direction computer_get_opposite_hit_direction(hit_direction dir)
{
	if (dir == HIT_DIRECTION_UP)
		return HIT_DIRECTION_DOWN;

	if (dir == HIT_DIRECTION_LEFT)
		return HIT_DIRECTION_RIGHT;

	if (dir == HIT_DIRECTION_DOWN)
		return HIT_DIRECTION_UP;

	if (dir == HIT_DIRECTION_RIGHT)
		return HIT_DIRECTION_LEFT;
	
	return HIT_DIRECTION_UNDERFINED;
}

ship_category computer_update_higher_ship_tier_stand(ship_category current, ship_category ctg)
{
	if (current == ctg)
	{
		if (current == BATTLESHIP_CATEGORY)
			return CRUISER_CATEGORY;

		if (current == CRUISER_CATEGORY)
			return DESTROYER_CATEGORY;

		if (current == DESTROYER_CATEGORY)
			return TORPEDO_BOAT_CATEGORY;
	}

	return BATTLESHIP_CATEGORY;
}

char computer_get_free_cells(game_field* field)
{
	char free_cells_count = 0;
	for (int i = 0; i < FIELD_SIZE; i++)
		for (int j = 0; j < FIELD_SIZE; j++)
			if (!game_field_check_coordinates_to_hit(field, i, j))
				free_cells_count++;
	return free_cells_count;
}

void computer_fill_array_linear_coords(game_field* field, computer_shot* shots, char gen_shots_count)
{
	for (int i = 0; i < FIELD_SIZE; i++)
	{
		for (int j = 0; j < FIELD_SIZE; j++)
		{
			if (game_field_check_coordinates_to_hit(field, j, i))
				continue;

			shots[gen_shots_count].pos_x = j;
			shots[gen_shots_count].pos_y = i;
		
			if (gen_shots_count < SHOT_ARRAY_SIZE - 1)
				gen_shots_count++;
			else
				break;						
		}
	}
}

void computer_gen_new_shots(game_field* field, computer_shot* shots, char gen_shots_count)
{
	if (computer_array_set == RANDOM_ARRAY_SET)
	{
		if (field->ships_cells_count < 4)
			computer_fill_array_linear_coords(field, shots, gen_shots_count);
		else
		{
			while (gen_shots_count < SHOT_ARRAY_SIZE - 1)
			{
				shots[gen_shots_count].pos_x = get_random_char_from_range(0, 9);
				shots[gen_shots_count].pos_y = get_random_char_from_range(0, 9);

				if (game_field_check_coordinates_to_hit(field, shots[gen_shots_count].pos_x, shots[gen_shots_count].pos_y))
					continue;
				
				gen_shots_count++;
			}
		}
	}
	else if (computer_array_set == SHOTS_STEPS_SET)
	{
		char free_cells_count = computer_get_free_cells(field);

		char pos_x = alg_pos_x;
		char pos_y = alg_pos_y;
		
		char iteration_count = 0;
		char iteration_total = (free_cells_count < FIELD_SIZE) ? free_cells_count : FIELD_SIZE;

		char can_change_alg_pos = 0;

		if (field->field[alg_pos_x][alg_pos_y] != CELL_STATE_SHIP_HIT)
			can_change_alg_pos = 1;

		while (iteration_count < iteration_total && gen_shots_count < SHOT_ARRAY_SIZE - 1)
		{
			if (pos_y + alg_step > FIELD_SIZE - 1)
			{
				pos_x++;
				if (pos_x > FIELD_SIZE - 1)
				{
					pos_x = 0;
					alg_step--;
				}

				pos_y = abs(pos_y + alg_step - FIELD_SIZE);
			}
			else
				pos_y += alg_step;

			if (alg_step <= 0)
			{
				computer_fill_array_linear_coords(field, shots, gen_shots_count);
				break;
			}

			if (game_field_check_coordinates_to_hit(field, pos_x, pos_y))
				continue;

			if (field->field[pos_x][pos_y] == CELL_STATE_SHIP && can_change_alg_pos)
			{
				alg_pos_x = pos_x;
				alg_pos_y = pos_y;
				can_change_alg_pos = 0;
			}

			shots[gen_shots_count].pos_x = pos_x;
			shots[gen_shots_count].pos_y = pos_y;
						
			iteration_count++;
			gen_shots_count++;
		}
	}
	else if (computer_array_set == CHESS_ARRAY_SET)
	{
		char pos_x = 0;
		char pos_y = 0;

		char can_change_alg_pos = 0;

		if (field->field[alg_pos_x][alg_pos_y] != CELL_STATE_SHIP_HIT)
			can_change_alg_pos = 1;

		for (int i = 0; i < FIELD_SIZE; i++)
		{			
			for (int j = 0; j < FIELD_SIZE; j++)
			{
				if ((i % 2 == 0 && j % 2 == 0) ||
					(i % 2 != 0 && j % 2 != 0))
				{
					pos_x = j;
					pos_y = i;
				}
				else
					continue;
				
				if (game_field_check_coordinates_to_hit(field, pos_x, pos_y))
					continue;
				
				shots[gen_shots_count].pos_x = pos_x;
				shots[gen_shots_count].pos_y = pos_y;

				if (field->field[i][j] == CELL_STATE_SHIP && can_change_alg_pos)
				{
					alg_pos_x = pos_x;
					alg_pos_y = pos_y;
					can_change_alg_pos = 0;
				}

				if (gen_shots_count < SHOT_ARRAY_SIZE - 1)
					gen_shots_count++;
				else
					break;
			}
		}

		if (gen_shots_count < SHOT_ARRAY_SIZE - 1)
			computer_fill_array_linear_coords(field, shots, gen_shots_count);
	}
	else
	{
		char free_cells_count = computer_get_free_cells(field);
		char temp_field[FIELD_SIZE][FIELD_SIZE];

		for (int i = 0; i < FIELD_SIZE; i++)
			for (int j = 0; j < FIELD_SIZE; j++)
				temp_field[i][j] = field->field[i][j];

		while (free_cells_count && gen_shots_count < SHOT_ARRAY_SIZE - 1)
		{
			shots[gen_shots_count].pos_x = get_random_char_from_range(0, 9);
			shots[gen_shots_count].pos_y = get_random_char_from_range(0, 9);

			if (temp_field[shots[gen_shots_count].pos_x][shots[gen_shots_count].pos_y] == CELL_STATE_HIT ||
				temp_field[shots[gen_shots_count].pos_x][shots[gen_shots_count].pos_y] == CELL_STATE_SHIP_DESTROY ||
				temp_field[shots[gen_shots_count].pos_x][shots[gen_shots_count].pos_y] == CELL_STATE_SHIP_HIT)
				continue;

			temp_field[shots[gen_shots_count].pos_x][shots[gen_shots_count].pos_y] = CELL_STATE_HIT;			

			gen_shots_count++;
			free_cells_count--;
		}
	}
}
