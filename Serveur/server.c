#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>

#include "server.h"

static void init(void)
{
#ifdef WIN32
   WSADATA wsa;
   int err = WSAStartup(MAKEWORD(2, 2), &wsa);
   if(err < 0)
   {
      puts("WSAStartup failed !");
      exit(EXIT_FAILURE);
   }
#endif
}

static void end(void)
{
#ifdef WIN32
   WSACleanup();
#endif
}

static void app(void)
{
   SOCKET sock = init_connection();
   char buffer[BUF_SIZE];
   /* the index for the array */
   int actual = 0;
   int max = sock;
   /* an array for all clients */
   Client clients[MAX_CLIENTS];
   /* An Array of all current Awale Games */
   Game games[MAX_GAMES];
   int numberOfGames = 0;

   fd_set rdfs;

   while(1)
   {
      int i = 0;
      FD_ZERO(&rdfs);

      /* add STDIN_FILENO */
      FD_SET(STDIN_FILENO, &rdfs);

      /* add the connection socket */
      FD_SET(sock, &rdfs);

      /* add socket of each client */
      for(i = 0; i < actual; i++)
      {
         FD_SET(clients[i].sock, &rdfs);
      }

      if(select(max + 1, &rdfs, NULL, NULL, NULL) == -1)
      {
         perror("select()");
         exit(errno);
      }

      /* something from standard input : i.e keyboard */
      if(FD_ISSET(STDIN_FILENO, &rdfs))
      {
         /* stop process when type on keyboard */
         break;
      }
      else if(FD_ISSET(sock, &rdfs))
      {
         /* new client */
         SOCKADDR_IN csin = { 0 };
         size_t sinsize = sizeof csin;
         int csock = accept(sock, (SOCKADDR *)&csin, &sinsize);
         if(csock == SOCKET_ERROR)
         {
            perror("accept()");
            continue;
         }

         /* after connecting the client sends its name */
         if(read_client(csock, buffer) == -1)
         {
            /* disconnected */
            continue;
         }

         /* what is the new maximum fd ? */
         max = csock > max ? csock : max;

         FD_SET(csock, &rdfs);

         Client c = { csock };
         strncpy(c.name, buffer, BUF_SIZE - 1);
         c.state = IN_MENU;
         clients[actual] = c;
         actual++;
         sendMenu(csock);
      }
      else
      {
         int i = 0;
         for(i = 0; i < actual; i++)
         {
            /* a client is talking */
            if(FD_ISSET(clients[i].sock, &rdfs))
            {
               Client client = clients[i];
               int c = read_client(clients[i].sock, buffer);
               /* client disconnected */
               if(c == 0)
               {
                  closesocket(clients[i].sock);
                  remove_client(clients, i, &actual);
                  strncpy(buffer, client.name, BUF_SIZE - 1);
                  strncat(buffer, " disconnected !", BUF_SIZE - strlen(buffer) - 1);
                  send_message_to_all_clients(clients, client, actual, buffer, 1);
               }
               else
               {
                  /* Gestion d'un client dans le Menu */
                  if(client.state == IN_MENU) {
                     switch (atoi(buffer))
                     {
                     /* 1. Afficher la liste des clients en ligne */
                     case 1:
                        sendPlayersList(client.sock, clients, actual);
                        write_client(client.sock, "6. Retour menu\n");
                        write_client(client.sock, "0. Se déconnecter\n");
                        break;

                     /* 2. Défier un autre joueur */
                     case 2:
                        sendAvailablePlayersList(client.sock, clients, actual);
                        write_client(client.sock, "Entrez le pseudo de la personne que vous souhaitez defier\n\n");
                        write_client(client.sock, "6. Retour menu\n");
                        write_client(client.sock, "0. Se déconnecter\n");
                        clients[i].state = DEFYING;
                        break;
                     /* 3. Observer une partie */
                     case 3:
                        write_client(client.sock, "Cette fonction n'est pas encore implémentée\r\n");
                        sendMenu(client.sock);
                        break;
                     /* 4. Lire les règles du jeu */
                     case 4:
                        sendRules(client.sock);
                        write_client(sock, "6. Retour menu\n");
                        write_client(sock, "0. Se déconnecter\n");
                        clients[i].state = READING_RULES;
                        break;
                     /* 5. Voir l'historique des parties jouée */
                     case 5:
                        write_client(client.sock, "Cette fonction sera implémentée sous peu\r\n");
                        sendMenu(client.sock);
                        break;
                     /* 6. Retour au menu */
                     case 6:
                        sendMenu(client.sock);
                        break;
                     /* 0. Déconnection d'un joueur */
                     case 0:
                        break;
                     
                     default:
                        write_client(client.sock, "Commande invalide, veuillez réessayer\r\n");
                        sendMenu(client.sock);
                        break;
                     }
                  }
                  /* Gestion du cas d'un joueur qui lit les règles */
                  if(client.state == READING_RULES) {
                     switch (atoi(buffer))
                     {
                     case 6:
                        sendMenu(client.sock);
                        clients[i].state = IN_MENU;
                        break;
                     case 0:

                        break;
                     
                     default:
                        write_client(client.sock, "Commande invalide, veuillez réessayer\r\n\n");
                        write_client(client.sock, "6. Retour menu\n");
                        write_client(client.sock, "0. Se déconnecter\n");
      
                        break;
                     }
                  }

                  /* Gestion d'un joueur qui choisi de défier un autre joueur */
                  if(client.state == DEFYING) {
                     char messageRetour[1024];
                     Client * adversaire = pseudoValid(buffer, clients, actual, messageRetour);
                     if(adversaire != NULL){
                        //Defier l'adversaire
                        createGame(&clients[i], adversaire, games, &numberOfGames);
                        char* message = client.name;
                        strcat(message, " vous défie vous pouvez accepter (1) ou refuser (2)\n");
                        adversaire->state = DEFIED;
                        write_client(adversaire->sock, message);
                        write_client(client.sock, messageRetour);
                        continue;
                     }
                     

                     switch (atoi(buffer))
                     {
                     case 6:
                        sendMenu(client.sock);
                        clients[i].state = IN_MENU;
                        break;
                     case 0:

                        break;
                     
                     default:
                        write_client(client.sock, messageRetour);
                        write_client(client.sock, "6. Retour menu\n");
                        write_client(client.sock, "0. Se déconnecter\n");
      
                        break;
                     }
                  }

                  /* Gestion du cas d'un joueur défié */
                  if (client.state == DEFIED) {
                     switch (atoi(buffer))
                     {
                     case 1:
                        acceptGame(&clients[i], games, numberOfGames);
                        break;
                     case 2:
                        refuseGame(&clients[i], games, numberOfGames);
                        break;
                     
                     default:
                        write_client(client.sock, "Commande invalide, choisissez d'accepter (1) ou de refuser (2)\r\n\n");
                        break;
                     }
                  }

                  /* Gestion d'une partie */
                  if (client.state == IN_GAME_PLAYER_1 || client.state == IN_GAME_PLAYER_2) {
                     Game * game = getGameByClient(&clients[i], games, numberOfGames);
                     int tile = atoi(buffer);
                     char message[4096];

                     /* Si ce n'est pas au tour du joueur ou que le move n'est pas légal, on renvoie l'état du jeu au client */
                     if (game->awale.currentPlayer != client.state || checkLegalMove(client.state, tile, &(game->awale)) == false) {
                        printGameState(message, game->awale);
                        write_client(game->player1->sock, strcat(message, "A vous de jouer choisissez une case\r\n"));
                        write_client(game->player2->sock, strcat(message, "En attente du joueur en face\r\n"));
                        continue;
                     } 
                     
                     /* Si le move est légal on joue et puis on envoie le plateau aux deux joueurs */
                     if(playTurn(tile, &(game->awale)) != true) {
                        printGameState(message, game->awale);
                        SOCKET currentPlayerSock = game->awale.currentPlayer == IN_GAME_PLAYER_1 ?
                           game->player1->sock : game->player2->sock;
                        SOCKET otherPlayerSock = game->awale.currentPlayer != IN_GAME_PLAYER_1 ?
                           game->player1->sock : game->player2->sock;
                        write_client(otherPlayerSock, message);
                        write_client(currentPlayerSock, strcat(message, "A vous de jouer\n"));
                        continue;
                     }
                     
                     /* Si c'est la fin de partie, on envoi le message pour savoir qui a gagné */
                     char messageJoueur1[1024];
                     char messageJoueur2[1024];
                     endGameMessage(messageJoueur1, &(game->awale), IN_GAME_PLAYER_1);
                     endGameMessage(messageJoueur2, &(game->awale), IN_GAME_PLAYER_2);
                     write_client(game->player1->sock, messageJoueur1);
                     write_client(game->player2->sock, messageJoueur2);
                  }

               }
               break;
            }
         }
      }
   }

   clear_clients(clients, actual);
   end_connection(sock);
}

static void clear_clients(Client *clients, int actual)
{
   int i = 0;
   for(i = 0; i < actual; i++)
   {
      closesocket(clients[i].sock);
   }
}

static void remove_client(Client *clients, int to_remove, int *actual)
{
   /* we remove the client in the array */
   memmove(clients + to_remove, clients + to_remove + 1, (*actual - to_remove - 1) * sizeof(Client));
   /* number client - 1 */
   (*actual)--;
}

static void send_message_to_all_clients(Client *clients, Client sender, int actual, const char *buffer, char from_server)
{
   int i = 0;
   char message[BUF_SIZE];
   message[0] = 0;
   for(i = 0; i < actual; i++)
   {
      /* we don't send message to the sender */
      if(sender.sock != clients[i].sock)
      {
         if(from_server == 0)
         {
            strncpy(message, sender.name, BUF_SIZE - 1);
            strncat(message, " : ", sizeof message - strlen(message) - 1);
         }
         strncat(message, buffer, sizeof message - strlen(message) - 1);
         write_client(clients[i].sock, message);
      }
   }
}

static int init_connection(void)
{
   SOCKET sock = socket(AF_INET, SOCK_STREAM, 0);
   SOCKADDR_IN sin = { 0 };

   if(sock == INVALID_SOCKET)
   {
      perror("socket()");
      exit(errno);
   }

   sin.sin_addr.s_addr = htonl(INADDR_ANY);
   sin.sin_port = htons(PORT);
   sin.sin_family = AF_INET;

   if(bind(sock,(SOCKADDR *) &sin, sizeof sin) == SOCKET_ERROR)
   {
      perror("bind()");
      exit(errno);
   }

   if(listen(sock, MAX_CLIENTS) == SOCKET_ERROR)
   {
      perror("listen()");
      exit(errno);
   }

   return sock;
}

static void end_connection(int sock)
{
   closesocket(sock);
}

static int read_client(SOCKET sock, char *buffer)
{
   int n = 0;

   if((n = recv(sock, buffer, BUF_SIZE - 1, 0)) < 0)
   {
      perror("recv()");
      /* if recv error we disonnect the client */
      n = 0;
   }

   buffer[n] = 0;

   return n;
}

static void write_client(SOCKET sock, const char *buffer)
{
   if(send(sock, buffer, strlen(buffer), 0) < 0)
   {
      perror("send()");
      exit(errno);
   }
}

static void sendMenu(SOCKET sock) {
   write_client(sock, "Bienvenue sur Awale, que voulez vous faire ?\n");
   write_client(sock, "1. Voir la liste des pseudos en ligne\n");
   write_client(sock, "2. Défier un autre joueur\n");
   write_client(sock, "3. (Observer une partie)\n");
   write_client(sock, "4. Voir les règles du jeu\n");
   write_client(sock, "5. Voir l'historique des parties\n");
   write_client(sock, "0. Se déconnecter\n");
}

static void sendPlayersList(SOCKET sock, Client *clients, int numberOfClients) { 
   write_client(sock, "Voici la liste des Joueurs connectés et de leur état\n");
   for(int i = 0; i < numberOfClients; i ++) {
      Client client = clients[i];
      write_client(sock, client.name);
      switch (client.state)
      {
      case IN_MENU:
      case READING_RULES:
      case DEFYING: 
         write_client(sock, ": Disponible\n");
         break;
      case IN_GAME_PLAYER_1:
      case IN_GAME_PLAYER_2:
         write_client(sock, ": En partie\n");
         break;
      case OBSERVATEUR: 
         write_client(sock, ": Regarde une partie\n");
         break;
      default:
         break;
      }
   }
};
static void sendAvailablePlayersList(SOCKET sock, Client *clients, int numberOfClients) {
   write_client(sock, "Voici la liste des Joueurs connectés que vous pouvez défier\n");
   for(int i = 0; i < numberOfClients; i ++) {
      Client client = clients[i];
      if(client.sock == sock) continue;
      if(client.state != IN_GAME_PLAYER_1 || client.state != IN_GAME_PLAYER_2 || client.state != OBSERVATEUR) {
         write_client(sock, client.name);
         write_client(sock, ": Disponible\n");
      }
   }
};

static void sendRules(SOCKET sock){
   write_client(sock, "Regles de l'Awale :\n\n");
   write_client(sock, "Plateau de jeu : Awale se joue sur un plateau composé de deux rangées de six trous, appelés maisons, pour un total de 12 maisons.\n\n");
   write_client(sock, "Pions : Chaque maison contient initialement 4 graines (ou pions). Ainsi, il y a un total de 48 graines au début du jeu.\n\n");
   write_client(sock, "But du jeu : L'objectif est de capturer le plus grand nombre possible de graines. Le joueur qui a capturé le plus de graines à la fin du jeu remporte la partie.\n\n");
   write_client(sock, "Déroulement du jeu : Les joueurs jouent à tour de rôle. Pendant son tour, un joueur prend toutes les graines d'une de ses maisons et les distribue une par une dans les maisons suivantes, dans le sens contraire des aiguilles d'une montre. Si la dernière graine atterrit dans une maison déjà occupée, le joueur continue de semer ses graines dans les maisons suivantes.\n\n");
   write_client(sock, "Captures : Si la dernière graine d un joueur tombe dans une maison qui rend le nombre total de graines dans cette maison égal à 2 ou 3, le joueur capture ces graines. Il les retire du plateau et les place dans son propre 'banc' (une rangée spéciale à sa droite).\n\n");
   write_client(sock, "Fin de la partie : Le jeu se termine lorsque l'un des joueurs ne peut plus jouer, c'est-à-dire lorsqu il ne reste plus de graines dans ses maisons. Le joueur qui a capturé le plus de graines dans son banc remporte la partie.\n\n");
   write_client(sock, "Gagnant : Le joueur qui a capturé le plus de graines à la fin de la partie est déclaré vainqueur.\n\n");
}

static Client * pseudoValid(const char* buffer, Client* clients, int nbClient, char * message) {
   for(int i = 0; i < nbClient; i++) {
      Client client = clients[i];
      if(strcmp(client.name, buffer) != 0) continue;
      strcat(message, buffer);
      strcat(message, " défié en attente de réponse\n");
      return &(clients[i]);
   }
   strcat(message, "Aucun joueur trouvé ne correspond au nom: ");
   strcat(message, buffer);
   strcat(message, "veuillez réessayer\n");
   return NULL;
};

static Game * createGame(Client * player1, Client * player2, Game* games, int * numberOfGames) {
   /* Initialize the game */
   games[(*numberOfGames)].player1 = player1;
   games[(*numberOfGames)].player2 = player2;
   games[(*numberOfGames)].awale = initGame(player1->name, player2->name);

   /* Increase the number of total games */
   (*numberOfGames)++;

   /* Set the client status accordingly */
   player1->state = WAITING;
   player2->state = DEFIED;

   /* Return a pointer to the game */
   return &games[(*numberOfGames) - 1];
}

static Game * acceptGame(Client * defiedClient, Game * games, int numberOfGames) {
   Game * game = getGameByClient(defiedClient, games, numberOfGames);
   game->player1->state = IN_GAME_PLAYER_1;
   game->player2->state = IN_GAME_PLAYER_2;
   char gameState[4096];
   printGameState(gameState, game->awale);
   write_client(game->player1->sock, gameState);
   write_client(game->player2->sock, gameState);

   return game;
}

static void refuseGame(Client * defiedClient, Game * games, int numberOfGames) {
   Game * game = getGameByClient(defiedClient, games, numberOfGames);
   game->player1->state = IN_MENU;
   game->player2->state = IN_MENU;
   SOCKET defiyingClientSocket = game->player1 == defiedClient ? game->player2->sock : game->player1->sock; 
   write_client(defiyingClientSocket, "Your opponent declined the challenge #LOOSER");
   sendMenu(game->player1->sock);
   sendMenu(game->player2->sock);
}


static Game * getGameByClient(Client * client, Game * games, int numberOfGames) {
   for(int i = 0; i < numberOfGames; i ++) {
      if (games[i].player2 == client || games[i].player1 == client) return &games[i];
   }
   return NULL;
}

int main(int argc, char **argv)
{
   init();

   app();

   end();

   return EXIT_SUCCESS;
}
