all: server client

server: Server/Server.c Server/common.h
	gcc -Wextra -pedantic Server/Server.c -o server -pthread -lrt

client: Client/Client.c
	gcc -Wextra -pedantic Client/Client.c -o client -pthread -lrt
