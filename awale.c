#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "awale.h"

Awale initGame(char* player1Name, char* player2Name) {
  Player * player1 = (Player*)malloc(sizeof(Player));
  player1->playerName = (char*)malloc(sizeof(char)*1024);
  strcpy(player1->playerName, player1Name);
  player1->playerNumber = 1;
  player1->score = 0;

  Player * player2 = (Player*)malloc(sizeof(Player));
  player2->playerName = (char*)malloc(sizeof(char)*1024);
  strcpy(player2->playerName, player2Name);
  player2->playerNumber = 2;
  player2->score = 0;

  int * gameState = (int *)malloc(sizeof(int) * 12);

  for(int i = 0; i < 12; i++) {
    gameState[i] = 3;
  }
  Awale awale;
  awale.player1 = player1;
  awale.player2 = player2;
  awale.currentPlayer = 1;
  awale.endOfGame = false;
  awale.gameState = gameState;

  return awale;
}

void printGameState(char * gameBoard, Awale game) {
  strcat(gameBoard, "\n\n  11 10  9  8  7  6\n");
  strcat(gameBoard, "---------------------\n| ");

  char temp[4];
  
  for(int i = 11; i > 5; i--) {
    if (game.gameState[i] < 10){
      snprintf(temp, sizeof(temp), " %d ", game.gameState[i]);
      strcat(gameBoard, temp);
    } else {
      snprintf(temp, sizeof(temp), "%d ", game.gameState[i]);
      strcat(gameBoard, temp);
    }
  }
  strcat(gameBoard,"|       Score\n| ");
  for(int i = 0; i <6; i++) {
    if (game.gameState[i] < 10){
      snprintf(temp, sizeof(temp), " %d ", game.gameState[i]);
      strcat(gameBoard, temp);
    } else {
      snprintf(temp, sizeof(temp), "%d ", game.gameState[i]);
      strcat(gameBoard, temp);
    }

  }
  strcat(gameBoard, "|       Player 1: ");
  snprintf(temp, sizeof(temp), " %d ", game.player1->score);
  strcat(gameBoard, temp);
  strcat(gameBoard, "- Player 2: ");
  snprintf(temp, sizeof(temp), " %d ", game.player2->score);
  strcat(gameBoard, temp);
  strcat(gameBoard, "\n---------------------\n   0  1  2  3  4  5\n\n");
}

int checkLegalMove(int playerNumber, int tile, Awale * game) {
  //Cas  1 : pas de graine  dans l'endroit selecctionnz
  if(game->gameState[tile] == 0) return false;

  //Cas 2 : numero selectionne hors zone de jeu
  if(playerNumber == 1 && (tile < 0 || tile > 5)) return false;
  if(playerNumber == 2 && (tile < 6 || tile > 11)) return false;

  //Cas 3: si plus de bille chez adversaire obligé de donner bille
  int opponentPlayer = (playerNumber==1) ? 2 : 1;
  int currentSide = (playerNumber==1) ? 0 : 6;
  int opponentSide = (opponentPlayer==1) ? 0 : 6;
  int sumOpponentSeeds = 0;
  for (int i = opponentSide; i < opponentSide + 6; i++){
    sumOpponentSeeds += game->gameState[i];
  }
  int possible = 0;
  if(sumOpponentSeeds == 0){
    for(int i = currentSide; i< currentSide+6; i++){
      if(game->gameState[i] > (6-i-currentSide)) {
        possible = 1;
        break;
      }
    }
    if(possible == 1 && game->gameState[tile] <= (6-tile-currentSide)){
      return false;
    }
  }
  //Cas 4 : Pas prendre en un seul coup toutes les graines de leur adversaire #BLC
  return true;
}

void moveSeeds(int tile, Awale * game) {
  int numberOfSeeds = game->gameState[tile];
  game->gameState[tile] = 0;
  for(int i = 1; i <= numberOfSeeds; i++) {
    game->gameState[(tile + i) % 12]++;
  }
  
  //Gagner les graines
  int checkedTile = (tile + numberOfSeeds) % 12;
  while ((game->gameState[checkedTile] == 2 || game->gameState[checkedTile] == 3)&&(((game->currentPlayer ==1)&&(checkedTile>5))||((game->currentPlayer ==2)&&(checkedTile<6)))) {
    Player* player = game->currentPlayer == 1 ? game->player1 : game->player2;
    player->score += game->gameState[checkedTile];
    game->gameState[checkedTile] = 0;

    checkedTile = (checkedTile + 11) % 12;
  }
}

int checkEndGame(Awale * game) {
  int startCheck = 0;
  if(game->currentPlayer == 2) startCheck = 6;
  for(int i = 0; i < 6; i++) {
    if(game->gameState[startCheck + i] != 0) return false;
  }
  return true;
}

int playTurn(int tile, Awale * game){
  //Check si le move est légal
  if(checkLegalMove(game->currentPlayer, tile, game) == false) return ERR_TILE_NUMBER;

  //Egrener
  moveSeeds(tile, game);

  game->currentPlayer = game->currentPlayer == 1 ? 2 : 1;

  //Voir si le jeu est fini
  return checkEndGame(game);
}

void endGameMessage(char * message, Awale * game, int playerNumber, int deconnection) {
  if (deconnection == true) {
    strcat(message, "Your opponent disconnected, congrat you win !\r\n");
    return;
  }
  Player * player1 = game->player1;
  Player * player2 = game->player2;
  if (player1->score > player2->score) {
    if(player1->playerNumber == playerNumber) strcat(message, "Congrats you win !\n");
    if(player2->playerNumber == playerNumber) strcat(message, "Sorry you loose\n");
  }
  else {
    if(player2->playerNumber == playerNumber) strcat(message, "Congrats you win !\n");
    if(player1->playerNumber == playerNumber) strcat(message, "Sorry you loose\n");
  }
}

void closeGame(Awale * game) {
  free(game->player1->playerName);
  free(game->player1);
  free(game->player2->playerName);
  free(game->player2);
  free(game->gameState);
}

/*
int main() {
  printf("Bienvenue dans cette nouvelle partie Joueur 1 vous commencez !!\n");
  char* gameBoard = malloc(4096);
  Awale game = initGame("test", "me");

  while (game.endOfGame == false || game.endOfGame == ERR_TILE_NUMBER) {
    printGameState(gameBoard, game);
    printf("%s",  gameBoard);
    int tile = 0;
    scanf("%d", &tile);
    game.endOfGame = playTurn(tile, &game);
  }
  printf("Bravo ! Le joueur %d a gagné :-)\n", game.currentPlayer);
  closeGame(&game);
}
*/