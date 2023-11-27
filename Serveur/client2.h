#ifndef CLIENT_H
#define CLIENT_H

#define IN_MENU 0
#define IN_GAME_PLAYER_1 1
#define IN_GAME_PLAYER_2 2
#define OBSERVATEUR 3
#define DEFYING 4
#define READING_RULES 5

#include "server2.h"

typedef int CLIENT_STATE;

typedef struct
{
   SOCKET sock;
   char name[BUF_SIZE];
   CLIENT_STATE state;
   Awale* game;
}Client;

#endif /* guard */
