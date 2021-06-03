#include "time.h"
#include "stdio.h"
#include "errno.h"
#include "unistd.h"
#include "stdlib.h"
#include "string.h"
#include "signal.h"
#include "strings.h"
#include "sys/sem.h"
#include "sys/ipc.h"
#include "pthread.h"
#include "arpa/inet.h"
#include "sys/types.h"
#include "sys/socket.h"
#include "netinet/in.h"

#include "field.h"
#include "define.h"
#include "utility.h"
#include "computer.h"
#include "renderer.h"

#define SELF_SHIPS 0
#define RANDOM_SHIPS 1
#define FILE_SHIPS 2

int sem_id;

char ship_place_mod;
char computer_array_set;

ship ship_destroyed;
client_shot global_shot;

void signal_handler(int nsig)
{
	if (nsig == SIGINT)
	{
		ncurses_renderer_destroy();
		semctl(sem_id, 0, IPC_RMID, 0);
		exit(0);
	}
}

typedef struct thread_player_info_struct
{
	char player_num;

	game_field* own_field;
	game_field* enemy_field;

	client_states* own_state;
	client_states* enemy_state;
} thread_player_info;

void sem_set_state(unsigned short sem_num, short state)
{
	struct sembuf op;
	op.sem_op = state;
	op.sem_flg = 0;
	op.sem_num = sem_num;
	semop(sem_id, &op, 1);	
}

void print_programm_use_and_exit()
{
	printf("Use: ./client <args>\n");
	
	printf("Gamemodes options:\n");
	printf("\t<nothing> -- player versus computer gamemode\n");
	printf("\t--computers -- computer versus computer gamemode\n");
	printf("\t--connect <ip-address> <port> -- player versus player gamemode\n\n");
	
	printf("Algorithms options:\n");
	printf("\t--alg <algorithm name>\n");
	printf("\t<nothing> -- set random algoritm by default\n");
	printf("\trandom -- set by default\n");
	printf("\tsteps -- to set colm steps algorithm\n");
	printf("\tgarant -- to set garant algorithm\n");
	printf("\tchess -- to set chess-board algorithm\n\n");
	
	printf("Ship placed options:\n");
	printf("\t<nothing> -- self input place\n");
	printf("\t--random -- place random ships to the field\n");
	printf("\t--file <file_name> -- place ships from file\n");
	exit(1);
}

char select_next_ship(int* placed_ships_category_count, ship* shp)
{
	*placed_ships_category_count = *placed_ships_category_count + 1;
	switch (shp->ship_ctg)
	{
		case BATTLESHIP_CATEGORY:
			shp->ship_ctg = CRUISER_CATEGORY;
			*placed_ships_category_count = 0;
			break;
		
		case CRUISER_CATEGORY:
			if (*placed_ships_category_count == CRUISER_SHIP_COUNT)
			{
				shp->ship_ctg = DESTROYER_CATEGORY;
				*placed_ships_category_count = 0;
			}
			break;
		
		case DESTROYER_CATEGORY:
			if (*placed_ships_category_count == DESTROYER_SHIP_COUNT)
			{
				shp->ship_ctg = TORPEDO_BOAT_CATEGORY;
				*placed_ships_category_count = 0;
			}
			break;
		
		case TORPEDO_BOAT_CATEGORY:
			if (*placed_ships_category_count == TORPEDO_SHIP_COUNT)
			{
				*placed_ships_category_count = 0;
				shp->ship_ctg = BATTLESHIP_CATEGORY;
				return 1;
			}
			break;

		default:
			shp->ship_ctg = BATTLESHIP_CATEGORY;
			break;
	}
	
	return 0;
}

void get_random_place_ship(ship* shp)
{
	shp->pos_x = get_random_char_from_range(0, 9);
	shp->pos_y = get_random_char_from_range(0, 9);
	shp->ship_dir = (get_random_char_from_range(0, 1)) ? SHIP_DIRECTION_VERTICAL : SHIP_DIRECTION_HORIZONTAL;
}

void place_random_ships_to_field(game_field* field)
{
	ship ship_to_place;
	char is_all_ship_placed = 0;

	int placed_ships_count = 0;
	int placed_ships_category_count = 0;

	ship_to_place.ship_ctg = BATTLESHIP_CATEGORY;

	while (!is_all_ship_placed)
	{
		get_random_place_ship(&ship_to_place);

		if (!game_field_is_can_place_ship(field, &ship_to_place))
			continue;
		
		game_field_place_ship_to_field(field, &ship_to_place);

		ship_struct_assignment_operation(&field->ships_info[placed_ships_count], &ship_to_place);
		is_all_ship_placed = select_next_ship(&placed_ships_category_count, &ship_to_place);
		
		placed_ships_count++;
	}
}

char get_next_char_from_file(FILE* file)
{
	int ch;
	while (ch != EOF)
	{
		ch = fgetc(file);

		if (ch == '\r' || ch == '\n' || ch == ' ')
			continue;
		else
			break;
	}

	if (ch == EOF)
	{
		printf("Error! Bad input from file! Fix the file and try again!\n");
		exit(1);
	}

	return ch;
}

int read_ships_positions_from_file(game_field* field, const char* file_name)
{
	FILE* file = fopen(file_name, "r");

	if (!file)
		return 0;

	ship placed_ship;
		
	int placed_ships_count = 0;
	int placed_ships_category_count = 0;

	char is_all_ship_placed = select_next_ship(&placed_ships_category_count, &placed_ship);

	while (!is_all_ship_placed)
	{
		placed_ship.pos_x = get_next_char_from_file(file) - '0';
		placed_ship.pos_y = get_next_char_from_file(file) - 'a';
		placed_ship.ship_dir = get_next_char_from_file(file);

		if (!is_coordinates_input_correct(placed_ship.pos_x, placed_ship.pos_y))
		{
			printf("Incorrect coordinates in file! Line %i!\n", placed_ships_count + 1);
			return 0;
		}

		if (!is_ship_direction_input_correct(placed_ship.ship_dir))
		{
			printf("Incorrect ship direction in file! Line %i!\n", placed_ships_count + 1);
			return 0;
		}

		if (!game_field_is_can_place_ship(field, &placed_ship))
		{
			printf("Cant place this ship from file! Line %i!\n", placed_ships_count + 1);
			return 0;
		}

		game_field_place_ship_to_field(field, &placed_ship);
		ship_struct_assignment_operation(&field->ships_info[placed_ships_count], &placed_ship);

		is_all_ship_placed = select_next_ship(&placed_ships_category_count, &placed_ship);
		placed_ships_count++;
	}

	fclose(file);

	return 1;
}

void clear_bad_ship_from_fields(game_field* field, ship* shp)
{
	for (int i = 0; i < shp->ship_ctg; i++)
	{
		if (shp->ship_dir == SHIP_DIRECTION_VERTICAL)
			field->field[shp->pos_x][shp->pos_y + i] = CELL_STATE_VOID;
		else
			field->field[shp->pos_x + i][shp->pos_y] = CELL_STATE_VOID;
	}	
}


/* ========================================================================= */
/* ========================================================================= */
/* ========================================================================= */







void* computer_player_pthread_function(void* thread_arguments)
{
	thread_player_info* player_info = (thread_player_info*) thread_arguments;

	int random_counter = 0;

	char ship_was_hit = 0;
	char need_regen_array = 0;
	char direction_is_right = 0;
	char current_shot_from_array = 0;

	computer_shot first_hit_shot;
	computer_shot shots_array[SHOT_ARRAY_SIZE];

	hit_direction hit_dir = HIT_DIRECTION_UNDERFINED;
	ship_category higher_tier_ship_stand = BATTLESHIP_CATEGORY;
		
	place_random_ships_to_field(player_info->own_field);
	computer_gen_new_shots(player_info->enemy_field, shots_array, 0);

	sem_set_state(get_enemy_num(player_info->player_num), SEMAPHORE_GIVE_ONE);
	sem_set_state(player_info->player_num, SEMAPHORE_REDUCE_ONE);

	while (!is_game_end((*player_info->own_state)))
	{
		client_shot shot;

		if ((*player_info->own_state) == CLIENT_STATE_WAIT_TURN)
		{
			if (need_regen_array || current_shot_from_array >= SHOT_ARRAY_SIZE - 1)
			{
				char gen_shots_count = 0;					

				if (ship_was_hit)
				{
					shots_array[gen_shots_count].pos_x = first_hit_shot.pos_x;
					shots_array[gen_shots_count].pos_y = first_hit_shot.pos_y;

					if (hit_dir == HIT_DIRECTION_UNDERFINED)
						hit_dir = computer_get_new_hit_direction(player_info->enemy_field, &first_hit_shot);

					for (int i = 0; i < higher_tier_ship_stand; i++)
					{
						if (computer_get_next_shot_by_direction(&shots_array[gen_shots_count], hit_dir))
							continue;

						shots_array[gen_shots_count + 1].pos_x = shots_array[gen_shots_count].pos_x;
						shots_array[gen_shots_count + 1].pos_y = shots_array[gen_shots_count].pos_y;

						gen_shots_count++;
					}					
				}

				computer_gen_new_shots(player_info->enemy_field, shots_array, gen_shots_count);

				need_regen_array = 0;
				current_shot_from_array = 0;
			}

			sem_set_state(player_info->player_num, SEMAPHORE_REDUCE_ONE);

			game_field_set_shot(player_info->own_field, &global_shot);
			if (global_shot.state == CELL_STATE_SHIP_DESTROY)
				game_field_ship_destroy(player_info->own_field, &ship_destroyed);

			if ((*player_info->own_state) == CLIENT_STATE_GAME_LOSE)
				break;
			
			sem_set_state(get_enemy_num(player_info->player_num), SEMAPHORE_GIVE_ONE);
		}
		else
		{
			if (need_regen_array && ship_was_hit)
			{
				if (hit_dir == HIT_DIRECTION_UNDERFINED)
					hit_dir = computer_get_new_hit_direction(player_info->enemy_field, &first_hit_shot);

				computer_shot temp;

				temp.pos_x = shot.pos_x;
				temp.pos_y = shot.pos_y;

				if (computer_get_next_shot_by_direction(&temp, hit_dir))
				{
					shot.pos_x = first_hit_shot.pos_x;
					shot.pos_y = first_hit_shot.pos_y;

					hit_dir = computer_get_opposite_hit_direction(hit_dir);
					computer_get_next_shot_by_direction(&temp, hit_dir);
				}

				shot.pos_x = temp.pos_x;
				shot.pos_y = temp.pos_y;
			}
			else
			{
				if (current_shot_from_array >= SHOT_ARRAY_SIZE - 1)
				{
					shot.pos_x = get_random_char_from_range(0, 9);
					shot.pos_y = get_random_char_from_range(0, 9);

					need_regen_array = 1;

					if (!game_field_check_coordinates_to_hit(player_info->enemy_field, shot.pos_x, shot.pos_y))
						random_counter++;
				}
				else
				{
					shot.pos_x = shots_array[current_shot_from_array].pos_x;
					shot.pos_y = shots_array[current_shot_from_array].pos_y;

					current_shot_from_array++;
				}
			}

			if (game_field_check_coordinates_to_hit(player_info->enemy_field, shot.pos_x, shot.pos_y))
				continue;
				
			shot.state = game_field_get_hit_result(player_info->enemy_field, &shot);
			game_field_set_shot(player_info->enemy_field, &shot);

			if (shot.state == CELL_STATE_SHIP_HIT)
			{
				if (!ship_was_hit)
				{
					ship_was_hit = 1;

					first_hit_shot.pos_x = shot.pos_x;
					first_hit_shot.pos_y = shot.pos_y;

					need_regen_array = 1;
					hit_dir = HIT_DIRECTION_UNDERFINED;
				}
				else
					direction_is_right = 1;

				char ship_num = game_field_get_hit_ship_num_by_coord(player_info->enemy_field->ships_info, &shot);
				if (game_field_check_ship_destroy(player_info->enemy_field, &player_info->enemy_field->ships_info[ship_num]))
				{
					if (ship_was_hit)
						current_shot_from_array = (int) higher_tier_ship_stand + 1;
					
					ship_was_hit = 0;
					need_regen_array = 1;
					direction_is_right = 0;

					hit_dir = HIT_DIRECTION_UNDERFINED;
					shot.state = CELL_STATE_SHIP_DESTROY;
					
					higher_tier_ship_stand = computer_update_higher_ship_tier_stand(higher_tier_ship_stand, player_info->enemy_field->ships_info[ship_num].ship_ctg);

					ship_struct_assignment_operation(&ship_destroyed, &player_info->enemy_field->ships_info[ship_num]);
					game_field_ship_destroy(player_info->enemy_field, &player_info->enemy_field->ships_info[ship_num]);
				}
			}

			shot_struct_assignment_operation(&global_shot, &shot);
			
			if (shot.state == CELL_STATE_HIT)
			{
				if (ship_was_hit)
				{
					if (!direction_is_right)
						hit_dir = HIT_DIRECTION_UNDERFINED;
					else
						hit_dir = computer_get_opposite_hit_direction(hit_dir);
					
					need_regen_array = 1;
					
					shot.pos_x = first_hit_shot.pos_x;
					shot.pos_y = first_hit_shot.pos_y;
				}

				(*player_info->own_state) = CLIENT_STATE_WAIT_TURN;
				(*player_info->enemy_state) = CLIENT_STATE_MAKE_TURN;
			}
			else
			{
				player_info->enemy_field->ships_cells_count--;
				if (player_info->enemy_field->ships_cells_count == 0)
				{
					(*player_info->own_state) = CLIENT_STATE_GAME_WIN;
					(*player_info->enemy_state) = CLIENT_STATE_GAME_LOSE;
				}
			}

			sem_set_state(get_enemy_num(player_info->player_num), SEMAPHORE_GIVE_ONE);
			
			if ((*player_info->own_state) == CLIENT_STATE_GAME_WIN)
				break;			
			
			sem_set_state(player_info->player_num, SEMAPHORE_REDUCE_ONE);

		}
	}

	printf("(Player %i) Random shots: %i\n", (player_info->player_num + 1), random_counter);

	return 0;
}





/* ========================================================================= */
/* ========================================================================= */
/* ========================================================================= */





void* player_pthread_function(void* thread_arguments)
{
	thread_player_info* player_info = (thread_player_info*) thread_arguments;
	game_field* computer_field = game_field_create(CREATE_WITHOUT_SHIPS_ARRAY);

	ncurses_renderer_init();

	if (!ship_place_mod)
	{
		ship place_msg;
		
		int placed_ships_count = 0;
		int placed_ships_category_count = 0;

		char is_all_ship_placed = select_next_ship(&placed_ships_category_count, &place_msg);

		while (!is_all_ship_placed)
		{
			ncurses_render_update(player_info->own_field, computer_field, CLIENT_STATE_PLACING_SHIPS);

			place_msg.pos_x = ncurses_get_user_input(0) - '0';
			place_msg.pos_y = ncurses_get_user_input(1) - 'a';

			if (!is_coordinates_input_correct(place_msg.pos_x, place_msg.pos_y))
			{
				ncurses_incorrect_coord_input_msg();
				continue;
			}

			place_msg.ship_dir = (ship_direction) ncurses_get_user_input(2);

			if (!is_ship_direction_input_correct(place_msg.ship_dir))
			{
				ncurses_incorrect_coord_input_msg();
				continue;
			}

			if (!game_field_is_can_place_ship(player_info->own_field, &place_msg))
			{
				ncurses_incorrect_coord_input_msg();
				continue;
			}

			game_field_place_ship_to_field(player_info->own_field, &place_msg);
			ship_struct_assignment_operation(&player_info->own_field->ships_info[placed_ships_count], &place_msg);

			is_all_ship_placed = select_next_ship(&placed_ships_category_count, &place_msg);
			placed_ships_count++;
		}
	}

	sem_set_state(get_enemy_num(player_info->player_num), SEMAPHORE_GIVE_ONE);
	sem_set_state(player_info->player_num, SEMAPHORE_REDUCE_ONE);

	while (!is_game_end((*player_info->own_state)))
	{
		client_shot shot;

		ncurses_render_update(player_info->own_field, computer_field, -1);

		if ((*player_info->own_state) == CLIENT_STATE_WAIT_TURN)
		{
			sem_set_state(player_info->player_num, SEMAPHORE_REDUCE_ONE);

			game_field_set_shot(player_info->own_field, &global_shot);
			
			if (global_shot.state == CELL_STATE_SHIP_DESTROY)
				game_field_ship_destroy(player_info->own_field, &ship_destroyed);

			if ((*player_info->own_state) == CLIENT_STATE_GAME_LOSE)
				break;

			sem_set_state(get_enemy_num(player_info->player_num), SEMAPHORE_GIVE_ONE);
		}
		else
		{
			if (player_info->own_field->ships_cells_count == 0)
				break;

			shot.pos_x = ncurses_get_user_input(0) - '0';
			shot.pos_y = ncurses_get_user_input(1) - 'a';

			if (!is_coordinates_input_correct(shot.pos_x, shot.pos_y))
			{
				ncurses_incorrect_coord_input_msg();
				continue;
			}
				
			if (game_field_check_coordinates_to_hit(player_info->enemy_field, shot.pos_x, shot.pos_y))
			{
				ncurses_incorrect_coord_input_msg();
				continue;
			}

			shot.state = game_field_get_hit_result(player_info->enemy_field, &shot);

			game_field_set_shot(computer_field, &shot);

			if (shot.state == CELL_STATE_SHIP_HIT)
			{
				char ship_num = game_field_get_hit_ship_num_by_coord(player_info->enemy_field->ships_info, &shot);
				if (game_field_check_ship_destroy(player_info->enemy_field, &player_info->enemy_field->ships_info[ship_num]))
				{
					shot.state = CELL_STATE_SHIP_DESTROY;

					ship_struct_assignment_operation(&ship_destroyed, &player_info->enemy_field->ships_info[ship_num]);

					game_field_ship_destroy(computer_field, &player_info->enemy_field->ships_info[ship_num]);
					game_field_ship_destroy(player_info->enemy_field, &player_info->enemy_field->ships_info[ship_num]);
				}
			}

			if (shot.state == CELL_STATE_HIT)
			{
				(*player_info->own_state) = CLIENT_STATE_WAIT_TURN;
				(*player_info->enemy_state) = CLIENT_STATE_MAKE_TURN;
			}
			else
			{
				player_info->enemy_field->ships_cells_count--;
				if (player_info->enemy_field->ships_cells_count == 0)
				{
					(*player_info->own_state) = CLIENT_STATE_GAME_WIN;
					(*player_info->enemy_state) = CLIENT_STATE_GAME_LOSE;
				}
			}

			shot_struct_assignment_operation(&global_shot, &shot);

			sem_set_state(get_enemy_num(player_info->player_num), SEMAPHORE_GIVE_ONE);
			
			if ((*player_info->own_state) == CLIENT_STATE_GAME_WIN)
				break;

			sem_set_state(player_info->player_num, SEMAPHORE_REDUCE_ONE);
		}
	}

	ncurses_renderer_destroy();

	return 0;
}




/* ========================================================================= */
/* ========================================================================= */
/* ========================================================================= */





void player_versus_player(char* server_ip, int server_port, char* file_name)
{
	game_field* own_field = game_field_create(CREATE_WITH_SHIPS_ARRAY);
	game_field* enemy_field = game_field_create(CREATE_WITHOUT_SHIPS_ARRAY);
		
	if (ship_place_mod == RANDOM_SHIPS)
		place_random_ships_to_field(own_field);
	else if (ship_place_mod == FILE_SHIPS)
	{
		if (!read_ships_positions_from_file(own_field, file_name))
		{
			printf("Error! Input file with ships coord is incorrect! Fix the file and try again!\n");
			exit(1);
		}
	}

	u_int16_t tcp_socket_fd = 0;
	
	client_states state;
	server_answer answer;

	socket_address_in server_address;
	socket_address_in client_address;

	bzero(&server_address, sizeof(server_address));
	bzero(&client_address, sizeof(client_address));

	if ((tcp_socket_fd = socket(PF_INET, SOCK_STREAM, 0)) < 0)
	{
		perror(NULL);
		exit(1);
	}

	server_address.sin_family = AF_INET;
	server_address.sin_port = htons(server_port);

	if (inet_aton(server_ip, &server_address.sin_addr) == 0)
		throw_exeption_and_exit(tcp_socket_fd, 0);

	if (connect(tcp_socket_fd, (socket_address*) &server_address, sizeof(server_address)) < 0)
		throw_exeption_and_exit(tcp_socket_fd, 0);

	// Get client state from server

	if (read(tcp_socket_fd, &state, sizeof(client_states)) < 0)
		throw_exeption_and_exit(tcp_socket_fd, 0);

	// Check is all two players ready to game

	if (state == CLIENT_STATE_WAITING_ANOTHER_PLAYER)
	{
		printf("Waiting another player...\n");

		if (read(tcp_socket_fd, &state, sizeof(client_states)) < 0)
			throw_exeption_and_exit(tcp_socket_fd, 0);
	}

	if (state == CLIENT_STATE_PLACING_SHIPS)
	{
		printf("Ready to play! Press any button!\n");
		getchar();
		fflush(stdin);
	}

	if (write(tcp_socket_fd, &ship_place_mod, sizeof(ship_place_mod)) < 0)
		throw_exeption_and_exit(tcp_socket_fd, 0);

	// Game variables

	ncurses_renderer_init();
	ncurses_render_update(own_field, enemy_field, state);

	if (!ship_place_mod)
	{
		ship place_msg;
		int placed_ships_category_count = 0;
		char is_all_ship_placed = select_next_ship(&placed_ships_category_count, &place_msg);

		while (!is_all_ship_placed)
		{
			ncurses_render_update(own_field, enemy_field, state);

			place_msg.pos_x = ncurses_get_user_input(0) - '0';
			place_msg.pos_y = ncurses_get_user_input(1) - 'a';

			if (!is_coordinates_input_correct(place_msg.pos_x, place_msg.pos_y))
			{
				ncurses_incorrect_coord_input_msg();
				continue;
			}

			place_msg.ship_dir = (ship_direction) ncurses_get_user_input(2);

			if (!is_ship_direction_input_correct(place_msg.ship_dir))
			{
				ncurses_incorrect_coord_input_msg();
				continue;
			}

			if (!game_field_is_can_place_ship(own_field, &place_msg))
			{
				ncurses_incorrect_coord_input_msg();
				continue;
			}

			if (write(tcp_socket_fd, &place_msg, sizeof(ship)) <= 0)
				throw_exeption_and_exit(tcp_socket_fd, 0);

			if (read(tcp_socket_fd, &answer, sizeof(server_answer)) <= 0)
				throw_exeption_and_exit(tcp_socket_fd, 0);

			if (answer == SERVER_ERROR)
				continue;

			game_field_place_ship_to_field(own_field, &place_msg);
			
			is_all_ship_placed = select_next_ship(&placed_ships_category_count, &place_msg);
		}
	}
	else
	{
		char placed_ships_count = 0;
		while (placed_ships_count < SHIPS_COUNT)
		{
			if (write(tcp_socket_fd, &own_field->ships_info[placed_ships_count], sizeof(ship)) <= 0)
				throw_exeption_and_exit(tcp_socket_fd, 0);

			if (read(tcp_socket_fd, &answer, sizeof(server_answer)) <= 0)
				throw_exeption_and_exit(tcp_socket_fd, 0);

			if (answer == SERVER_ERROR)
			{
				ship new_ship;

				new_ship.ship_ctg = own_field->ships_info[placed_ships_count].ship_ctg;
				clear_bad_ship_from_fields(own_field, &own_field->ships_info[placed_ships_count]);

				do
				{
					get_random_place_ship(&new_ship);
				} while (!game_field_is_can_place_ship(own_field, &new_ship));

				game_field_place_ship_to_field(own_field, &new_ship);

				continue;
			}

			placed_ships_count++;
		}
	}

	// Send that you are placed all ships and ready to battle
	state = CLIENT_STATE_WAITING_ANOTHER_PLAYER;
	
	ncurses_render_update(own_field, enemy_field, state);
	
	if (write(tcp_socket_fd, &state, sizeof(client_states)) <= 0)
		throw_exeption_and_exit(tcp_socket_fd, 0);

	// Receive from server a player turn
	if (read(tcp_socket_fd, &state, sizeof(client_states)) <= 0)
		throw_exeption_and_exit(tcp_socket_fd, 0);

	// Main cycle

	while (1)
	{
		client_shot shot;
		
		ncurses_render_update(own_field, enemy_field, state);

		if (state == CLIENT_STATE_WAIT_TURN)
		{
			// Get enemy shot;
			
			if (read(tcp_socket_fd, &shot, sizeof(client_shot)) <= 0)
				throw_exeption_and_exit(tcp_socket_fd, 0);	
			
			game_field_set_shot(own_field, &shot);
			
			if (shot.state == CELL_STATE_SHIP_DESTROY)
			{
				ship destroyed_ship_info;
				if (read(tcp_socket_fd, &destroyed_ship_info, sizeof(ship)) <= 0)
					throw_exeption_and_exit(tcp_socket_fd, 0);
				game_field_ship_destroy(own_field, &destroyed_ship_info);				
			}

			if (read(tcp_socket_fd, &state, sizeof(client_states)) <= 0)
				throw_exeption_and_exit(tcp_socket_fd, 0);

			if (state == CLIENT_STATE_GAME_LOSE)
				break;
		}
		else
		{
			shot.pos_x = ncurses_get_user_input(0) - '0';
			shot.pos_y = ncurses_get_user_input(1) - 'a';

			if (!is_coordinates_input_correct(shot.pos_x, shot.pos_y))
			{
				ncurses_incorrect_coord_input_msg();
				continue;
			}
			
			if (game_field_check_coordinates_to_hit(enemy_field, shot.pos_x, shot.pos_y))
			{
				ncurses_incorrect_coord_input_msg();
				continue;
			}

			// There you send information to the server about your turn
			if (write(tcp_socket_fd, &shot, sizeof(client_shot)) <= 0)
				throw_exeption_and_exit(tcp_socket_fd, 0);

			if (read(tcp_socket_fd, &answer, sizeof(server_answer)) <= 0)
				throw_exeption_and_exit(tcp_socket_fd, 0);
			
			if (answer == SERVER_ERROR)
				continue;

			// Read server info about your hit
			if (read(tcp_socket_fd, &shot, sizeof(client_shot)) <= 0)
				throw_exeption_and_exit(tcp_socket_fd, 0);

			game_field_set_shot(enemy_field, &shot);
			
			if (shot.state == CELL_STATE_SHIP_DESTROY)
			{
				ship destroyed_ship_info;
				if (read(tcp_socket_fd, &destroyed_ship_info, sizeof(ship)) <= 0)
					throw_exeption_and_exit(tcp_socket_fd, 0);
				game_field_ship_destroy(enemy_field, &destroyed_ship_info);
			}

			// Read server info about your state
			if (read(tcp_socket_fd, &state, sizeof(client_states)) <= 0)
				throw_exeption_and_exit(tcp_socket_fd, 0);

			if (state == CLIENT_STATE_GAME_WIN)
				break;
		}
	}
	
	ncurses_renderer_destroy();

	if (state == CLIENT_STATE_GAME_WIN)
		printf("You are win the game! :)\n");
	else
		printf("You are lose :(\n");

	own_field = game_field_destroy(own_field);
	enemy_field = game_field_destroy(enemy_field);

	close(tcp_socket_fd);
	
	exit(0);
}






/* ========================================================================= */
/* ========================================================================= */
/* ========================================================================= */






int main(int argc, char* argv[])
{
	srand(time(NULL));
	signal(SIGINT, signal_handler);

	char ip_index_from_argv = 0;
	char port_index_from_argv = 0;
	char file_name_index_from_argv = 0;

	void* (*first_player_function_ptr)(void* thread_arguments);
	void* (*second_player_function_ptr)(void* thread_arguments);

	first_player_function_ptr = &player_pthread_function;
	second_player_function_ptr = &computer_player_pthread_function;

	for (int i = 1; i < argc; i++)
	{
		if (strcmp(argv[i], "--connect") == 0)
		{
			if (i + 2 > argc)
			{
				printf("Not enough connect arguments! Required: --connect <ip-address> <port>\n");
				return 0;
			}
			
			if (first_player_function_ptr != computer_player_pthread_function)
			{
				ip_index_from_argv = i + 1;
				port_index_from_argv = i + 2;
			}

			i += 2;
		}
		else if (strcmp(argv[i], "--computers") == 0)
		{
			if (ip_index_from_argv == 0)
				first_player_function_ptr = &computer_player_pthread_function;
		}
		else if (strcmp(argv[i], "--alg") == 0)
		{
			if (i + 1 > argc)
			{
				printf("Not enough algorithm arguments! Required: --alg <algorithm>\n");
				return 0;
			}

			i++;

			if (strcmp(argv[i], "random") == 0)
				computer_array_set = RANDOM_ARRAY_SET;
			else if (strcmp(argv[i], "steps") == 0)
				computer_array_set = SHOTS_STEPS_SET;
			else if (strcmp(argv[i], "garant") == 0)
				computer_array_set = GARANT_NON_RANDOM_SET;
			else if (strcmp(argv[i], "chess") == 0)
				computer_array_set = CHESS_ARRAY_SET;
			else
			{
				printf("Unknown algorithm! User --help to see avaible algoritms\n");
				return 0;
			}
		}
		else if (strcmp(argv[i], "--random") == 0)
		{
			if (ship_place_mod != FILE_SHIPS)
				ship_place_mod = RANDOM_SHIPS;
		}
		else if (strcmp(argv[i], "--file") == 0)
		{
			if (i + 1 > argc)
			{
				printf("Not enough arguments! Required: --file <file_name>\n");
				return 0;
			}

			i++;

			if (ship_place_mod != RANDOM_SHIPS)
			{
				ship_place_mod = FILE_SHIPS;
				file_name_index_from_argv = i;
			}
		}
		else if (strcmp(argv[i], "--help") == 0)
			print_programm_use_and_exit();
		else
		{
			printf("Unknown command! Use --help to see arguments list!\n");
			return 0;
		}
	}

	/* */

	if (ip_index_from_argv != 0)
		player_versus_player(argv[ip_index_from_argv], atoi(argv[port_index_from_argv]), argv[file_name_index_from_argv]);	

	/* */

	/* */

	game_field* first_field = game_field_create(CREATE_WITH_SHIPS_ARRAY);
	game_field* second_field = game_field_create(CREATE_WITH_SHIPS_ARRAY);

	client_states first_player_state;
	client_states second_player_state;

	thread_player_info first_player_thread_info;
	thread_player_info second_player_thread_info;

	/* */

	/* */

	if (first_player_function_ptr == player_pthread_function)
	{
		if (ship_place_mod == RANDOM_SHIPS)
			place_random_ships_to_field(first_field);
		else if (ship_place_mod == FILE_SHIPS)
		{
			if (!read_ships_positions_from_file(first_field, argv[file_name_index_from_argv]))
			{
				printf("Error! Input file with ships coord is incorrect! Fix the file and try again!\n");
				return 0;
			}
		}
	}
	
	/* */

	/* */

	first_player_thread_info.player_num = FIRST_PLAYER_NUM;

	first_player_thread_info.own_field = first_field;
	first_player_thread_info.enemy_field = second_field;

	first_player_thread_info.own_state = &first_player_state;
	first_player_thread_info.enemy_state = &second_player_state;

	/* */

	/* */

	second_player_thread_info.player_num = SECOND_PLAYER_NUM;

	second_player_thread_info.own_field = second_field;
	second_player_thread_info.enemy_field = first_field;

	second_player_thread_info.own_state = &second_player_state;
	second_player_thread_info.enemy_state = &first_player_state;

	/* */

	/* */

	if (get_random_char_from_range(0, 1))
	{
		first_player_state = CLIENT_STATE_MAKE_TURN;
		second_player_state = CLIENT_STATE_WAIT_TURN;
	}
	else
	{
		first_player_state = CLIENT_STATE_WAIT_TURN;
		second_player_state = CLIENT_STATE_MAKE_TURN;
	}

	/* */


	/* ================================================= */

	sem_id = semget(IPC_PRIVATE, 2, 0600 | IPC_CREAT);

	first_field->ships_cells_count = SHIPS_CELLS_COUNT;
	second_field->ships_cells_count = SHIPS_CELLS_COUNT;

	/* ================================================= */

	int first_thread_return_value;
	int second_thread_return_value;

	pthread_t first_player_thread;
	pthread_t second_player_thread;

	int thread_result1 = pthread_create(&first_player_thread, NULL, first_player_function_ptr, &first_player_thread_info);
	int thread_result2 = pthread_create(&second_player_thread, NULL, second_player_function_ptr, &second_player_thread_info);

	pthread_join(first_player_thread, (void**) &first_thread_return_value);
	pthread_join(second_player_thread, (void**) &second_thread_return_value);
	
	native_renderer_update(first_field, second_field);

	printf("============== RESULT ==============\n");

	if (first_player_state == CLIENT_STATE_GAME_WIN)
	{
		printf("Winner: First player!\n");
		printf("Looser: Second player!\n");
	}
	else
	{
		printf("Looser: First player!\n");
		printf("Winner: Second player!\n");
	}

	printf("------------------------------------\n");

	printf("First player ship cells count = %i\n", first_field->ships_cells_count);
	printf("Second player ship cells count = %i\n", second_field->ships_cells_count);

	printf("====================================\n");

	semctl(sem_id, 0, IPC_RMID, 0);

	first_field = game_field_destroy(first_field);
	second_field = game_field_destroy(second_field);

	return 0;
}