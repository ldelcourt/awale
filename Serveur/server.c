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
               if(c == 0 || atoi(buffer) == 21)
               {
                  Game * playerGame = getGameByClient(&clients[i], games, numberOfGames);
                  if (playerGame != NULL) {
                     char message[4096];
                     Client * opponent = client.sock == playerGame->player1->sock ?
                           playerGame->player2 :
                           playerGame->player1;
                     endGameMessage(message, &(playerGame->awale), client.state == IN_GAME_PLAYER_1 ? 2 : 1, true);
                     write_client(opponent->sock, message);
                     sendMenu(opponent->sock);
                     opponent->state = IN_MENU;
                     removeGame(playerGame, games, &numberOfGames);
                  }
                  closesocket(clients[i].sock);
                  remove_client(clients, i, &actual);
                  strncpy(buffer, client.name, BUF_SIZE - 1);
                  strncat(buffer, " disconnected !", BUF_SIZE - strlen(buffer) - 1);
               }
               else
               {
                  /* Gestion d'une partie */
                  if (client.state == IN_GAME_PLAYER_1 || client.state == IN_GAME_PLAYER_2) {
                     Game * game = getGameByClient(&clients[i], games, numberOfGames);
                     int tile = atoi(buffer);
                     char message[4096];

                     /* Si ce n'est pas au tour du joueur ou que le move n'est pas légal, on renvoie l'état du jeu au client */
                     if (game->awale.currentPlayer != client.state || checkLegalMove(client.state, tile, &(game->awale)) == false) {
                        printGameState(message, game->awale);
                        strcat(message, "Vous ne pouvez pas choisir cette case, choisissez une autre case");
                        strcat(message, game->awale.currentPlayer == 1 ? " entre 0 et 5\r\n" : " entre 6 et 11\r\n");
                        if(game->awale.currentPlayer == 1){
                           write_client(game->player1->sock,  message);
                        } else {
                           write_client(game->player2->sock,  message);
                        }
                        continue;
                     } 
                     
                     /* Si le move est légal on joue et puis on envoie le plateau aux deux joueurs */
                     if(playTurn(tile, &(game->awale)) != true) {
                        addMove(tile, game);
                        printGameState(message, game->awale);
                        SOCKET currentPlayerSock = game->awale.currentPlayer == IN_GAME_PLAYER_1 ?
                           game->player1->sock : game->player2->sock;
                        SOCKET otherPlayerSock = game->awale.currentPlayer != IN_GAME_PLAYER_1 ?
                           game->player1->sock : game->player2->sock;
                        char temp[4096]; 
                        strcpy(temp, message);
                        strcat(message, "A vous de jouer, choisissez une case");
                        strcat(message, game->awale.currentPlayer == 1 ? " entre 0 et 5\r\n" : " entre 6 et 11\r\n");
                        strcat(temp, "En attente du joueur en face\r\n");
                        write_client(currentPlayerSock, message);
                        write_client(otherPlayerSock, temp);
                        continue;
                     }
                     
                     /* Si c'est la fin de partie, on envoi le message pour savoir qui a gagné */
                     char messageJoueur1[1024];
                     char messageJoueur2[1024];
                     endGameMessage(messageJoueur1, &(game->awale), IN_GAME_PLAYER_1, false);
                     endGameMessage(messageJoueur2, &(game->awale), IN_GAME_PLAYER_2, false);
                     game->player1->state = IN_MENU;
                     game->player2->state = IN_MENU;
                     write_client(game->player1->sock, messageJoueur1);
                     write_client(game->player2->sock, messageJoueur2);
                     removeGame(game, games, &numberOfGames);
                  }

                  /* Gestion d'un client dans le Menu */
                  if(client.state == IN_MENU) {
                     switch (atoi(buffer))
                     {
                     /* 1. Afficher la liste des clients en ligne */
                     case 1:
                        sendPlayersList(client.sock, clients, actual);
                        write_client(client.sock, "6. Retour menu\n");
                        write_client(client.sock, "21. Se déconnecter\n");
                        break;

                     /* 2. Défier un autre joueur */
                     case 2:
                        sendAvailablePlayersList(client.sock, clients, actual);
                        write_client(client.sock, "\nEntrez le pseudo de la personne que vous souhaitez defier\n\n");
                        write_client(client.sock, "6. Retour menu\n");
                        write_client(client.sock, "21. Se déconnecter\n");
                        clients[i].state = DEFYING;
                        break;
                     /* 3. Observer une partie */
                     case 3:
                        sendAvailableGamesList(client.sock, games, numberOfGames);
                        write_client(client.sock, "\nEntrez le numéro de la partie pour voir le plateau actuel\n\n");
                        write_client(client.sock, "6. Retour menu\n");
                        write_client(client.sock, "21. Se déconnecter\n");
                        clients[i].state = OBSERVATEUR;
                        break;
                     /* 4. Lire les règles du jeu */
                     case 4:
                        sendRules(client.sock);
                        write_client(client.sock, "6. Retour menu\n");
                        write_client(client.sock, "21. Se déconnecter\n");
                        clients[i].state = READING_RULES;
                        break;
                     /* 5. Envoyer un message a un joueur*/
                     case 5:
                        sendPlayersList(client.sock, clients, actual);
                        write_client(client.sock, "Entrer le nom du joueur suivi du message\n\n");
                        write_client(client.sock, "6. Retour menu\n");
                        write_client(client.sock, "21. Se déconnecter\n");
                        clients[i].state = TCHATING;
                        break;
                     /* 6. Retour au menu */
                     case 6:
                        sendMenu(client.sock);
                        break;
                     /* 7. Voir l'historique des parties */
                     case 7: 
                        clients[i].state = CONSULTING_HISTORY;
                        char history[1024];
                        memset(history, 0, sizeof(history));
                        viewHistory(history);
                        write_client(client.sock, "Voici l'historiques des parties\n\n");
                        write_client(client.sock, history);
                        write_client(client.sock, "6. Retour menu\n");
                        write_client(client.sock, "21. Se déconnecter\n");
                        break;
                     
                     default:
                        write_client(client.sock, "Commande invalide, veuillez réessayer\r\n");
                        sendMenu(client.sock);
                        break;
                     }
                  }
                  /* Gestion du cas d'un joueur qui observe une partie */
                  if(client.state == OBSERVATEUR) {
                     if((atoi(buffer) <= (numberOfGames + 1))&&((atoi(buffer) != 6)||(atoi(buffer) != 21))){
                        int numGameChoose = atoi(buffer)-1;
                        Game * gameChoose = &games[numGameChoose];
                        char message[4096];
                        printGameState(message, gameChoose->awale);
                        write_client(client.sock, message);
                        char temp[4];
                        snprintf(temp, sizeof(temp), " %d ", atoi(buffer));
                        strcat(temp, ". Actualiser\n");
                        write_client(client.sock, temp);
                        write_client(client.sock, "6. Retour menu\n");
                        write_client(client.sock, "21. Se déconnecter\n");

                     } else {
                        switch (atoi(buffer))
                        {
                        case 6:
                           sendMenu(client.sock);
                           clients[i].state = IN_MENU;
                           break;
            
                        default:
                           write_client(client.sock, "Commande invalide, veuillez réessayer\r\n\n");
                           write_client(client.sock, "6. Retour menu\n");
                           write_client(client.sock, "21. Se déconnecter\n");
                           break;
                        }
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
                     
                     default:
                        write_client(client.sock, "Commande invalide, veuillez réessayer\r\n\n");
                        write_client(client.sock, "6. Retour menu\n");
                        write_client(client.sock, "21. Se déconnecter\n");
      
                        break;
                     }
                  }

                  /* Gestion du cas d'un joueur qui envoie un message */
                  if(client.state == TCHATING) {
                     char messageRetour[1024];

                     if (strchr(buffer, ' ') != NULL) {
                        char pseudo[50];
                        char message[974];

                        if (sscanf(buffer, "%49s %974[^\n]", pseudo, message) == 2) {
                           Client *destinataire = pseudoValid(pseudo, clients, actual, messageRetour);

                           if (destinataire != NULL) {
                              char fullMessage[1024];
                              snprintf(fullMessage, sizeof(fullMessage), "%s vous envoie : %s\n", client.name, message);
                              write_client(destinataire->sock, fullMessage);
                              write_client(client.sock, "\nMessage envoyé\n");
                           } else {
                              write_client(client.sock, "\nDestinataire pas trouvé\n");
                           }
                        }
                     } else {

                     switch (atoi(buffer))
                     {
                     case 6:
                        sendMenu(client.sock);
                        clients[i].state = IN_MENU;
                        break;
                     
                     default:
                        write_client(client.sock, "Commande invalide\n\n");
                        write_client(client.sock, "Pseudo Message pour envoyer un message\n");
                        write_client(client.sock, "6. Retour menu\n");
                        write_client(client.sock, "21. Se déconnecter\n");
      
                        break;
                     }
                     }
                  }


                  /* Gestion d'un joueur qui choisi de défier un autre joueur */
                  if(client.state == DEFYING) {
                     char messageRetour[1024];
                     memset(messageRetour, 0, sizeof(messageRetour));
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
                     default:
                        write_client(client.sock, messageRetour);
                        write_client(client.sock, "6. Retour menu\n");
                        write_client(client.sock, "21. Se déconnecter\n");
      
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

                  /* Gestion du cas d'un joueur consulant l'historique des parties */
                  if(client.state == CONSULTING_HISTORY) {
                     switch (atoi(buffer))
                     {
                     case 6:
                        sendMenu(client.sock);
                        clients[i].state = IN_MENU;
                        break;
                     default:
                        write_client(client.sock, "Commande invalide, veuillez réessayer\r\n\n");
                        write_client(client.sock, "6. Retour menu\n");
                        write_client(client.sock, "21. Se déconnecter\n");
      
                        break;
                     }
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

static void removeGame(Game * gameToRemove, Game * games, int * numberOfGame) {
   /* Save the game in history */
   saveGame(gameToRemove);

   /* Free the appropriate memory */
   free(gameToRemove->moves);
   closeGame(&(gameToRemove->awale));

   /* Remove the game from the array of games */
   for(int i = 0; i < (*numberOfGame); i++) {
      if(gameToRemove != &(games[i])) continue;
      memmove(gameToRemove, gameToRemove + 1, (*numberOfGame - i - 1) * sizeof(Game));

   }

   /* Decrease the number of total games */
   (*numberOfGame)--;
}

static void saveGame(Game *gameToSave)
{
    FILE *file = fopen("saved_games.txt", "a");

    if (file == NULL)
    {
        perror("Error opening file for writing");
        return;
    }

    /* Save player names, scores, and moves */ 
    fprintf(file, "%s %s %d %d %d", gameToSave->player1->name, gameToSave->player2->name,
            gameToSave->awale.player1->score, gameToSave->awale.player2->score, gameToSave->numberOfMoves);

    for (int i = 0; i < gameToSave->numberOfMoves; i++)
    {
        fprintf(file, " %d", gameToSave->moves[i]);
    }

    fprintf(file, "\n");

    fclose(file);
}

static void viewHistory(char *history) {
    FILE *file = fopen("saved_games.txt", "r");

    if (file == NULL)
    {
        perror("Error opening file for reading");
        return;
    }

    char line[BUF_SIZE];
    int gameNumber = 1;

   /* Reads each line of the file */
    while (fgets(line, sizeof(line), file) != NULL)
    {
        char player1Name[BUF_SIZE];
        char player2Name[BUF_SIZE];
        int score1, score2, numberOfMoves;

        if (sscanf(line, "%s %s %d %d %d", player1Name, player2Name, &score1, &score2, &numberOfMoves) == 5)
        {
            /* Append the game informations to the history variable */
            sprintf(history + strlen(history), "%d. %s %d - %s %d : In %d moves\n",
                    gameNumber, player1Name, score1, player2Name, score2, numberOfMoves);

            gameNumber++;
        }
    }

    fclose(file);
}

static void addMove(int move, Game *game) {
   game->moves[game->numberOfMoves] = move;
   game->numberOfMoves++;
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
   write_client(sock, "\nBienvenue sur Awale, que voulez vous faire ?\n");
   write_client(sock, "1. Voir la liste des pseudos en ligne\n");
   write_client(sock, "2. Défier un autre joueur\n");
   write_client(sock, "3. Voir les parties en cours\n");
   write_client(sock, "4. Voir les règles du jeu\n");
   write_client(sock, "5. Envoyer un message a un joueur\n");
   write_client(sock, "7. Consulter l'historique des parties\n");
   write_client(sock, "21. Se déconnecter\n");
}

static void sendPlayersList(SOCKET sock, Client *clients, int numberOfClients) { 
   write_client(sock, "Voici la liste des joueurs connectés et de leur état : \n");
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
   write_client(sock, "\n");
};
static void sendAvailablePlayersList(SOCKET sock, Client *clients, int numberOfClients) {
   write_client(sock, "Voici la liste des joueurs connectés que vous pouvez défier: \n\n");
   for(int i = 0; i < numberOfClients; i ++) {
      Client client = clients[i];
      if(client.sock == sock) continue;
      if(client.state != IN_GAME_PLAYER_1 && client.state != IN_GAME_PLAYER_2 && client.state != OBSERVATEUR) {
         write_client(sock, client.name);
         write_client(sock, ": Disponible\n");
      }
   }
};

static void sendAvailableGamesList(SOCKET sock, Game * games, int numberOfGames){
   char buffer[BUF_SIZE];
   int i;

   write_client(sock, "Liste des parties en cours : \n");

   for (i = 0; i < numberOfGames; i++) {
      if (games[i].player1->state == IN_GAME_PLAYER_1 && games[i].player2->state == IN_GAME_PLAYER_2) {
         snprintf(buffer, BUF_SIZE, "%d. %s vs %s\n", i + 1, games[i].player1->name, games[i].player2->name);
         write_client(sock, buffer);
      }
   }
}

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
   games[(*numberOfGames)].moves = (History)malloc(sizeof(History) * 200);
   games[(*numberOfGames)].numberOfMoves = 0;

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
   memset(gameState, 0, sizeof(gameState));
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
