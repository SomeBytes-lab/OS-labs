all:
	gcc field.c -o field.o -c
	gcc client.c -o client.o -c
	gcc server.c -o server.o -c
	gcc utility.c -o utility.o -c
	gcc computer.c -o computer.o -c
	gcc renderer.c -o renderer.o -c 

	gcc -lpthread -lncurses server.o field.o utility.o renderer.o -o server
	gcc -lpthread -lncurses client.o field.o utility.o computer.o renderer.o -o client

	rm field.o client.o server.o computer.o utility.o renderer.o