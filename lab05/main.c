#include "stdio.h"
#include "stdlib.h"
#include "unistd.h"
#include "time.h"
#include "sys/wait.h"
#include "sys/stat.h"
#include "fcntl.h"
#include <sys/cdefs.h>

void print_array_int(int* arr, int size)
{
   	for (int i = 0; i < size; i++)
		printf("%d ", arr[i]);
	printf("\n");
}

int compare_int_value(const void* a, const void* b)
{
	return *((int*) b) - *((int*) a);
}

int main(int argv, char* argc[])
{
	if (argv <= 1)
	{
		printf("Error! Not enough params!\n");
		return -1;
	}


	int array_size = atoi(argc[1]);
	int* array = malloc(sizeof(int) * array_size);

	srand(time(NULL));
	for (int i = 0; i < array_size; i++)
	{
		array[i] = rand() % 100;
		printf("%d ", array[i]);
	}
	printf("\n");

	int fd_pipe[2], fd_fifo;
	size_t size;

	if (pipe(fd_pipe) < 0)
	{
		printf("Error! Can't create pipe!\n");
		return -1;
	}

	const char* file_name = "temp.fifo";
	(void) umask(0);

	if (mknod(file_name, S_IFIFO | 0666, 0) < 0)
	{
		printf("Error! Can't create FIFO!\n");
		return -1;
	}

	pid_t child_process = fork();

	if (child_process == -1)
	{
		printf("Error! Can't fork child!\n");
		return -1;
	}
	else if (child_process == 0)
	{
		close(fd_pipe[1]);

		int* new_array = malloc(sizeof(int) * array_size);
		size = read(fd_pipe[0], new_array, sizeof(int) * array_size);

		if (size < 0)
		{
			printf("Error! Child can't read array!\n");
			return -1;
		}

		qsort(new_array, array_size, sizeof(int), compare_int_value);
		close(fd_pipe[0]);

		if ((fd_fifo = open(file_name, O_WRONLY)) < 0)
		{
			printf("Error! Child can't open FIFO for writing!\n");
			return -1;
		}

		size = write(fd_fifo, new_array, sizeof(int) * array_size);
		if (size != sizeof(int) * array_size)
		{
			printf("Error! Child can't write all array using FIFO!\n");
			return -1;
		}

		close(fd_fifo);
		free(new_array);

		printf("Child process done!\n");
		return 0;
	}
	else
	{
		close(fd_pipe[0]);
		size = write(fd_pipe[1], array, sizeof(int) * array_size);

		if (size != sizeof(int) * array_size)
		{
			printf("Error! Parent can't write all array!\n");
			return -1;
		}

		close(fd_pipe[1]);

		if ((fd_fifo = open(file_name, O_RDONLY)) < 0)
		{
			printf("Error! Parent can't open FIFO for reading!\n");
			return -1;
		}

		size = read(fd_fifo, array, sizeof(int) * array_size);
		if (size < 0)
		{
			printf("Error! Parent can't read array using FIFO!\n");
			return -1;
		}

		printf("Parent recieved this array: ");
		print_array_int(array, array_size);
		
		close(fd_fifo);
	}

	char delete_fifo_file[124];
	sprintf(delete_fifo_file, "rm %s", file_name);
	system(delete_fifo_file);

	free(array);
	return 0;
}
