#ifndef CLIENT_H
#define CLIENT_H

#define IN_MENU 0
#define IN_GAME_PLAYER_1 1
#define IN_GAME_PLAYER_2 2
#define OBSERVATEUR 3
#define DEFYING 4
#define DEFIED 5
#define WAITING 6
#define READING_RULES 7
#define TCHATING 8
#define CONSULTING_HISTORY 9


#include "server.h"
#include "awale.h"

typedef int CLIENT_STATE;

typedef struct
{
   SOCKET sock;
   char name[BUF_SIZE];
   CLIENT_STATE state;
   Awale* game;
}Client;

#endif /* guard */
