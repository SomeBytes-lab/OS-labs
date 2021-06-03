#ifndef DEFINE_H
#define DEFINE_H

#define RANDOM_ARRAY_SET 0
#define SHOTS_STEPS_SET 1
#define GARANT_NON_RANDOM_SET 2
#define CHESS_ARRAY_SET 3

#define FIRST_PLAYER_NUM 0
#define SECOND_PLAYER_NUM 1

#define CREATE_WITHOUT_SHIPS_ARRAY 0
#define CREATE_WITH_SHIPS_ARRAY 1

#define SEMAPHORE_GIVE_ONE 1
#define SEMAPHORE_REDUCE_ONE -1

#define FIELD_SIZE 10
#define SHIPS_COUNT 10
#define SHOT_ARRAY_SIZE 30
#define SHIPS_CELLS_COUNT 20

#define TORPEDO_SHIP_COUNT 4
#define DESTROYER_SHIP_COUNT 3
#define CRUISER_SHIP_COUNT 2
#define BATTLESHIP_SHIP_COUNT 1



typedef enum client_states_enum
{
	CLIENT_STATE_WAITING_ANOTHER_PLAYER,
	CLIENT_STATE_PLACING_SHIPS,
	CLIENT_STATE_BATTLE,
	CLIENT_STATE_GAME_WIN,
	CLIENT_STATE_GAME_LOSE,
	CLIENT_STATE_WAIT_TURN,
	CLIENT_STATE_MAKE_TURN
} client_states;

typedef enum cell_state_enum
{
	CELL_STATE_VOID = ' ',
	CELL_STATE_HIT = 'O',
	CELL_STATE_SHIP = '%',
	CELL_STATE_SHIP_HIT = 'X',
	CELL_STATE_SHIP_DESTROY = '@'
} cell_state;

typedef enum ship_category_enum
{
	TORPEDO_BOAT_CATEGORY = 1,
	DESTROYER_CATEGORY,
	CRUISER_CATEGORY,
	BATTLESHIP_CATEGORY
} ship_category;

typedef enum ship_direction_enum
{
	SHIP_DIRECTION_VERTICAL = 'v',
	SHIP_DIRECTION_HORIZONTAL = 'h'
} ship_direction;

typedef enum server_answer_enum
{
	SERVER_OK,
	SERVER_ERROR
} server_answer;

typedef struct sockaddr socket_address; 
typedef struct sockaddr_in socket_address_in;

typedef struct client_shot_struct
{
	char pos_x;
	char pos_y;
	
	cell_state state;
} client_shot;

typedef struct computer_shot_struct
{
	char pos_x;
	char pos_y;
} computer_shot;

typedef struct ship_struct
{
	char pos_x;
	char pos_y;

	ship_category ship_ctg;
	ship_direction ship_dir;
} ship;

typedef struct game_field_struct
{
	ship* ships_info;
	char ships_cells_count;
	char field[FIELD_SIZE][FIELD_SIZE];
} game_field;

#endif