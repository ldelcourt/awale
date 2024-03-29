#ifndef AWALE_H
#define AWALE_H

#define true 1
#define false 0
#define ERR_TILE_NUMBER 2

typedef struct {
  int playerNumber;
  char* playerName;
  int score;
} Player;

typedef struct {
  Player* player1;
  Player* player2;
  int currentPlayer;
  int endOfGame;
  int * gameState;
} Awale;

Awale initGame(char* player1Name, char* player2Name);
void printGameState(char * gameBoard, Awale game);
int checkLegalMove(int playerNumber, int tile, Awale * game);
void moveSeeds(int tile, Awale * game);
int checkEndGame(Awale * game);
int playTurn(int tile, Awale * game);
void endGameMessage(char * message, Awale * game, int playerNumber, int deconnection);
void closeGame(Awale * game);

#endif /* AWALE_H */
