#ifndef SERVER_H
#define SERVER_H

#ifdef WIN32

#include <winsock2.h>

#elif defined (linux)

#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h> /* close */
#include <netdb.h> /* gethostbyname */
#define INVALID_SOCKET -1
#define SOCKET_ERROR -1
#define closesocket(s) close(s)
typedef int SOCKET;
typedef struct sockaddr_in SOCKADDR_IN;
typedef struct sockaddr SOCKADDR;
typedef struct in_addr IN_ADDR;

#else

#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h> /* close */
#include <netdb.h> /* gethostbyname */
#define INVALID_SOCKET -1
#define SOCKET_ERROR -1
#define closesocket(s) close(s)
typedef int SOCKET;
typedef struct sockaddr_in SOCKADDR_IN;
typedef struct sockaddr SOCKADDR;
typedef struct in_addr IN_ADDR;

#endif

#define CRLF        "\r\n"
#define PORT         1977
#define MAX_CLIENTS     100
#define MAX_GAMES   50

#define BUF_SIZE    1024


#include "awale.h"
#include "client.h"

typedef struct {
  Awale awale;
  Client* player1;
  Client* player2;
} Game;

static void init(void);
static void end(void);
static void app(void);
static int init_connection(void);
static void end_connection(int sock);
static int read_client(SOCKET sock, char *buffer);
static void write_client(SOCKET sock, const char *buffer);
static void send_message_to_all_clients(Client *clients, Client client, int actual, const char *buffer, char from_server);
static void remove_client(Client *clients, int to_remove, int *actual);
static void clear_clients(Client *clients, int actual);
static void sendMenu(SOCKET sock);
static void sendPlayersList(SOCKET sock, Client *clients, int numberOfClients);
static void sendAvailablePlayersList(SOCKET sock, Client *clients, int numberOfClients);
static void sendAvailableGamesList(SOCKET sock, Game * games, int numberOfGames);
static void sendRules(SOCKET sock);

static void removeGame(Game *gameToRemove, Game *games, int *numberOfGames);
static Client * pseudoValid(const char* buffer, Client* clients, int nbClients, char * message);
static Game * createGame(Client * player1, Client * player2, Game* games, int * numberOfGames);
static Game * acceptGame(Client * defiedClient, Game * games, int numberOfGames);
static void refuseGame(Client * defiedClient, Game * games, int numberOfGames);
static Game * getGameByClient(Client * client, Game * games, int numberOfGames);

#endif /* guard */
