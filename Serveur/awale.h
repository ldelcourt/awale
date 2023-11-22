#ifndef AWALE_H
#define AWALE_H

#define ERR_TILE_NUMBER 2

typedef struct player {
  int playerNumber;
  char* playerName;
  int score;
} Player;

typedef struct awale {
  Player* player1;
  Player* player2;
  int currentPlayer;
  int endOfGame;
  int gameState[12];
} Awale;

void initGame(char* player1Name, char* player2Name);
void printGameState();
int checkLegalMove(int playerNumber, int tile);
void moveSeeds(int tile);
int checkEndGame();
int playTurn(int tile);

#endif /* AWALE_H */
