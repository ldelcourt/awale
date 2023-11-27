#include <stdio.h>
#include <stdlib.h>
#include <string.h>

struct player {
  int playerNumber;
  char* playerName;
  int score;
};

struct awale {
  struct player* player1;
  struct player* player2;
  int currentPlayer;
  int endOfGame;
  int gameState[12];
};

struct player* player1;
struct player* player2;
int currentPlayer = 1;
int endOfGame = false;
int gameState[12];

void initGame(char* player1Name, char* player2Name) {
  player1 = (struct player*)malloc(sizeof(struct player));
  player1->playerName = player1Name;
  player1->playerNumber = 1;
  player1->score = 0;

  player2 = (struct player*)malloc(sizeof(struct player));
  player2->playerName = player2Name;
  player2->playerNumber = 2;
  player2->score = 0;

  for(int i = 0; i < 12; i++) {
    gameState[i] = 3;
  }
}

void printGameState(char * gameBoard) {
  strcat(gameBoard, "\n\n  11 10  9  8  7  6\n");
  strcat(gameBoard, "---------------------\n| ");

  char temp[4];
  
  for(int i = 11; i > 5; i--) {
    if (gameState[i] < 10){
      snprintf(temp, sizeof(temp), " %d ", gameState[i]);
      strcat(gameBoard, temp);
    } else {
      snprintf(temp, sizeof(temp), "%d ", gameState[i]);
      strcat(gameBoard, temp);
    }
  }
  strcat(gameBoard,"|       Score\n| ");
  for(int i = 0; i <6; i++) {
    if (gameState[i] < 10){
      snprintf(temp, sizeof(temp), " %d ", gameState[i]);
      strcat(gameBoard, temp);
    } else {
      snprintf(temp, sizeof(temp), "%d ", gameState[i]);
      strcat(gameBoard, temp);
    }

  }
  strcat(gameBoard, "|       Player 1: ");
  snprintf(temp, sizeof(temp), " %d ", player1->score);
  strcat(gameBoard, temp);
  strcat(gameBoard, "- Player 2: ");
  snprintf(temp, sizeof(temp), " %d ", player2->score);
  strcat(gameBoard, temp);
  strcat(gameBoard, "\n---------------------\n   0  1  2  3  4  5\n\n");
}

int checkLegalMove(int playerNumber, int tile) {
  //Cas  1 : pas de graine  dans l'endroit selecctionnz
  if(gameState[tile] == 0) return false;

  //Cas 2 : numero selectionne hors zone de jeu
  if(playerNumber == 1 && (tile < 0 || tile > 5)) return false;
  if(playerNumber == 2 && (tile < 6 || tile > 11)) return false;

  //Cas 3: si plus de bille chez adversaire obligé de donner bille
  int opponentPlayer = (playerNumber==1) ? 2 : 1;
  int currentSide = (playerNumber==1) ? 0 : 6;
  int opponentSide = (opponentPlayer==1) ? 0 : 6;
  int sumOpponentSeeds = 0;
  for (int i = opponentSide; i < opponentSide + 6; i++){
    sumOpponentSeeds += gameState[i];
  }
  int possible = 0;
  if(sumOpponentSeeds == 0){
    for(int i = currentSide; i< currentSide+6; i++){
      if(gameState[i] > (6-i-currentSide)) {
        possible = 1;
        break;
      }
    }
    if(possible == 1 && gameState[tile] <= (6-tile-currentSide)){
      return false;
    }
  }
  //Cas 4 : Pas prendre en un seul coup toutes les graines de leur adversaire #BLC
  return true;
}

void moveSeeds(int tile) {
  int numberOfSeeds = gameState[tile];
  gameState[tile] = 0;
  for(int i = 1; i <= numberOfSeeds; i++) {
    gameState[(tile + i) % 12]++;
  }
  
  //Gagner les graines
  int checkedTile = (tile + numberOfSeeds) % 12;
  while (gameState[checkedTile] == 2 || gameState[checkedTile] == 3) {
    struct player* player = currentPlayer == 1 ? player1 : player2;
    player->score += gameState[checkedTile];
    gameState[checkedTile] = 0;

    checkedTile = (checkedTile + 11) % 12;
  }
}

int checkEndGame() {
  int startCheck = 0;
  if(currentPlayer == 2) startCheck = 6;
  for(int i = 0; i < 6; i++) {
    if(gameState[startCheck + i] != 0) return false;
  }
  return true;
}

int playTurn(int tile){
  //Check si le move est légal
  if(checkLegalMove(currentPlayer, tile) == false) return ERR_TILE_NUMBER;

  //Egrener
  moveSeeds(tile);

  currentPlayer = currentPlayer == 1 ? 2 : 1;

  //Voir si le jeu est fini
  return checkEndGame();
}


int main() {
  initGame("test", "me");
  printf("Bienvenue dans cette nouvelle partie Joueur 1 vous commencez !!\n");
  char* gameBoard = malloc(4096);

  while (endOfGame == false || endOfGame == ERR_TILE_NUMBER) {
    printGameState(gameBoard);
    printf("%s",  gameBoard);
    int tile = 0;
    scanf("%d", &tile);
    endOfGame = playTurn(tile);
  }
  printf("Bravo ! Le joueur %d a gagné :-)\n", currentPlayer);
  free(player1); 
  free(player2);
  free(gameBoard);
}