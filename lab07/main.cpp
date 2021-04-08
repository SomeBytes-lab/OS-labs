#include "stdio.h"
#include "stdlib.h"
#include "unistd.h"
#include "time.h"
#include "pthread.h"

#include "sys/ipc.h"
#include "sys/msg.h"

#include <algorithm>

typedef struct {
	long mtype;
	int msg_data[4];
} msg_struct;

void swap_values(int* first, int* second)
{
	int temp = *first;
	*first = *second;
	*second = *first;
}

int get_factorial(int i)
{
	if (i == 0)
		return 1;
	else
		return i * get_factorial(i - 1);
}

void* pthread_work(void* args)
{
	msg_struct pthread_msg;	
	int msg_id = *((int *) args);

	ssize_t msg_len = msgrcv(msg_id, &pthread_msg, sizeof(pthread_msg), 0, 0);
	printf("Thread received random numbers. Ready to work!\n");
	
	do
	{
		msgsnd(msg_id, &pthread_msg, sizeof(pthread_msg), 0);
	} while(std::next_permutation(pthread_msg.msg_data, pthread_msg.msg_data + 4));

	return 0;
}

void print_received_msg(msg_struct* msg)
{
	for (int i = 0; i < 4; i++)
		printf("%d ", msg->msg_data[i]);
	printf("\n");
}

int compare_int_value(const void* a, const void* b)
{
	return *((int*) a) - *((int*) b);
}

int main(void)
{
	pthread_t thread;
	int random_numbers[4];
	
	srand(time(NULL));
	for (int i = 0; i < 4; i++)
		random_numbers[i] = rand() % 1000;

	qsort(random_numbers, 4, sizeof(int), compare_int_value);

	printf("Random numbers is: ");
	for (int i = 0; i < 4; i++)
		printf("%i ", random_numbers[i]);
	printf("\n");

	int msg_id = msgget(IPC_PRIVATE, 0600 | IPC_CREAT);
	if (msg_id < 0)
	{
		perror("Error with msgget()!\n");
		return -1;
	}

	printf("Message id = %i\n", msg_id);

	int res1 = pthread_create(&thread, NULL, pthread_work, &msg_id);

	// prepare to send values to thread

	msg_struct parent_msg;

	parent_msg.mtype = 7;
	for (int i = 0; i < 4; i++)
		parent_msg.msg_data[i] = random_numbers[i];

	msgsnd(msg_id, &parent_msg, sizeof(parent_msg), 0);

	int count_msg_received = 0;
	int count_msg_to_receive = get_factorial(4);

	printf("Needs: %d\n", count_msg_to_receive);

	while (count_msg_received != count_msg_to_receive)
	{
		ssize_t msg_len = msgrcv(msg_id, &parent_msg, sizeof(parent_msg), 0, 0);
		printf("Received %li bytes: ", msg_len);
		print_received_msg(&parent_msg);
		count_msg_received++;
	}

	msgctl(msg_id, IPC_RMID, NULL);

	return 0;
}