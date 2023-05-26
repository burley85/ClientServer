all: server.c
		gcc -o server server.c -lwsock32