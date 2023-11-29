CC = gcc
CFLAGS = -Wall
SRC_DIR = Serveur

all: 
	gcc -Wall -c Serveur/server.c -o server.o
	gcc -Wall -c awale.c -o awale.o
	gcc -Wall server.o awale.o -o server.exe
	gcc -Wall Client/client2.c -o client.exe

clean:
	rm -f awale.o server
