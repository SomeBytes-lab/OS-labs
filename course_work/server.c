#include "time.h"
#include "stdio.h"
#include "errno.h"
#include "unistd.h"
#include "stdlib.h"
#include "string.h"
#include "dirent.h"
#include "signal.h"
#include "sys/sem.h"
#include "sys/ipc.h"
#include "pthread.h"
#include "strings.h"
#include "sys/stat.h"
#include "arpa/inet.h"
#include "sys/types.h"
#include "sys/socket.h"
#include "netinet/in.h"

#include "field.h"
#include "define.h"
#include "utility.h"
#include "renderer.h"

typedef struct thread_player_info_struct
{
	char player_num;

	game_field* own_field;
	game_field* enemy_field;

	client_states* own_state;
	client_states* enemy_state;

	u_int16_t tcp_socket_fd;
} thread_player_info;


int sem_id;

ship ship_destroyed;
client_shot global_shot;

void print_use_help_and_exit()
{
	printf("*----------- Use this options ------------*\n");
	printf("| <nothing> == To start with any port     |\n");
	printf("| <port> == To start with selected port   |\n");
	printf("| <help> == To print help                 |\n");
	printf("*-----------------------------------------*\n");
	exit(0);
}

void signal_handler(int nsig)
{
	if (nsig == SIGINT)
	{
		semctl(sem_id, 0, IPC_RMID, 0);
		exit(0);
	}
}

void sem_set_state(unsigned short sem_num, short state)
{
	struct sembuf op;
	op.sem_op = state;
	op.sem_flg = 0;
	op.sem_num = sem_num;
	semop(sem_id, &op, 1);	
}

char is_player_input_correct_ship_struct(ship* shp)
{
	if (shp->pos_x < 0 || shp->pos_x > 9)
		return 0;
	if (shp->pos_y < 0 || shp->pos_y > 9)
		return 0;
	if (shp->ship_dir != SHIP_DIRECTION_VERTICAL && shp->ship_dir != SHIP_DIRECTION_HORIZONTAL)
		return 0;
	return 1;
}

char is_player_input_correct_shot_struct(client_shot* shot)
{
	if (shot->pos_x < 0 || shot->pos_x > 9)
		return 0;
	if (shot->pos_y < 0 || shot->pos_y > 9)
		return 0;
	return 1;
}

/* ========================================================================= */
/* ========================================================================= */
/* ========================================================================= */

void* player_pthread(void* player_struct)
{
	signal(SIGINT, signal_handler);
	
	thread_player_info* player_info = (thread_player_info*) player_struct;
	server_answer serv_answer;
	
	char ship_place_mod = 0;
	
	if (read(player_info->tcp_socket_fd, &ship_place_mod, sizeof(ship_place_mod)) < 0)
		throw_exeption_and_exit(player_info->tcp_socket_fd, player_info->player_num);

	if (!ship_place_mod)
	{
		char placed_ships_count = 0;
		while (placed_ships_count < SHIPS_COUNT)
		{
			ship ship_place;
			serv_answer = SERVER_ERROR;

			if (read(player_info->tcp_socket_fd, &ship_place, sizeof(ship)) <= 0)
				throw_exeption_and_exit(player_info->tcp_socket_fd, player_info->player_num);

			if (!is_player_input_correct_ship_struct(&ship_place))
			{
				if (write(player_info->tcp_socket_fd, &serv_answer, sizeof(server_answer)) <= 0)
					throw_exeption_and_exit(player_info->tcp_socket_fd, player_info->player_num);
				continue;
			}

			if (!game_field_is_can_place_ship(player_info->own_field, &ship_place))
			{
				if (write(player_info->tcp_socket_fd, &serv_answer, sizeof(server_answer)) <= 0)
					throw_exeption_and_exit(player_info->tcp_socket_fd, player_info->player_num);
				continue;
			}

			serv_answer = SERVER_OK;
			if (write(player_info->tcp_socket_fd, &serv_answer, sizeof(server_answer)) <= 0)
				throw_exeption_and_exit(player_info->tcp_socket_fd, player_info->player_num);

			game_field_place_ship_to_field(player_info->own_field, &ship_place);
			ship_struct_assignment_operation(&player_info->own_field->ships_info[placed_ships_count], &ship_place);

			placed_ships_count++;
		}
	}
	else
	{
		char placed_ships_count = 0;
		while (placed_ships_count < SHIPS_COUNT)
		{
			ship ship_place;
			serv_answer = SERVER_ERROR;

			if (read(player_info->tcp_socket_fd, &ship_place, sizeof(ship)) <= 0)
				throw_exeption_and_exit(player_info->tcp_socket_fd, player_info->player_num);

			if (!is_player_input_correct_ship_struct(&ship_place))
			{
				if (write(player_info->tcp_socket_fd, &serv_answer, sizeof(server_answer)) <= 0)
					throw_exeption_and_exit(player_info->tcp_socket_fd, player_info->player_num);
				continue;
			}

			if (!game_field_is_can_place_ship(player_info->own_field, &ship_place))
			{
				if (write(player_info->tcp_socket_fd, &serv_answer, sizeof(server_answer)) <= 0)
					throw_exeption_and_exit(player_info->tcp_socket_fd, player_info->player_num);
				continue;
			}

			serv_answer = SERVER_OK;
			if (write(player_info->tcp_socket_fd, &serv_answer, sizeof(server_answer)) <= 0)
				throw_exeption_and_exit(player_info->tcp_socket_fd, player_info->player_num);

			game_field_place_ship_to_field(player_info->own_field, &ship_place);
			ship_struct_assignment_operation(&player_info->own_field->ships_info[placed_ships_count], &ship_place);

			placed_ships_count++;
		}
	}

	if (read(player_info->tcp_socket_fd, player_info->own_state, sizeof(client_states)) <= 0)
		throw_exeption_and_exit(player_info->tcp_socket_fd, player_info->player_num);

	sem_set_state(get_enemy_num(player_info->player_num), SEMAPHORE_GIVE_ONE);
	sem_set_state(player_info->player_num, SEMAPHORE_REDUCE_ONE);

	if (player_info->player_num == FIRST_PLAYER_NUM)
	{
		if (get_random_char_from_range(0, 1))
		{
			(*player_info->own_state) = CLIENT_STATE_MAKE_TURN;
			(*player_info->enemy_state) = CLIENT_STATE_WAIT_TURN;
		}
		else
		{
			(*player_info->own_state) = CLIENT_STATE_WAIT_TURN;
			(*player_info->enemy_state) = CLIENT_STATE_MAKE_TURN;
		}
	}

	sem_set_state(get_enemy_num(player_info->player_num), SEMAPHORE_GIVE_ONE);
	sem_set_state(player_info->player_num, SEMAPHORE_REDUCE_ONE);

	if (write(player_info->tcp_socket_fd, player_info->own_state, sizeof(client_states)) <= 0)
		throw_exeption_and_exit(player_info->tcp_socket_fd, player_info->player_num);

	while (1)
	{
		client_shot shot;

		if (player_info->player_num == FIRST_PLAYER_NUM)
			native_renderer_update(player_info->own_field, player_info->enemy_field);

		if ((*player_info->own_state) == CLIENT_STATE_MAKE_TURN)
		{
			serv_answer = SERVER_ERROR;
			while (serv_answer != SERVER_OK)
			{
				if (read(player_info->tcp_socket_fd, &shot, sizeof(client_shot)) <= 0)
					throw_exeption_and_exit(player_info->tcp_socket_fd, player_info->player_num);
				
				if (!is_player_input_correct_shot_struct(&shot))
				{
					if (write(player_info->tcp_socket_fd, &serv_answer, sizeof(server_answer)) <= 0)
						throw_exeption_and_exit(player_info->tcp_socket_fd, player_info->player_num);
					continue;
				}

				serv_answer = SERVER_OK;
				if (write(player_info->tcp_socket_fd, &serv_answer, sizeof(server_answer)) <= 0)
					throw_exeption_and_exit(player_info->tcp_socket_fd, player_info->player_num);
			}

			shot.state = game_field_get_hit_result(player_info->enemy_field, &shot);
			shot_struct_assignment_operation(&global_shot, &shot);

			if (shot.state == CELL_STATE_SHIP_HIT)
			{
				char ship_num = game_field_get_hit_ship_num_by_coord(player_info->enemy_field->ships_info, &shot);
				if (game_field_check_ship_destroy(player_info->enemy_field, &player_info->enemy_field->ships_info[ship_num]))
				{
					global_shot.state = CELL_STATE_SHIP_DESTROY;

					ship_struct_assignment_operation(&ship_destroyed, &player_info->enemy_field->ships_info[ship_num]);
					game_field_ship_destroy(player_info->enemy_field, &player_info->enemy_field->ships_info[ship_num]);
					
					if (write(player_info->tcp_socket_fd, &global_shot, sizeof(client_shot)) <= 0)
						throw_exeption_and_exit(player_info->tcp_socket_fd, player_info->player_num);

					if (write(player_info->tcp_socket_fd, &player_info->enemy_field->ships_info[ship_num], sizeof(ship)) < 0)
						throw_exeption_and_exit(player_info->tcp_socket_fd, player_info->player_num);
				}
				else
				{
					if (write(player_info->tcp_socket_fd, &global_shot, sizeof(client_shot)) <= 0)
						throw_exeption_and_exit(player_info->tcp_socket_fd, player_info->player_num);
				}				
			}
			else
			{
				if (write(player_info->tcp_socket_fd, &global_shot, sizeof(client_shot)) <= 0)
					throw_exeption_and_exit(player_info->tcp_socket_fd, player_info->player_num);				
			}

			if (shot.state == CELL_STATE_HIT)
			{
				*(player_info->own_state) = CLIENT_STATE_WAIT_TURN;
				*(player_info->enemy_state) = CLIENT_STATE_MAKE_TURN;
			}
			else
			{
				player_info->enemy_field->ships_cells_count--;
				if (player_info->enemy_field->ships_cells_count == 0)
				{
					*(player_info->own_state) = CLIENT_STATE_GAME_WIN;
					*(player_info->enemy_state) = CLIENT_STATE_GAME_LOSE;
				}
			}

			if (write(player_info->tcp_socket_fd, player_info->own_state, sizeof(client_states)) <= 0)
				throw_exeption_and_exit(player_info->tcp_socket_fd, player_info->player_num);

			sem_set_state(get_enemy_num(player_info->player_num), SEMAPHORE_GIVE_ONE);
			sem_set_state(player_info->player_num, SEMAPHORE_REDUCE_ONE);

			if ((*player_info->own_state) == CLIENT_STATE_GAME_WIN)
				break;
		}
		else
		{
			sem_set_state(player_info->player_num, SEMAPHORE_REDUCE_ONE);

			if (write(player_info->tcp_socket_fd, &global_shot, sizeof(client_shot)) <= 0)
				throw_exeption_and_exit(player_info->tcp_socket_fd, player_info->player_num);
			
			if (global_shot.state == CELL_STATE_SHIP_DESTROY)
				if (write(player_info->tcp_socket_fd, &ship_destroyed, sizeof(ship)) <= 0)
					throw_exeption_and_exit(player_info->tcp_socket_fd, player_info->player_num);

			if (write(player_info->tcp_socket_fd, player_info->own_state, sizeof(client_states)) <= 0)
				throw_exeption_and_exit(player_info->tcp_socket_fd, player_info->player_num);

			sem_set_state(get_enemy_num(player_info->player_num), SEMAPHORE_GIVE_ONE);
		
			if ((*player_info->own_state) == CLIENT_STATE_GAME_LOSE)
				break;
		}
	}

	return 0;
}

/* ========================================================================= */
/* ========================================================================= */
/* ========================================================================= */

void print_server_port(const u_int16_t tcp_socket_fd)
{
	struct sockaddr_in socket_address;
	socklen_t socket_len = sizeof(socket_address);

	getsockname(tcp_socket_fd, (struct sockaddr*) &socket_address, &socket_len);
	printf("Server started in %i port\n", ntohs(socket_address.sin_port));
}

int main(int argc, char* argv[])
{	
	signal(SIGINT, signal_handler);
	
	/* ===================================== */
	/* ======== ARGUMENT PROCESSING ======== */
	/* ===================================== */

	uint16_t server_port;

	if (argc == 1)
		server_port = 0;
	if (argc == 2)
	{	
		if (strcmp(argv[1], "help") == 0)
			print_use_help_and_exit();
	
		server_port = atoi(argv[1]);
	}
	else
		print_use_help_and_exit();


	/* ===================================== */
	/* ======= SOCKET INITIALIZATION ======= */
	/* ===================================== */

	uint32_t received_bytes = 0;

	u_int16_t tcp_socket_fd;
	u_int16_t tcp_first_player_socket_fd;
	u_int16_t tcp_second_player_socket_fd;

	socket_address_in server_address;
	socket_address_in client_address;

	client_states first_client_state;
	client_states second_client_state;


	/* ============================= */
	/* === Trying to init socket === */

	if ((tcp_socket_fd = socket(PF_INET, SOCK_STREAM, 0)) < 0)
	{
		perror(NULL);
		return -1;
	}

	/* ============================= */
	/* == Set up server variables == */

	server_address.sin_family = AF_INET;
	server_address.sin_port = htons(server_port);
	server_address.sin_addr.s_addr = htonl(INADDR_ANY);	

	if (bind(tcp_socket_fd, (struct sockaddr*) &server_address, sizeof(server_address)) < 0)
	{
		printf("Error! Cant get %i port! Trying to get another!\n", server_port);

		server_address.sin_port = htons(0);
		if (bind(tcp_socket_fd, (struct sockaddr*) &server_address, sizeof(server_address)) < 0)
		{
			perror(NULL);
			close(tcp_socket_fd);
			return -1;
		}
	}

	print_server_port(tcp_socket_fd);

	/* ============================= */
	/* === Set number of listens === */

	if (listen(tcp_socket_fd, 1) < 0)
	{
		perror(NULL);
		close(tcp_socket_fd);
		return -1;
	}


	/* ===================================== */
	/* ====== CLIENTS INITIALIZATION ======= */
	/* ===================================== */

	/* ============================= */
	/* ======== FIRST PLAYER ======= */

	tcp_first_player_socket_fd = accept(tcp_socket_fd, (socket_address*) &client_address, &received_bytes);

	if (tcp_first_player_socket_fd < 0)
	{
		perror(NULL);
		close(tcp_socket_fd);
		return -1;
	}

	first_client_state = CLIENT_STATE_WAITING_ANOTHER_PLAYER;

	if (write(tcp_first_player_socket_fd, &first_client_state, sizeof(client_states)) < 0)
	{
		perror(NULL);
		close(tcp_socket_fd);
		return -1;		
	}

	printf("First player is successfully connected!\n");
	printf("Waiting second player...\n");

	/* ============================= */
	/* ======= SECOND PLAYER ======= */	

	tcp_second_player_socket_fd = accept(tcp_socket_fd, (socket_address*) &client_address, &received_bytes);

	if (tcp_second_player_socket_fd < 0)
	{
		perror(NULL);
		close(tcp_socket_fd);
		return -1;
	}

	first_client_state = CLIENT_STATE_PLACING_SHIPS;
	second_client_state = CLIENT_STATE_PLACING_SHIPS;

	if (write(tcp_first_player_socket_fd, &first_client_state, sizeof(client_states)) < 0)
	{
		perror(NULL);
		close(tcp_socket_fd);
		return -1;		
	}

	if (write(tcp_second_player_socket_fd, &second_client_state, sizeof(client_states)) < 0)
	{
		perror(NULL);
		close(tcp_socket_fd);
		return -1;
	}


	/* ===================================== */
	/* ======== GAME VARIABLES INIT ======== */
	/* ===================================== */

	sem_id = semget(IPC_PRIVATE, 2, 0600 | IPC_CREAT);

	game_field* first_player_field = game_field_create(CREATE_WITH_SHIPS_ARRAY);
	game_field* second_player_field = game_field_create(CREATE_WITH_SHIPS_ARRAY);

	first_player_field->ships_cells_count = SHIPS_CELLS_COUNT;
	second_player_field->ships_cells_count = SHIPS_CELLS_COUNT;

	thread_player_info first_player_thread_info;
	thread_player_info second_player_thread_info;

	/* ============================== */
	/* == FIRST PLAYER STRUCT INIT == */

	first_player_thread_info.player_num = FIRST_PLAYER_NUM;
	first_player_thread_info.tcp_socket_fd = tcp_first_player_socket_fd;
	
	first_player_thread_info.own_field = first_player_field;
	first_player_thread_info.enemy_field = second_player_field;
	
	first_player_thread_info.own_state = &first_client_state;
	first_player_thread_info.enemy_state = &second_client_state;

	/* ============================= */
	/* = SECOND PLAYER STRUCT INIT = */

	second_player_thread_info.player_num = SECOND_PLAYER_NUM;
	second_player_thread_info.tcp_socket_fd = tcp_second_player_socket_fd;

	second_player_thread_info.own_field = second_player_field;
	second_player_thread_info.enemy_field = first_player_field;

	
	second_player_thread_info.own_state = &second_client_state;
	second_player_thread_info.enemy_state = &first_client_state;

	/* ===================================== */
	/* ======= THREADS START ROUTINE ======= */
	/* ===================================== */

	int first_thread_return_value;
	int second_thread_return_value;

	pthread_t first_player_thread;
	pthread_t second_player_thread;

	int thread_result1 = pthread_create(&first_player_thread, NULL, player_pthread, &first_player_thread_info);
	int thread_result2 = pthread_create(&second_player_thread, NULL, player_pthread, &second_player_thread_info);

	pthread_join(first_player_thread, (void**) &first_thread_return_value);
	pthread_join(second_player_thread, (void**) &second_thread_return_value);

	native_renderer_clear();
	native_renderer_update(first_player_field, second_player_field);

	printf("First thread exit with status: %i\n", first_thread_return_value);
	printf("Second thread exit with status: %i\n", second_thread_return_value);

	printf("Server shutting down...\n");

	first_player_field = game_field_destroy(first_player_field);
	second_player_field = game_field_destroy(second_player_field);

	return 0;
}