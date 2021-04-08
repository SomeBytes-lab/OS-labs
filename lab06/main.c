#include "stdio.h"
#include "stdlib.h"
#include "unistd.h"
#include "time.h"
#include "stdlib.h"

#include "sys/shm.h"
#include "sys/types.h"
#include "sys/sem.h"
#include "sys/ipc.h"
#include "sys/wait.h"

#define SEMAPHORE_UNLOCK 1
#define SEMAPHORE_LOCK  -1

void array_fill_random_value(int* array, int size, int min, int max)
{
	for (int i = 0; i < size; i++)
		array[i] = min + rand() % max;
}

void* allocate_shared_memory(size_t mem_size, int* mem_id)
{
	*mem_id = shmget(IPC_PRIVATE, mem_size, 0600 | IPC_CREAT | IPC_EXCL);
	if (*mem_id <= 0)
	{
		perror("error with memId");
		return NULL;
	}

	void* mem = shmat(*mem_id, 0, 0);
	if (NULL == mem)
		perror("error with shmat");
	
	return mem;
}

void semaphore_set_state(int sem_id, int num, int state)
{
	struct sembuf op;
	op.sem_op = state;
	op.sem_flg = 0;
	op.sem_num = num;
	semop(sem_id, &op, 1);
}

char semaphore_lock(int sem_id, int num, char* array_check_is_lock_ptr)
{
	char is_lock = array_check_is_lock_ptr[num];
	semaphore_set_state(sem_id, num, SEMAPHORE_LOCK);
	array_check_is_lock_ptr[num] = 1;
	return is_lock;
}

void semaphore_unlock(int sem_id, int num, char* array_check_is_lock_ptr)
{
	semaphore_set_state(sem_id, num, SEMAPHORE_UNLOCK);
	array_check_is_lock_ptr[num] = 0;
}

void swap_values(int* first, int* second)
{
	int temp = *first;
	*first = *second;
	*second = temp;
}

void child_main_code(int* array, char* array_check_ptr, int array_size, int sem_id)
{
	double factor = 1.2473309;
	int step = array_size - 1;

	while (step >= 1)
	{
		for (int i = 0; i + step < array_size; i++)
		{
			semaphore_lock(sem_id, i, array_check_ptr);
			semaphore_lock(sem_id, i + step, array_check_ptr);

			if (array[i] > array[i + step])
				swap_values(&array[i], &array[i + step]);

			semaphore_unlock(sem_id, i + step, array_check_ptr);
			semaphore_unlock(sem_id, i, array_check_ptr);
		}
		step /= factor;
	}

	exit(0);
}

void parent_main_code(int* array, char* array_check_ptr, int array_size, int sem_id, pid_t child_id)
{
	int iteration = 0;
	while (!waitpid(child_id, NULL, WNOHANG))
	{
		printf("--- This is iteration %i ---\n", iteration);
		for (int i = 0; i < array_size; i++)
		{
			if (semaphore_lock(sem_id, i, array_check_ptr))
				printf("[%d] ", array[i]);
			else
				printf("%d ", array[i]);
			semaphore_unlock(sem_id, i, array_check_ptr);
		}
		printf("\n");
		iteration++;
	}

	printf("============== RESULT ==============\n");
	printf("Iteration count: %i\n", iteration);
	printf("====================================\n");
}

void free_shared_memory(int* mem_id)
{
	char resource_delete_command_buff[124];
	sprintf(resource_delete_command_buff, "ipcrm -m %i", *mem_id);
	system(resource_delete_command_buff);
	*mem_id = 0;
}

void free_semaphores(int* sem_id)
{
	char resource_delete_command_buff[124];
	sprintf(resource_delete_command_buff, "ipcrm -s %i", *sem_id);
	system(resource_delete_command_buff);
	*sem_id = 0;
}

int main(int argv, char* argc[])
{
	if (argv <= 3)
	{
		printf("Error! Not enough arguments! Required: 3 (array_size, min, max)\n");
		return -1;
	}
	
	/* --- array mem init --- */
	
	int array_size = atoi(argc[1]);
	int array_min_value = atoi(argc[2]);
	int array_max_value = atoi(argc[3]);

	int mem_id;
	int* array_ptr = allocate_shared_memory(sizeof(int) * array_size, &mem_id);

	array_fill_random_value(array_ptr, array_size, array_min_value, array_max_value);
	
	/* --- semaphore init --- */

	int sem_id = semget(IPC_PRIVATE, array_size, 0600 | IPC_CREAT);

	int check_mem_id;
	char* array_check_is_lock = allocate_shared_memory(sizeof(char) * array_size, &check_mem_id);

	if (sem_id < 0)
	{
		perror("Error with semget()!\n");
		return -1;
	}

	printf("Semaphore set id = %i\n", sem_id);

	for (int i = 0; i < array_size; i++)
		semaphore_set_state(sem_id, i, SEMAPHORE_UNLOCK);

	/* --- lab --- */

	pid_t child_process_id = fork();

	if (child_process_id == -1)
		perror("Error with fork() - process 1\n");
	else if (child_process_id == 0)
		child_main_code(array_ptr, array_check_is_lock, array_size, sem_id);
	else
		parent_main_code(array_ptr, array_check_is_lock, array_size, sem_id, child_process_id);


	free_semaphores(&sem_id);
	free_shared_memory(&mem_id);
	free_shared_memory(&check_mem_id);

	return 0;
}