#pragma once

void authorize(const char *id, const char *pass);

void gameStart();

void gameOver();

void roundStart(int round);

void oneRound();

void roundOver(int round);

int observe();

void putDown(int row, int col);

void noStep();

void step();

void saveChessBoard();
