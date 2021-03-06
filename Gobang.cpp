#include "Define.h"
#include "Square.h"
#include "ClientSocket.h"
#include "Gobang.h"
#include <windows.h>
#include <iostream>
#include <stdlib.h>
#include <string>
#include <stdio.h>
#include <cmath>
using namespace std;

#define random(x) (rand()%x)
#define ROWS 15
#define COLS 15
#define ROUNDS 2

Square board[ROWS][COLS];
int ownColor = -1, oppositeColor = -1;
char lastmsg[16] = { "" };

bool Start = true;
int chessOrder[ROWS*COLS][2];//记录每一步的位置
int chessCount = 0;

struct Compete
{
	int row;
	int col;
	int myAdvan;
	int oppoAdvan;//是指在模拟对方落子时对方认为的棋盘上的优势度
	int relativeAdvan;//myAdvan - oppoAdvan
};

int stepCount = 0;

const int MaxTopStep = 10;//一次选择的最多优势步数走法
const int MaxLoopCount = 2;
//int tempOwnColor = ownColor;

void authorize(const char *id, const char *pass) {
	connectServer();
	std::cout << "Authorize " << id << std::endl;
	char msgBuf[BUFSIZE];
	memset(msgBuf, 0, BUFSIZE);
	msgBuf[0] = 'A';
	memcpy(&msgBuf[1], id, 9);
	memcpy(&msgBuf[10], pass, 6);
	int rtn = sendMsg(msgBuf);
	// printf("Authorize Return %d\n", rtn);
	if (rtn != 0) printf("Authorized Failed\n");
}

void gameStart() {
	/*char id[12], passwd[10];
	std::cout << "ID: " << std::endl;
	std::cin >> id;
	std::cout << "PASSWD: " << std::endl;
	std::cin >> passwd;*/
	authorize(ID, PASSWORD);
	std::cout << "Game Start" << std::endl;
	for (int round = 0; round < ROUNDS; round++) {
		roundStart(round);
		oneRound();
		roundOver(round);
	}
	gameOver();
	close();
}

void gameOver() {
	std::cout << "Game Over" << std::endl;
}

void roundStart(int round) {
	std::cout << "Round " << round << " Ready Start" << std::endl;
	for (int r = 0; r < ROWS; r++) {
		for (int c = 0; c < COLS; c++) {
			board[r][c].reset();
		}
	}
	memset(lastmsg, 0, sizeof(lastmsg));
	int rtn = recvMsg();
	if (rtn != 0) return;
	if (strlen(recvBuf) < 2)
		printf("Authorize Failed\n");
	else
		printf("Round start received msg %s\n", recvBuf);
	switch (recvBuf[1]) {
	case 'B':
		ownColor = 0;
		oppositeColor = 1;

		Start = true;//////////////////
		stepCount = 0;
		memset(chessOrder, 0, sizeof(int) * 2 * COLS * ROWS);
		chessCount = 0;

		rtn = sendMsg("BB");
		if (rtn != 0) return;
		break;
	case 'W':
		ownColor = 1;
		oppositeColor = 0;

		Start = true;//////////////////
		stepCount = 0;
		memset(chessOrder, 0, sizeof(int) * 2 * COLS * ROWS);
		chessCount = 0;

		rtn = sendMsg("BW");
		std::cout << "Send BW" << rtn << std::endl;
		if (rtn != 0) return;
		break;
	default:
		printf("Authorized Failed\n");
		break;
	}
}

void oneRound() {
	int DIS_FREQ = 5, STEP = 1;
	switch (ownColor) {
	case 0:
		while (STEP < 10000) {

			if (STEP != 1 && (STEP - 1) % DIS_FREQ == 0) {
				int ret = observe();       // self disappeared
				if (ret >= 1) break;
				else if (ret != -8) {
					std::cout << "ERROR: Not Self(BLACK) Disappeared" << std::endl;
				}
			}
			step();                        // take action, send message

			if (observe() >= 1) break;     // receive RET Code
										   // saveChessBoard();
			if (STEP != 1 && (STEP - 1) % DIS_FREQ == 0) {
				int ret = observe();       // see white disappear
				if (ret >= 1) break;
				else if (ret != -9) {
					std::cout << ret << " ERROR: Not White Disappeared" << std::endl;
				}
			}

			if (observe() >= 1) break;    // see white move
			STEP++;
			// saveChessBoard();
		}
		printf("One Round End\n");
		break;
	case 1:
		while (STEP < 10000) {

			if (STEP != 1 && (STEP - 1) % DIS_FREQ == 0) {
				int ret = observe();       // black disappeared
				if (ret >= 1) break;
				else if (ret != -8) {
					std::cout << "ERROR: Not Black Disappeared" << std::endl;
				}
			}
			if (observe() >= 1) break;    // see black move

			if (STEP != 1 && (STEP - 1) % DIS_FREQ == 0) {
				int ret = observe();      // self disappeared
				if (ret >= 1) break;
				else if (ret != -9) {
					std::cout << "ERROR: Not Self Disappeared" << std::endl;
				}
			}

			step();                        // take action, send message
			if (observe() >= 1) break;     // receive RET Code
										   // saveChessBoard();
			STEP++;
		}
		printf("One Round End\n");
		break;
	default:
		break;
	}
}

void roundOver(int round) {
	std::cout << "Round " << round << " Over" << std::endl;
	for (int r = 0; r < ROWS; r++) {
		for (int c = 0; c < COLS; c++) {
			board[r][c].reset();
		}
	}
	ownColor = oppositeColor = -1;
}

void lastMsg() {
	printf(lastmsg);
	puts("");
}

int observe() {
	int rtn = 0;
	int recvrtn = recvMsg();
	if (recvrtn != 0) return 1;
	printf("receive msg %s\n", recvBuf);
	switch (recvBuf[0]) {
	case 'R':   // return messages
	{
		switch (recvBuf[1]) {
		case '0':    // valid step
			switch (recvBuf[2]) {
			case 'P':   // update chessboard
			{
				int desRow = (recvBuf[3] - '0') * 10 + recvBuf[4] - '0';
				int desCol = (recvBuf[5] - '0') * 10 + recvBuf[6] - '0';

				chessOrder[chessCount][0] = desRow;
				chessOrder[chessCount][1] = desCol;/////////////////////////////////////
				chessCount++;

				board[desRow][desCol].color = recvBuf[7] - '0';
				board[desRow][desCol].empty = false;
				memcpy(lastmsg, recvBuf, strlen(recvBuf));
				break;
			}
			case 'D':   // Disappeared
			{
				int desRow = (recvBuf[3] - '0') * 10 + recvBuf[4] - '0';
				int desCol = (recvBuf[5] - '0') * 10 + recvBuf[6] - '0';
				board[desRow][desCol].color = -1;
				board[desRow][desCol].empty = true;
				if (recvBuf[7] - '0' == 0)  // black disappear
					rtn = -8;
				else
					rtn = -9;
				memcpy(lastmsg, recvBuf, strlen(recvBuf));
				break;
			}
			case 'N':   // R0N: enemy wrong step
			{
				break;
			}
			}
			break;
		case '1':
			std::cout << "Error -1: Msg format error\n";
			rtn = -1;
			break;
		case '2':
			std::cout << "Error -2: Coordinate error\n";
			rtn = -2;
			break;
		case '4':
			std::cout << "Error -4: Invalid step\n";
			rtn = -4;
			break;
		default:
			std::cout << "Error -5: Other error\n";
			rtn = -5;
			break;
		}
		break;
	}
	case 'E':
	{
		switch (recvBuf[1]) {
		case '0':
			// game over
			rtn = 2;
			break;
		case '1':
			// round over
			rtn = 1;
		default:
			break;
		}
		break;
	}
	}
	return rtn;
}

void putDown(int row, int col) {
	char msg[6];
	memset(msg, 0, sizeof(msg));
	msg[0] = 'S';
	msg[1] = 'P';
	msg[2] = '0' + row / 10;
	msg[3] = '0' + row % 10;
	msg[4] = '0' + col / 10;
	msg[5] = '0' + col % 10;
	board[row][col].color = ownColor;
	board[row][col].empty = false;
	lastMsg();
	printf("put down (%c%c, %c%c)\n", msg[2], msg[3], msg[4], msg[5]);
	sendMsg(msg);
}

void noStep() {
	sendMsg("SN");
	printf("send msg %s\n", "SN");
}

void sort(Compete *compete)
{
	for (int i = 0; i < MaxTopStep; i++)
	{
		int maxNumber = i;
		for (int j = i; j < ROWS*COLS; j++)
		{
			if (compete[maxNumber].relativeAdvan < compete[j].relativeAdvan)
			{
				maxNumber = j;
			}
		}

		swap(compete[maxNumber].col, compete[i].col);
		swap(compete[maxNumber].row, compete[i].row);
		swap(compete[maxNumber].myAdvan, compete[i].myAdvan);
		swap(compete[maxNumber].oppoAdvan, compete[i].oppoAdvan);
		swap(compete[maxNumber].relativeAdvan, compete[i].relativeAdvan);
	}
}

bool aroundExistChess(int col, int row)
{
	for (int i = 1; i <= 2; i++)
	{
		if ((row - i >= 0 && !board[row - i][col].empty) || (row + i < ROWS && !board[row + i][col].empty) || (col + i < COLS && !board[row][col + i].empty) || (col - i >= 0 && !board[row][col - i].empty) || (col - i >= 0 && row - i >= 0 && !board[row - i][col - i].empty) || (col + i < COLS && row + i < ROWS && !board[row + i][col + i].empty) || (col + i < COLS && row - i >= 0 && !board[row - i][col + i].empty) || (col - i >= 0 && row + i < ROWS && !board[row + i][col - i].empty))
		{
			return true;
		}
	}
	return false;
}

void aroundCount(int col, int row, int color, int &upCount, int &downCount, int &leftCount, int &rightCount, int &rightUpCount, int &leftUpCount, int &leftDownCount, int &rightDownCount)
{
	upCount = 0;//↑
	downCount = 0;//↓
	leftCount = 0;//←
	rightCount = 0;//→
	rightUpCount = 0;//↗
	leftUpCount = 0;//↖
	leftDownCount = 0;//↙
	rightDownCount = 0;//↘

	for (int i = 1; i <= 4; i++)
	{
		if ((row - i) >= 0 && !board[row - i][col].empty&&board[row - i][col].color == color)
		{
			upCount++;
		}
		if ((row + i) < ROWS && !board[row + i][col].empty&&board[row + i][col].color == color)
		{
			downCount++;
		}
		if ((col - i) >= 0 && !board[row][col - i].empty&&board[row][col - i].color == color)
		{
			leftCount++;
		}
		if ((col + i) < COLS && !board[row][col + i].empty&&board[row][col + i].color == color)
		{
			rightCount++;
		}
		if ((row - i) >= 0 && (col - i) >= 0 && !board[row - i][col - i].empty&&board[row - i][col - i].color == color)
		{
			leftUpCount++;
		}
		if ((row - i) >= 0 && (col + i) < COLS && !board[row - i][col + i].empty&&board[row - i][col + i].color == color)
		{
			rightUpCount++;
		}
		if ((row + i) < ROWS && (col - i) >= 0 && !board[row + i][col - i].empty&&board[row + i][col - i].color == color)
		{
			leftDownCount++;
		}
		if ((row + i) < ROWS && (col + i) < COLS && !board[row + i][col + i].empty&&board[row + i][col + i].color == color)
		{
			rightDownCount++;
		}
	}
}

int alive4Judge(int col, int row, int color, int &upCount, int &downCount, int &leftCount, int &rightCount, int &rightUpCount, int &leftUpCount, int &leftDownCount, int &rightDownCount)//有几个alive4就返回几
{
	int alive4Count = 0;

	if (upCount >= 4 && row - 5 >= 0 && (board[row - 5][col].empty || board[row - 5][col].color == color))//最后一个格子不能是对方的子
	{
		alive4Count++;
	}
	if (downCount >= 4 && row + 5 < ROWS && (board[row + 5][col].empty || board[row + 5][col].color == color))
	{
		alive4Count++;
	}
	if (leftCount >= 4 && col - 5 >= 0 && (board[row][col - 5].empty || board[row][col - 5].color == color))
	{
		alive4Count++;
	}
	if (rightCount >= 4 && col + 5 < COLS && (board[row][col + 5].empty || board[row][col + 5].color == color))
	{
		alive4Count++;
	}
	if (leftUpCount >= 4 && row - 5 >= 0 && col - 5 >= 0 && (board[row - 5][col - 5].empty || board[row - 5][col - 5].color == color))
	{
		alive4Count++;
	}
	if (rightUpCount >= 4 && row - 5 >= 0 && col + 5 < COLS && (board[row - 5][col + 5].empty || board[row - 5][col + 5].color == color))
	{
		alive4Count++;
	}
	if (leftDownCount >= 4 && row + 5 < ROWS && col - 5 >= 0 && (board[row + 5][col - 5].empty || board[row + 5][col - 5].color == color))
	{
		alive4Count++;
	}
	if (rightDownCount >= 4 && row + 5 < ROWS && col + 5 < COLS && (board[row + 5][col + 5].empty || board[row + 5][col + 5].color == color))
	{
		alive4Count++;
	}

	return alive4Count;
}

int attack4Judge(int col, int row, int color, int &upCount, int &downCount, int &leftCount, int &rightCount, int &rightUpCount, int &leftUpCount, int &leftDownCount, int &rightDownCount)
{
	int attack4Count = 0;

	if (upCount >= 4)
	{
		attack4Count++;
	}
	if (downCount >= 4)
	{
		attack4Count++;
	}
	if (leftCount >= 4)
	{
		attack4Count++;
	}
	if (rightCount >= 4)
	{
		attack4Count++;
	}
	if (leftUpCount >= 4)
	{
		attack4Count++;
	}
	if (rightUpCount >= 4)
	{
		attack4Count++;
	}
	if (leftDownCount >= 4)
	{
		attack4Count++;
	}
	if (rightDownCount >= 4)
	{
		attack4Count++;
	}

	return attack4Count;
}

int jump4Judge(int col, int row, int color, int &upCount, int &downCount, int &leftCount, int &rightCount, int &rightUpCount, int &leftUpCount, int &leftDownCount, int &rightDownCount)
{
	int jump4Count = 0;

	if (upCount < 4 && downCount < 4 && board[row + 1][col].color == color && board[row - 1][col].color == color && (board[row + 2][col].color == color || board[row - 2][col].color == color) && upCount + downCount >= 4)
	{
		if ((upCount >= 2 && downCount >= 2 && board[row + 2][col].color == color && board[row - 2][col].color == color) || upCount == 1 && board[row + 3][col].color == color || downCount == 1 && board[row - 3][col].color == color)
		{
			jump4Count++;
		}
	}
	if (leftCount < 4 && rightCount < 4 && board[row][col - 1].color == color && board[row][col + 1].color == color && (board[row][col + 2].color == color || board[row][col - 2].color == color) && leftCount + rightCount >= 4)
	{
		if ((leftCount >= 2 && rightCount >= 2 && board[row][col + 2].color == color && board[row][col - 2].color == color) || leftCount == 1 && board[row][col + 3].color == color || rightCount == 1 && board[row][col - 3].color == color)
		{
			jump4Count++;
		}
	}
	if (leftUpCount < 4 && rightDownCount < 4 && board[row - 1][col - 1].color == color && board[row + 1][col + 1].color == color && (board[row + 2][col + 2].color == color || board[row - 2][col - 2].color == color) && leftUpCount + rightDownCount >= 4)
	{
		if ((leftUpCount >= 2 && rightDownCount >= 2 && board[row + 2][col + 2].color == color && board[row - 2][col - 2].color == color) || (leftUpCount == 1 && board[row + 3][col + 3].color == color) || (rightDownCount == 1 && board[row - 3][col - 3].color == color))
		{
			jump4Count++;
		}
	}
	if (leftDownCount < 4 && rightUpCount < 4 && board[row + 1][col - 1].color == color && board[row - 1][col + 1].color == color && (board[row - 2][col + 2].color == color || board[row + 2][col - 2].color == color) && leftDownCount + rightUpCount >= 4)
	{
		if ((leftDownCount >= 2 && rightUpCount >= 2 && board[row - 2][col + 2].color == color && board[row + 2][col - 2].color == color) || (leftDownCount == 1 && board[row - 3][col + 3].color == color) || (rightUpCount == 1 && board[row + 3][col - 3].color == color))
		{
			jump4Count++;
		}
	}


	return jump4Count;
}

int alive3Judge(int col, int row, int color, int &upCount, int &downCount, int &leftCount, int &rightCount, int &rightUpCount, int &leftUpCount, int &leftDownCount, int &rightDownCount)
{
	int alive3Count = 0;

	if (upCount == 3 && row - 4 >= 0 && (row - 5 >= 0 || row + 1 < ROWS) && board[row - 4][col].empty && (board[row - 5][col].empty || board[row + 1][col].empty))//第4格必是空的（否则跳3或冲3）,最后一个括号内的判断条件是防止眠3
	{
		alive3Count++;
	}
	if (downCount == 3 && row + 4 < ROWS && (row - 1 >= 0 || row + 4 < ROWS) && board[row + 4][col].empty && (board[row + 5][col].empty || board[row - 1][col].empty))
	{
		alive3Count++;
	}
	if (leftCount == 3 && col - 4 >= 0 && (col - 5 >= 0 || col + 1 < ROWS) && board[row][col - 4].empty && (board[row][col - 5].empty || board[row][col + 1].empty))
	{
		alive3Count++;
	}
	if (rightCount == 3 && col + 4 < COLS && (col - 1 >= 0 || col + 5 < ROWS) && board[row][col + 4].empty && (board[row][col + 5].empty || board[row][col - 1].empty))
	{
		alive3Count++;
	}
	if (leftUpCount == 3 && row - 4 >= 0 && col - 4 >= 0 && ((row - 5 >= 0 && col - 5 >= 0) || (row + 1 < ROWS && col + 1 < COLS)) && board[row - 4][col - 4].empty && (board[row - 5][col - 5].empty || board[row + 1][col + 1].empty))
	{
		alive3Count++;
	}
	if (rightUpCount == 3 && row - 4 >= 0 && col + 4 < COLS && ((row - 5 >= 0 && col + 5 < COLS) || (row + 1 < ROWS && col - 1 >= 0)) && board[row - 4][col + 4].empty && (board[row - 5][col + 5].empty || board[row + 1][col - 1].empty))
	{
		alive3Count++;
	}
	if (leftDownCount == 3 && row + 4 < ROWS && col - 4 >= 0 && ((col - 5 >= 0 && row + 5 < ROWS) || (col + 1 < COLS && row - 1 >= 0)) && board[row + 4][col - 4].empty && (board[row + 5][col - 5].empty || board[row - 1][col + 1].empty))
	{
		alive3Count++;
	}
	if (rightDownCount == 3 && row + 4 < ROWS && col + 4 < COLS && ((row + 5 < ROWS && col + 5 < COLS) || (row - 1 >= 0 && col - 1 >= 0)) && board[row + 4][col + 4].empty && (board[row + 5][col + 5].empty || board[row - 1][col - 1].empty))
	{
		alive3Count++;
	}

	return alive3Count;
}

int jumpAlive3Judge(int col, int row, int color, int oppoColor, int &upCount, int &downCount, int &leftCount, int &rightCount, int &rightUpCount, int &leftUpCount, int &leftDownCount, int &rightDownCount)
{
	int jumpAlive3Count = 0;

	if ((upCount == 2 && downCount == 1 || upCount == 1 && downCount == 2) && board[row - 1][col].color == color && board[row + 1][col].color == color && (board[row + 2][col].color == color && row + 3 < ROWS && board[row + 3][col].color != oppoColor || board[row - 2][col].color == color && row - 3 >= 0 && board[row - 3][col].color != oppoColor) && !(board[row + 2][col].color == color && board[row - 2][col].color == color))
	{
		jumpAlive3Count++;
	}
	if ((leftCount == 2 && rightCount == 1 || leftCount == 1 && rightCount == 2) && board[row][col - 1].color == color && board[row][col + 1].color == color && (board[row][col + 2].color == color && col + 3 < COLS && board[row][col + 3].color != oppoColor || board[row][col - 2].color == color && col - 3 >= 0 && board[row][col - 3].color != oppoColor) && !(board[row][col + 2].color == color && board[row][col - 2].color == color))
	{
		jumpAlive3Count++;
	}
	if ((leftUpCount == 2 && rightDownCount == 1 || leftUpCount == 1 && rightDownCount == 2) && board[row - 1][col - 1].color == color && board[row + 1][col + 1].color == color && (board[row + 2][col + 2].color == color && col + 3 < COLS && row + 3 < ROWS && board[row + 3][col + 3].color != oppoColor || board[row - 2][col - 2].color == color && col - 3 >= 0 && row - 3 >= 0 && board[row - 3][col - 3].color != oppoColor) && !(board[row + 2][col + 2].color == color && board[row - 2][col - 2].color == color))
	{
		jumpAlive3Count++;
	}
	if ((leftDownCount == 2 && rightUpCount == 1 || leftDownCount == 1 && rightUpCount == 2) && board[row + 1][col - 1].color == color && board[row - 1][col + 1].color == color && (board[row - 2][col + 2].color == color && col + 3 < COLS && row - 3 >= 0 && board[row - 3][col + 3].color != oppoColor || board[row + 2][col - 2].color == color && col - 3 >= 0 && row + 3 < ROWS && board[row + 3][col - 3].color != oppoColor) && !(board[row - 2][col + 2].color == color && board[row + 2][col - 2].color == color))
	{
		jumpAlive3Count++;
	}


	return jumpAlive3Count;
}

int attack3Judge(int col, int row, int color, int &upCount, int &downCount, int &leftCount, int &rightCount, int &rightUpCount, int &leftUpCount, int &leftDownCount, int &rightDownCount)
{
	int attack3Count = 0;

	if (upCount == 3 && (row - 4 < 0 || (board[row - 4][col].color != color && !board[row - 4][col].empty)) && !(row + 1 >= ROWS || board[row + 1][col].color != color && !board[row + 1][col].empty))
	{
		attack3Count++;
	}
	if (downCount == 3 && (row + 4 >= ROWS || (board[row + 4][col].color != color && !board[row + 4][col].empty)) && !(row - 1 < 0 || (board[row - 1][col].color != color && !board[row - 1][col].empty)))
	{
		attack3Count++;
	}
	if (leftCount == 3 && (col - 4 < 0 || (board[row][col - 4].color != color && !board[row][col - 4].empty)) && !(col + 1 >= COLS || (board[row][col + 1].color != color && !board[row][col + 1].empty)))
	{
		attack3Count++;
	}
	if (rightCount == 3 && (col + 4 >= COLS || (board[row][col + 4].color != color && !board[row][col + 4].empty)) && !(col - 1 < 0 || (board[row][col - 1].color != color && !board[row][col - 1].empty)))
	{
		attack3Count++;
	}
	if (leftUpCount == 3 && (row - 4 < 0 || col - 4 < 0 || (board[row - 4][col - 4].color != color && !board[row - 4][col - 4].empty)) && !(row + 1 >= ROWS || col + 1 >= COLS || (board[row + 1][col + 1].color != color && !board[row + 1][col + 1].empty)))
	{
		attack3Count++;
	}
	if (rightUpCount == 3 && (row - 4 < 0 || col + 4 >= COLS || (board[row - 4][col + 4].color != color && !board[row - 4][col + 4].empty)) && !(row + 1 >= ROWS || col - 1 < 0 || (board[row + 1][col - 1].color != color && !board[row + 1][col - 1].empty)))
	{
		attack3Count++;
	}
	if (leftDownCount == 3 && (row + 4 >= ROWS || col - 4 < 0 || (board[row + 4][col - 4].color != color && !board[row + 4][col - 4].empty)) && !(row - 1 < 0 || col + 1 >= COLS || (board[row - 1][col + 1].color != color && !board[row - 1][col + 1].empty)))
	{
		attack3Count++;
	}
	if (rightDownCount == 3 && (row + 4 >= ROWS || col + 4 >= COLS || (board[row + 4][col + 4].color != color && !board[row + 4][col + 4].empty)) && !(row - 1 < 0 || col - 1 < 0 || (board[row - 1][col - 1].color != color && !board[row - 1][col - 1].empty)))
	{
		attack3Count++;
	}

	return attack3Count;
}

int jumpAttack3Judge(int col, int row, int color, int oppoColor, int &upCount, int &downCount, int &leftCount, int &rightCount, int &rightUpCount, int &leftUpCount, int &leftDownCount, int &rightDownCount)
{
	int jumpAttack3Count = 0;

	if ((upCount == 2 && downCount == 1 || upCount == 1 && downCount == 2) && board[row - 1][col].color == color && board[row + 1][col].color == color && (board[row + 2][col].color == color && (row + 3 >= ROWS || board[row + 3][col].color == oppoColor) && row - 2 >= 0 && board[row - 2][col].empty || board[row - 2][col].color == color && (row - 3 < 0 || board[row - 3][col].color == oppoColor) && row + 2 < ROWS && board[row + 2][col].empty) && !(board[row + 2][col].color == color && board[row - 2][col].color == color))
	{
		jumpAttack3Count++;
	}
	if ((leftCount == 2 && rightCount == 1 || leftCount == 1 && rightCount == 2) && board[row][col - 1].color == color && board[row][col + 1].color == color && (board[row][col + 2].color == color && (col + 3 >= COLS || board[row][col + 3].color == oppoColor) && col - 2 >= 0 && board[row][col - 2].empty || board[row][col - 2].color == color && (col - 3 < 0 || board[row][col - 3].color == oppoColor) && row + 2 < ROWS && board[row + 2][col].empty) && !(board[row][col + 2].color == color && board[row][col - 2].color == color))
	{
		jumpAttack3Count++;
	}
	if ((leftUpCount == 2 && rightDownCount == 1 || leftUpCount == 1 && rightDownCount == 2) && board[row - 1][col - 1].color == color && board[row + 1][col + 1].color == color && (board[row + 2][col + 2].color == color && (col + 3 >= COLS || row + 3 >= ROWS || board[row + 3][col + 3].color == oppoColor) && row - 2 >= 0 && col - 2 >= 0 && board[row - 2][col - 2].empty || board[row - 2][col - 2].color == color && (col - 3 < 0 || row - 3 < 0 || board[row - 3][col - 3].color != oppoColor) && row + 2 < ROWS && col + 2 < COLS && board[row + 2][col + 2].empty) && !(board[row + 2][col + 2].color == color && board[row - 2][col - 2].color == color))
	{
		jumpAttack3Count++;
	}
	if ((leftDownCount == 2 && rightUpCount == 1 || leftDownCount == 1 && rightUpCount == 2) && board[row + 1][col - 1].color == color && board[row - 1][col + 1].color == color && (board[row - 2][col + 2].color == color && (col + 3 >= COLS || row - 3 < 0 || board[row - 3][col + 3].color == oppoColor) && row + 2 < ROWS && col - 2 >= 0 && board[row + 2][col - 2].empty || board[row + 2][col - 2].color == color && (col - 3 < 0 || row + 3 >= ROWS || board[row + 3][col - 3].color == oppoColor) && row - 2 > 0 && col + 2 < COLS && board[row - 2][col + 2].empty) && !(board[row - 2][col + 2].color == color && board[row + 2][col - 2].color == color))
	{
		jumpAttack3Count++;
	}


	return jumpAttack3Count;
}

int sleep3Judge1(int col, int row, int color, int oppoColor, int &upCount, int &downCount, int &leftCount, int &rightCount, int &rightUpCount, int &leftUpCount, int &leftDownCount, int &rightDownCount)
{
	int sleep3Count = 0;

	if ((upCount == 2 && downCount == 1 && board[row - 1][col].color == color && board[row - 2][col].color == color && board[row + 1][col].empty && board[row + 2][col].color == color && row + 3 < ROWS && row - 3 >= 0 && board[row + 3][col].empty && board[row - 3][col].empty) || (upCount == 1 && downCount == 2 && board[row + 1][col].color == color && board[row + 2][col].color == color && board[row - 1][col].empty && board[row - 2][col].color == color && row + 3 < ROWS && row - 3 >= 0 && board[row + 3][col].empty && board[row - 3][col].empty))
	{
		sleep3Count++;
	}
	if ((leftCount == 2 && rightCount == 1 && board[row][col - 1].color == color && board[row][col - 2].color == color && board[row][col + 1].empty && board[row][col + 2].color == color && col + 3 < COLS && col - 3 >= 0 && board[row][col + 3].empty && board[row][col - 3].empty) || (leftCount == 1 && rightCount == 2 && board[row][col + 1].color == color && board[row][col + 2].color == color && board[row][col - 1].empty && board[row][col - 2].color == color && col + 3 < COLS && col - 3 >= 0 && board[row][col + 3].empty && board[row][col].empty))
	{
		sleep3Count++;
	}
	if ((leftUpCount == 2 && rightDownCount == 1 && board[row - 1][col - 1].color == color && board[row - 2][col - 2].color == color && board[row + 1][col + 1].empty && board[row + 2][col + 2].color == color && col + 3 < COLS && row + 3 < ROWS && col - 3 >= 0 && row - 3 >= 0 && board[row + 3][col + 3].empty && board[row - 3][col - 3].empty) || (leftUpCount == 1 && rightDownCount == 2 && board[row + 1][col + 1].color == color && board[row + 2][col + 2].color == color && board[row - 1][col - 1].empty && board[row - 2][col - 2].color == color && col + 3 < COLS && row + 3 < ROWS && col - 3 >= 0 && row - 3 >= 0 && board[row + 3][col + 3].empty && board[row - 3][col - 3].empty))
	{
		sleep3Count++;
	}
	if ((leftDownCount == 2 && rightUpCount == 1 && board[row + 1][col - 1].color == color && board[row + 2][col - 2].color == color && board[row - 1][col + 1].empty && board[row - 2][col + 2].color == color && col + 3 < COLS && row + 3 < ROWS && col - 3 >= 0 && row - 3 >= 0 && board[row - 3][col + 3].empty && board[row + 3][col - 3].empty) || (leftDownCount == 1 && rightUpCount == 2 && board[row - 1][col + 1].color == color && board[row - 2][col + 2].color == color && board[row + 1][col - 1].empty && board[row + 2][col - 2].color == color && col + 3 < COLS && row + 3 < ROWS && col - 3 >= 0 && row - 3 >= 0 && board[row - 3][col + 3].empty && board[row + 3][col - 3].empty))
	{
		sleep3Count++;
	}

	return sleep3Count;
}

int sleep3Judge2(int col, int row, int color, int oppoColor, int &upCount, int &downCount, int &leftCount, int &rightCount, int &rightUpCount, int &leftUpCount, int &leftDownCount, int &rightDownCount)
{
	int sleep3Count = 0;

	if ((upCount == 2 && downCount == 1 && row - 3 >= 0 && board[row - 1][col].color == color && board[row - 2][col].empty && board[row - 3][col].color == color && board[row + 1][col].color == color) || (upCount == 1 && downCount == 2 && row + 3 < ROWS && board[row + 1][col].color == color && board[row + 2][col].empty && board[row + 3][col].color == color && board[row - 1][col].color == color))
	{
		sleep3Count++;
	}
	if ((leftCount == 2 && rightCount == 1 && col - 3 >= 0 && board[row][col - 1].color == color && board[row][col - 2].empty && board[row][col - 3].color == color && board[row][col + 1].color == color) || (leftCount == 1 && rightCount == 2 && col + 3 < COLS && board[row][col + 1].color == color && board[row][col + 2].empty && board[row][col + 3].color == color && board[row][col - 1].color == color))
	{
		sleep3Count++;
	}
	if ((leftUpCount == 2 && rightDownCount == 1 && col - 3 >= 0 && row - 3 >= 0 && board[row - 1][col - 1].color == color && board[row - 2][col - 2].empty && board[row - 3][col - 3].color == color && board[row + 1][col + 1].color == color) || (leftUpCount == 1 && rightDownCount == 2 && col + 3 < COLS  && row + 3 < ROWS && board[row + 1][col + 1].color == color && board[row + 2][col + 2].empty && board[row + 3][col + 3].color == color && board[row - 1][col - 1].color == color))
	{
		sleep3Count++;
	}
	if ((leftDownCount == 2 && rightUpCount == 1 && col - 3 >= 0 && row + 3 >= 0 && board[row + 1][col - 1].color == color && board[row + 2][col - 2].empty && board[row + 3][col - 3].color == color && board[row - 1][col + 1].color == color) || (leftDownCount == 1 && rightUpCount == 2 && col + 3 < COLS  && row + 3 < ROWS && board[row - 1][col + 1].color == color && board[row - 2][col + 2].empty && board[row - 3][col + 3].color == color && board[row + 1][col - 1].color == color))
	{
		sleep3Count++;
	}

	return sleep3Count;
}

int die3Judge(int col, int row, int color, int &upCount, int &downCount, int &leftCount, int &rightCount, int &rightUpCount, int &leftUpCount, int &leftDownCount, int &rightDownCount)
{
	int die3Count = 0;

	if (upCount == 3 && (row - 4 < 0 || (board[row - 4][col].color != color && !board[row - 4][col].empty)) && (row + 1 >= ROWS || board[row + 1][col].color != color && !board[row + 1][col].empty))
	{
		die3Count++;
	}
	if (downCount == 3 && (row + 4 >= ROWS || (board[row + 4][col].color != color && !board[row + 4][col].empty)) && (row - 1 < 0 || (board[row - 1][col].color != color && !board[row - 1][col].empty)))
	{
		die3Count++;
	}
	if (leftCount == 3 && (col - 4 < 0 || (board[row][col - 4].color != color && !board[row][col - 4].empty)) && (col + 1 >= COLS || (board[row][col + 1].color != color && !board[row][col + 1].empty)))
	{
		die3Count++;
	}
	if (rightCount == 3 && (col + 4 >= COLS || (board[row][col + 4].color != color && !board[row][col + 4].empty)) && (col - 1 < 0 || (board[row][col - 1].color != color && !board[row][col - 1].empty)))
	{
		die3Count++;
	}
	if (leftUpCount == 3 && (row - 4 < 0 || col - 4 < 0 || (board[row - 4][col - 4].color != color && !board[row - 4][col - 4].empty)) && (row + 1 >= ROWS || col + 1 >= COLS || (board[row + 1][col + 1].color != color && !board[row + 1][col + 1].empty)))
	{
		die3Count++;
	}
	if (rightUpCount == 3 && (row - 4 < 0 || col + 4 >= COLS || (board[row - 4][col + 4].color != color && !board[row - 4][col + 4].empty)) && (row + 1 >= ROWS || col - 1 < 0 || (board[row + 1][col - 1].color != color && !board[row + 1][col - 1].empty)))
	{
		die3Count++;
	}
	if (leftDownCount == 3 && (row + 4 >= ROWS || col - 4 < 0 || (board[row + 4][col - 4].color != color && !board[row + 4][col - 4].empty)) && (row - 1 < 0 || col + 1 >= COLS || (board[row - 1][col + 1].color != color && !board[row - 1][col + 1].empty)))
	{
		die3Count++;
	}
	if (rightDownCount == 3 && (row + 4 >= ROWS || col + 4 >= COLS || (board[row + 4][col + 4].color != color && !board[row + 4][col + 4].empty)) && (row - 1 < 0 || col - 1 < 0 || (board[row - 1][col - 1].color != color && !board[row - 1][col - 1].empty)))
	{
		die3Count++;
	}

	return die3Count;
}

int another3Judge(int col, int row, int color, int &upCount, int &downCount, int &leftCount, int &rightCount, int &rightUpCount, int &leftUpCount, int &leftDownCount, int &rightDownCount)
{
	int another3Count = 0;

	if (upCount == 3)
	{
		another3Count++;
	}
	if (downCount == 3)
	{
		another3Count++;
	}
	if (leftCount == 3)
	{
		another3Count++;
	}
	if (rightCount == 3)
	{
		another3Count++;
	}
	if (leftUpCount == 3)
	{
		another3Count++;
	}
	if (rightUpCount == 3)
	{
		another3Count++;
	}
	if (leftDownCount == 3)
	{
		another3Count++;
	}
	if (rightDownCount == 3)
	{
		another3Count++;
	}

	return another3Count;
}

int alive2Judge(int col, int row, int color, int &upCount, int &downCount, int &leftCount, int &rightCount, int &rightUpCount, int &leftUpCount, int &leftDownCount, int &rightDownCount)
{
	int alive2Count = 0;

	if (upCount == 2 && row - 4 >= 0 && (row - 5 >= 0 || row + 1 < ROWS) && board[row - 3][col].empty  && board[row - 4][col].empty && (board[row - 5][col].empty || board[row + 1][col].empty))
	{
		alive2Count++;
	}
	if (downCount == 2 && row + 4 < ROWS && (row - 1 >= 0 || row + 4 < ROWS) && board[row + 3][col].empty  && board[row + 4][col].empty && (board[row + 5][col].empty || board[row - 1][col].empty))
	{
		alive2Count++;
	}
	if (leftCount == 2 && col - 4 >= 0 && (col - 5 >= 0 || col + 1 < ROWS) && board[row][col - 3].empty && board[row][col - 4].empty && (board[row][col - 5].empty || board[row][col + 1].empty))
	{
		alive2Count++;
	}
	if (rightCount == 2 && col + 4 < COLS && (col - 1 >= 0 || col + 5 < COLS) && board[row][col + 3].empty && board[row][col + 4].empty && (board[row][col + 5].empty || board[row][col - 1].empty))
	{
		alive2Count++;
	}
	if (leftUpCount == 2 && row - 4 >= 0 && col - 4 >= 0 && ((row - 5 >= 0 && col - 5 >= 0) || (row + 1 < ROWS && col + 1 < COLS)) && board[row - 3][col - 3].empty  && board[row - 4][col - 4].empty && (board[row - 5][col - 5].empty || board[row + 1][col + 1].empty))
	{
		alive2Count++;
	}
	if (rightUpCount == 2 && row - 4 >= 0 && col + 4 < COLS && ((row - 5 >= 0 && col + 5 < COLS) || (row + 1 < ROWS && col - 1 >= 0)) && board[row - 3][col + 3].empty && board[row - 4][col + 4].empty && (board[row - 5][col + 5].empty || board[row + 1][col - 1].empty))
	{
		alive2Count++;
	}
	if (leftDownCount == 2 && row + 4 < ROWS && col - 4 >= 0 && ((col - 5 >= 0 && row + 5 < ROWS) || (col + 1 < COLS && row - 1 >= 0)) && board[row + 3][col - 3].empty && board[row + 4][col - 4].empty && (board[row + 5][col - 5].empty || board[row - 1][col + 1].empty))
	{
		alive2Count++;
	}
	if (rightDownCount == 2 && row + 4 < ROWS && col + 4 < COLS && ((row + 5 < ROWS && col + 5 < COLS) || (row - 1 >= 0 && col - 1 >= 0)) && board[row + 3][col + 3].empty && board[row + 4][col + 4].empty && (board[row + 5][col + 5].empty || board[row - 1][col - 1].empty))
	{
		alive2Count++;
	}

	return alive2Count;
}

int jumpAlive2Judge(int col, int row, int color, int oppoColor, int &upCount, int &downCount, int &leftCount, int &rightCount, int &rightUpCount, int &leftUpCount, int &leftDownCount, int &rightDownCount)
{
	int jumpAlive2Count = 0;

	if (upCount == 1 && downCount == 1 && row + 1 < ROWS && row - 1 >= 0 && board[row + 1][col].color == color && board[row - 1][col].color == color && ((row + 2 < ROWS && board[row + 2][col].empty) && (row - 2 >= 0 && board[row - 2][col].empty)) && ((row + 3 < ROWS && board[row + 3][col].empty) || (row - 3 >= 0 && board[row - 3][col].empty)))
	{
		jumpAlive2Count++;
	}
	if (leftCount == 1 && rightCount == 1 && col + 1 < ROWS && col - 1 >= 0 && board[row][col + 1].color == color && board[row][col - 1].color == color && ((col + 2 < COLS && board[row][col + 2].empty) && (col - 2 >= 0 && board[row][col - 2].empty)) && ((col + 3 < COLS && board[row][col + 3].empty) || (col - 3 >= 0 && board[row][col - 3].empty)))
	{
		jumpAlive2Count++;
	}
	if (rightDownCount == 1 && leftUpCount == 1 && row + 1 < ROWS && row - 1 >= 0 && col + 1 < ROWS && col - 1 >= 0 && board[row + 1][col + 1].color == color && board[row - 1][col - 1].color == color && ((col + 2 < COLS && row + 2 < ROWS && board[row + 2][col + 2].empty) && (col - 2 >= 0 && row - 2 >= 0 && board[row - 2][col - 2].empty)) && ((col + 3 < COLS && row + 3 < ROWS && board[row + 3][col + 3].empty) || (col - 3 >= 0 && row - 3 >= 0 && board[row - 3][col - 3].empty)))
	{
		jumpAlive2Count++;
	}
	if (leftDownCount == 1 && rightUpCount == 1 && row + 1 < ROWS && row - 1 >= 0 && col + 1 < ROWS && col - 1 >= 0 && board[row - 1][col + 1].color == color && board[row + 1][col - 1].color == color && ((col + 2 < COLS && row - 2 < ROWS && board[row - 2][col + 2].empty) && (col - 2 >= 0 && row + 2 >= 0 && board[row + 2][col - 2].empty)) && ((col + 3 < COLS && row - 3 < ROWS && board[row - 3][col + 3].empty) || (col - 3 >= 0 && row + 3 >= 0 && board[row + 3][col - 3].empty)))
	{
		jumpAlive2Count++;
	}
	return jumpAlive2Count;
}

int attack2Judge(int col, int row, int color, int oppoColor, int &upCount, int &downCount, int &leftCount, int &rightCount, int &rightUpCount, int &leftUpCount, int &leftDownCount, int &rightDownCount)
{
	int attack2Count = 0;

	if (upCount == 2 && row + 1 < ROWS && row - 1 >= 0 && board[row - 1][col].color == color && (board[row - 3][col].color == oppoColor || row - 3 < 0) && board[row + 1][col].empty)
	{
		attack2Count++;
	}
	if (downCount == 2 && row - 1 >= 0 && row + 1 < ROWS && board[row + 1][col].color == color && (board[row + 3][col].color == oppoColor || row + 3 >= ROWS) && board[row - 1][col].empty)
	{
		attack2Count++;
	}
	if (rightCount == 2 && col - 1 >= 0 && col + 1 < COLS && board[row][col + 1].color == color && (board[row][col + 3].color == oppoColor || col + 3 >= ROWS) && board[row][col - 1].empty)
	{
		attack2Count++;
	}
	if (leftCount == 2 && col + 1 < COLS && col - 1 >= 0 && board[row][col - 1].color == color && (board[row][col - 3].color == oppoColor || col - 3 < 0) && board[row][col + 1].empty)
	{
		attack2Count++;
	}
	if (leftUpCount == 2 && col + 1 < COLS && row + 1 < ROWS && col - 1 >= 0 && row - 1 >= 0 && board[row - 1][col - 1].color == color && (board[row - 3][col - 3].color == oppoColor || row - 3 < 0 || col - 3 < 0) && board[row + 1][col + 1].empty)
	{
		attack2Count++;
	}
	if (rightUpCount == 2 && col - 1 >= 0 && row + 1 < ROWS && col + 1 < COLS && row - 1 >= 0 && board[row - 1][col + 1].color == color && (board[row - 3][col + 3].color == oppoColor || row - 3 < 0 || col + 3 >= COLS) && board[row + 1][col - 1].empty)
	{
		attack2Count++;
	}
	if (leftDownCount == 2 && col + 1 < COLS && row - 1 >= 0 && col - 1 >= 0 && row + 1 < ROWS && board[row + 1][col - 1].color == color && (board[row + 3][col - 3].color == oppoColor || row + 3 >= ROWS || col - 3 < 0) && board[row - 1][col + 1].empty)
	{
		attack2Count++;
	}
	if (rightDownCount == 2 && col - 1 >= 0 && row - 1 >= 0 && col + 1 < COLS && row + 1 < ROWS && board[row + 1][col + 1].color == color && (board[row + 3][col + 3].color == oppoColor || row + 3 >= ROWS || col + 3 > COLS) && board[row - 1][col - 1].empty)
	{
		attack2Count++;
	}

	return attack2Count;
}

int jumpAttack2Judge(int col, int row, int color, int oppoColor, int &upCount, int &downCount, int &leftCount, int &rightCount, int &rightUpCount, int &leftUpCount, int &leftDownCount, int &rightDownCount)
{
	int jumpAttack2Count = 0;

	if (upCount == 1 && downCount == 1 && row + 1 < ROWS && row - 1 >= 0 && board[row + 1][col].color == color && board[row - 1][col].color == color && (((row + 2 >= ROWS || board[row + 2][col].color == oppoColor) && (row - 3 >= 0 && board[row - 3][col].empty)) || ((row - 2 < ROWS || board[row - 2][col].color == oppoColor) && (row + 3 < ROWS && board[row + 3][col].empty))))
	{
		jumpAttack2Count++;
	}
	if (leftCount == 1 && rightCount == 1 && col + 1 < ROWS && col - 1 >= 0 && board[row][col + 1].color == color && board[row][col - 1].color == color && (((col + 2 >= COLS || board[row][col + 2].color == oppoColor) && (col - 3 >= 0 && board[row][col - 3].empty)) || ((col - 2 < COLS || board[row][col - 2].color == oppoColor) && (col + 3 < COLS && board[row][col + 3].empty))))
	{
		jumpAttack2Count++;
	}
	if (rightDownCount == 1 && leftUpCount == 1 && row + 1 < ROWS && row - 1 >= 0 && col + 1 < ROWS && col - 1 >= 0 && board[row + 1][col + 1].color == color && board[row - 1][col - 1].color == color && (((col + 2 >= COLS || row + 2 >= ROWS || board[row + 2][col + 2].color == oppoColor) && (col - 3 >= 0 && row - 3 >= 0 && board[row - 3][col - 3].empty)) || ((col - 2 < 0 || row - 2 < 0 || board[row - 2][col - 2].color == oppoColor) && (col + 3 < COLS && row + 3 < ROWS && board[row + 3][col + 3].empty))))
	{
		jumpAttack2Count++;
	}
	if (leftDownCount == 1 && rightUpCount == 1 && row + 1 < ROWS && row - 1 >= 0 && col + 1 < ROWS && col - 1 >= 0 && board[row - 1][col + 1].color == color && board[row + 1][col - 1].color == color && (((col + 2 >= COLS || row - 2 < 0 || board[row - 2][col + 2].color == oppoColor) && (col - 3 >= 0 && row + 3 < ROWS && board[row + 3][col - 3].empty)) || ((col - 2 < 0 || row + 2 >= ROWS || board[row + 2][col - 2].color == oppoColor) && (col + 3 < COLS && row - 3 >= 0 && board[row - 3][col + 3].empty))))
	{
		jumpAttack2Count++;
	}
	return jumpAttack2Count;
}

int another2Judge(int col, int row, int color, int &upCount, int &downCount, int &leftCount, int &rightCount, int &rightUpCount, int &leftUpCount, int &leftDownCount, int &rightDownCount)
{
	int another2Count = 0;

	if (upCount == 2)
	{
		another2Count++;
	}
	if (downCount == 2)
	{
		another2Count++;
	}
	if (leftCount == 2)
	{
		another2Count++;
	}
	if (rightCount == 2)
	{
		another2Count++;
	}
	if (leftUpCount == 2)
	{
		another2Count++;
	}
	if (rightUpCount == 2)
	{
		another2Count++;
	}
	if (leftDownCount == 2)
	{
		another2Count++;
	}
	if (rightDownCount == 2)
	{
		another2Count++;
	}

	return another2Count;
}

int alive1Judge(int col, int row, int color, int &upCount, int &downCount, int &leftCount, int &rightCount, int &rightUpCount, int &leftUpCount, int &leftDownCount, int &rightDownCount)
{
	int alive1Count = 0;

	if (upCount == 1 && row - 4 >= 0 && (row - 5 >= 0 || row + 1 < ROWS) && board[row - 2][col].empty && board[row - 3][col].empty && board[row - 4][col].empty && (board[row - 5][col].empty || board[row + 1][col].empty))
	{
		alive1Count++;
	}
	if (downCount == 1 && row + 4 < ROWS && (row - 1 >= 0 || row + 4 < ROWS) && board[row + 2][col].empty && board[row + 3][col].empty && board[row + 4][col].empty && (board[row + 5][col].empty || board[row - 1][col].empty))
	{
		alive1Count++;
	}
	if (leftCount == 1 && col - 4 >= 0 && (col - 5 >= 0 || col + 1 < ROWS) && board[row][col - 2].empty && board[row][col - 3].empty && board[row][col - 4].empty && (board[row][col - 5].empty || board[row][col + 1].empty))
	{
		alive1Count++;
	}
	if (rightCount == 1 && col + 4 < COLS && (col - 1 >= 0 || col + 5 < COLS) && board[row][col + 2].empty && board[row][col + 3].empty && board[row][col + 4].empty && (board[row][col + 5].empty || board[row][col - 1].empty))
	{
		alive1Count++;
	}
	if (leftUpCount == 1 && row - 4 >= 0 && col - 4 >= 0 && ((row - 5 >= 0 && col - 5 >= 0) || (row + 1 < ROWS && col + 1 < COLS)) && board[row - 2][col - 2].empty && board[row - 3][col - 3].empty && board[row - 4][col - 4].empty && (board[row - 5][col - 5].empty || board[row + 1][col + 1].empty))
	{
		alive1Count++;
	}
	if (rightUpCount == 1 && row - 4 >= 0 && col + 4 < COLS && ((row - 5 >= 0 && col + 5 < COLS) || (row + 1 < ROWS && col - 1 >= 0)) && board[row - 2][col + 2].empty && board[row - 3][col + 3].empty && board[row - 4][col + 4].empty && (board[row - 5][col + 5].empty || board[row + 1][col - 1].empty))
	{
		alive1Count++;
	}
	if (leftDownCount == 1 && row + 4 < ROWS && col - 4 >= 0 && ((col - 5 >= 0 && row + 5 < ROWS) || (col + 1 < COLS && row - 1 >= 0)) && board[row + 2][col - 2].empty && board[row + 3][col - 3].empty && board[row + 4][col - 4].empty && (board[row + 5][col - 5].empty || board[row - 1][col + 1].empty))
	{
		alive1Count++;
	}
	if (rightDownCount == 1 && row + 4 < ROWS && col + 4 < COLS && ((row + 5 < ROWS && col + 5 < COLS) || (row - 1 >= 0 && col - 1 >= 0)) && board[row + 2][col + 2].empty && board[row + 3][col + 3].empty && board[row + 4][col + 4].empty && (board[row + 5][col + 5].empty || board[row - 1][col - 1].empty))
	{
		alive1Count++;
	}

	return alive1Count;
}

int another1Judge(int col, int row, int color, int &upCount, int &downCount, int &leftCount, int &rightCount, int &rightUpCount, int &leftUpCount, int &leftDownCount, int &rightDownCount)
{
	int another1Count = 0;

	if (upCount == 1)
	{
		another1Count++;
	}
	if (downCount == 1)
	{
		another1Count++;
	}
	if (leftCount == 1)
	{
		another1Count++;
	}
	if (rightCount == 1)
	{
		another1Count++;
	}
	if (leftUpCount == 1)
	{
		another1Count++;
	}
	if (rightUpCount == 1)
	{
		another1Count++;
	}
	if (leftDownCount == 1)
	{
		another1Count++;
	}
	if (rightDownCount == 1)
	{
		another1Count++;
	}

	return another1Count;
}

int winJudge(int col, int row, int color, int &upCount, int &downCount, int &leftCount, int &rightCount, int &rightUpCount, int &leftUpCount, int &leftDownCount, int &rightDownCount)//有几个alive4就返回几
{
	int winCount = 0;

	if (board[row][col].color == color&&board[row + 1][col].color == color&&board[row + 2][col].color == color&&board[row - 1][col].color == color&&board[row - 2][col].color == color)
	{
		winCount++;
	}
	if (board[row][col].color == color&&board[row][col + 1].color == color&&board[row][col + 2].color == color&&board[row][col - 1].color == color&&board[row][col - 2].color == color)
	{
		winCount++;
	}
	if (board[row][col].color == color&&board[row + 1][col + 1].color == color&&board[row + 2][col + 2].color == color&&board[row - 1][col - 1].color == color&&board[row - 2][col - 2].color == color)
	{
		winCount++;
	}
	if (board[row][col].color == color&&board[row - 1][col + 1].color == color&&board[row - 2][col + 2].color == color&&board[row + 1][col - 1].color == color&&board[row + 2][col - 2].color == color)
	{
		winCount++;
	}

	return winCount;
}

void CalculateAdvantage(int myAdvan[ROWS][COLS], int oppoAdvan[ROWS][COLS])
{
	memset(myAdvan, 0, sizeof(int)*ROWS*COLS);
	memset(oppoAdvan, 0, sizeof(int)*ROWS*COLS);

	//int win = 1000000;
	//int doubleAlive4 = 200000;//被alive4包含
	int myAlive4 = 200000;
	//int doubleAttack4 = 100000;//被attack4包含
	//int attack4Alive3 = 90000;
	int myAttack4 = 180000;
	int myJump4 = 177000;
	//int jumpAttack4 = 75000;
	//int attack3alive3 = 75000;
	int myAlive3 = 60000;
	int myJumpAlive3 = 55000;

	int myAttack3 = 18000;
	int mySleep3 = 17000;
	int myJumpAttack3 = 17000;
	int myDie3 = 600;//是指一端封住，另一端间隔一空格后被封住（只能形成死4）
	int myAnother3 = 775;

	int myAlive2 = 22500;//双二必是可以走出双三的，所以alive3 > alive2 * 2 > sleep3
	int myJumpAlive2 = 20500;
	int myAttack2 = 8000;
	int myJumpAttack2 = 4000;
	int myAnother2 = 500;

	int myAlive1 = 1025;
	int myAnother1 = 225;

	int oppoAlive4 = 80000;
	int oppoAttack4 = 70000;
	int oppoJump4 = 65000;

	int oppoAlive3 = 45000;
	int oppoJumpAlive3 = 40000;

	int oppoAttack3 = 19000;
	int oppoSleep3 = 18000;
	int oppoJumpAttack3 = 18000;
	int oppoDie3 = 600;//是指一端封住，另一端间隔一空格后被封住（只能形成死4）
	int oppoAnother3 = 725;

	int oppoAlive2 = 21000;
	int oppoJumpAlive2 = 20000;
	int oppoAttack2 = 8000;
	int oppoJumpAttack2 = 4000;
	int oppoAnother2 = 425;

	int oppoAlive1 = 1000;
	int oppoAnother1 = 200;

	int nothing = 1;


	int myUpCount = 0;//↑
	int myDownCount = 0;//↓
	int myLeftCount = 0;//←
	int myRightCount = 0;//→
	int myRightUpCount = 0;//↗
	int myLeftUpCount = 0;//↖
	int myLeftDownCount = 0;//↙
	int myRightDownCount = 0;//↘

	int oppoUpCount = 0;//↑
	int oppoDownCount = 0;//↓
	int oppoLeftCount = 0;//←
	int oppoRightCount = 0;//→
	int oppoRightUpCount = 0;//↗
	int oppoLeftUpCount = 0;//↖
	int oppoLeftDownCount = 0;//↙
	int oppoRightDownCount = 0;//↘

	for (int i = 0; i < ROWS; i++)
	{
		for (int j = 0; j < COLS; j++)
		{
			if (board[i][j].empty)
			{
				myAdvan[i][j] = myAdvan[i][j] + (14 - (abs(7 - i) + abs(7 - j))) * nothing;
				oppoAdvan[i][j] = oppoAdvan[i][j] + (14 - (abs(7 - i) + abs(7 - j))) * nothing;

				if (aroundExistChess(j, i))
				{
					aroundCount(j, i, ownColor, myUpCount, myDownCount, myLeftCount, myRightCount, myRightUpCount, myLeftUpCount, myLeftDownCount, myRightDownCount);
					aroundCount(j, i, oppositeColor, oppoUpCount, oppoDownCount, oppoLeftCount, oppoRightCount, oppoRightUpCount, oppoLeftUpCount, oppoLeftDownCount, oppoRightDownCount);
					int oppoAlive4Count = alive4Judge(j, i, oppositeColor, oppoUpCount, oppoDownCount, oppoLeftCount, oppoRightCount, oppoRightUpCount, oppoLeftUpCount, oppoLeftDownCount, oppoRightDownCount);
					int myAlive4Count = alive4Judge(j, i, ownColor, myUpCount, myDownCount, myLeftCount, myRightCount, myRightUpCount, myLeftUpCount, myLeftDownCount, myRightDownCount);
					//myAdvan[i][j] += alive4*alive4Judge(j, i, ownColor);
					//oppoAdvan[i][j] += alive4*alive4Judge(j, i, oppositeColor);
					myAdvan[i][j] += myAlive4*myAlive4Count;
					oppoAdvan[i][j] += oppoAlive4*oppoAlive4Count;

					if (myAlive4Count == 0)//无活4
					{
						myAdvan[i][j] += myAttack4*attack4Judge(j, i, ownColor, myUpCount, myDownCount, myLeftCount, myRightCount, myRightUpCount, myLeftUpCount, myLeftDownCount, myRightDownCount);
						myAdvan[i][j] += myJump4*jump4Judge(j, i, ownColor, myUpCount, myDownCount, myLeftCount, myRightCount, myRightUpCount, myLeftUpCount, myLeftDownCount, myRightDownCount);

					}
					if (oppoAlive4Count == 0)
					{
						oppoAdvan[i][j] += oppoAttack4*attack4Judge(j, i, oppositeColor, oppoUpCount, oppoDownCount, oppoLeftCount, oppoRightCount, oppoRightUpCount, oppoLeftUpCount, oppoLeftDownCount, oppoRightDownCount);
						oppoAdvan[i][j] += oppoJump4*jump4Judge(j, i, oppositeColor, oppoUpCount, oppoDownCount, oppoLeftCount, oppoRightCount, oppoRightUpCount, oppoLeftUpCount, oppoLeftDownCount, oppoRightDownCount);
					}

					int myAlive3Count = alive3Judge(j, i, ownColor, myUpCount, myDownCount, myLeftCount, myRightCount, myRightUpCount, myLeftUpCount, myLeftDownCount, myRightDownCount);
					int oppoAlive3Count = alive3Judge(j, i, oppositeColor, oppoUpCount, oppoDownCount, oppoLeftCount, oppoRightCount, oppoRightUpCount, oppoLeftUpCount, oppoLeftDownCount, oppoRightDownCount);
					int myAttack3Count = attack3Judge(j, i, ownColor, myUpCount, myDownCount, myLeftCount, myRightCount, myRightUpCount, myLeftUpCount, myLeftDownCount, myRightDownCount);
					int oppoAttack3Count = attack3Judge(j, i, oppositeColor, oppoUpCount, oppoDownCount, oppoLeftCount, oppoRightCount, oppoRightUpCount, oppoLeftUpCount, oppoLeftDownCount, oppoRightDownCount);
					int myJumpAlive3Count = jumpAlive3Judge(j, i, ownColor, oppositeColor, myUpCount, myDownCount, myLeftCount, myRightCount, myRightUpCount, myLeftUpCount, myLeftDownCount, myRightDownCount);
					int oppoJumpAlive3Count = jumpAlive3Judge(j, i, oppositeColor, ownColor, oppoUpCount, oppoDownCount, oppoLeftCount, oppoRightCount, oppoRightUpCount, oppoLeftUpCount, oppoLeftDownCount, oppoRightDownCount);
					int myJumpAttack3Count = jumpAttack3Judge(j, i, ownColor, oppositeColor, myUpCount, myDownCount, myLeftCount, myRightCount, myRightUpCount, myLeftUpCount, myLeftDownCount, myRightDownCount);
					int oppoJumpAttack3Count = jumpAttack3Judge(j, i, oppositeColor, ownColor, oppoUpCount, oppoDownCount, oppoLeftCount, oppoRightCount, oppoRightUpCount, oppoLeftUpCount, oppoLeftDownCount, oppoRightDownCount);
					int mySleep3Count = sleep3Judge1(j, i, ownColor, oppositeColor, myUpCount, myDownCount, myLeftCount, myRightCount, myRightUpCount, myLeftUpCount, myLeftDownCount, myRightDownCount) + sleep3Judge2(j, i, ownColor, oppositeColor, myUpCount, myDownCount, myLeftCount, myRightCount, myRightUpCount, myLeftUpCount, myLeftDownCount, myRightDownCount);
					int oppoSleep3Count = sleep3Judge1(j, i, oppositeColor, ownColor, oppoUpCount, oppoDownCount, oppoLeftCount, oppoRightCount, oppoRightUpCount, oppoLeftUpCount, oppoLeftDownCount, oppoRightDownCount) + sleep3Judge2(j, i, oppositeColor, ownColor, oppoUpCount, oppoDownCount, oppoLeftCount, oppoRightCount, oppoRightUpCount, oppoLeftUpCount, oppoLeftDownCount, oppoRightDownCount);
					int myDie3Count = die3Judge(j, i, ownColor, myUpCount, myDownCount, myLeftCount, myRightCount, myRightUpCount, myLeftUpCount, myLeftDownCount, myRightDownCount);
					int oppoDie3Count = die3Judge(j, i, oppositeColor, oppoUpCount, oppoDownCount, oppoLeftCount, oppoRightCount, oppoRightUpCount, oppoLeftUpCount, oppoLeftDownCount, oppoRightDownCount);
					myAdvan[i][j] += myAlive3*myAlive3Count;
					oppoAdvan[i][j] += oppoAlive3*oppoAlive3Count;
					myAdvan[i][j] += myAttack3*myAttack3Count;
					oppoAdvan[i][j] += oppoAttack3*oppoAttack3Count;
					myAdvan[i][j] += myJumpAlive3*myJumpAlive3Count;
					oppoAdvan[i][j] += oppoJumpAlive3*oppoJumpAlive3Count;
					myAdvan[i][j] += myJumpAttack3*myJumpAttack3Count;
					oppoAdvan[i][j] += oppoJumpAttack3*oppoJumpAttack3Count;
					myAdvan[i][j] += mySleep3*mySleep3Count;
					oppoAdvan[i][j] += oppoSleep3*oppoSleep3Count;
					myAdvan[i][j] += myDie3*myDie3Count;
					oppoAdvan[i][j] += oppoDie3*oppoDie3Count;
					if (myAlive3Count == 0 && myAttack3Count == 0 && myJumpAlive3Count == 0 && myJumpAttack3Count == 0 && mySleep3Count == 0 && myDie3Count == 0)
					{
						myAdvan[i][j] += myAnother3*another3Judge(j, i, ownColor, myUpCount, myDownCount, myLeftCount, myRightCount, myRightUpCount, myLeftUpCount, myLeftDownCount, myRightDownCount);
					}
					if (oppoAlive3Count == 0 && oppoAttack3Count == 0 && oppoJumpAlive3Count == 0 && oppoJumpAttack3Count == 0 && oppoSleep3Count == 0 && oppoDie3Count == 0)
					{
						oppoAdvan[i][j] += oppoAnother3*another3Judge(j, i, oppositeColor, oppoUpCount, oppoDownCount, oppoLeftCount, oppoRightCount, oppoRightUpCount, oppoLeftUpCount, oppoLeftDownCount, oppoRightDownCount);
					}

					int myAlive2Count = alive2Judge(j, i, ownColor, myUpCount, myDownCount, myLeftCount, myRightCount, myRightUpCount, myLeftUpCount, myLeftDownCount, myRightDownCount);
					int oppoAlive2Count = alive2Judge(j, i, oppositeColor, oppoUpCount, oppoDownCount, oppoLeftCount, oppoRightCount, oppoRightUpCount, oppoLeftUpCount, oppoLeftDownCount, oppoRightDownCount);
					int myJumpAlive2Count = jumpAlive2Judge(j, i, ownColor, oppositeColor, myUpCount, myDownCount, myLeftCount, myRightCount, myRightUpCount, myLeftUpCount, myLeftDownCount, myRightDownCount);
					int oppoJumpAlive2Count = jumpAlive2Judge(j, i, oppositeColor, ownColor, oppoUpCount, oppoDownCount, oppoLeftCount, oppoRightCount, oppoRightUpCount, oppoLeftUpCount, oppoLeftDownCount, oppoRightDownCount);
					int myAttack2Count = attack2Judge(j, i, ownColor, oppositeColor, myUpCount, myDownCount, myLeftCount, myRightCount, myRightUpCount, myLeftUpCount, myLeftDownCount, myRightDownCount);
					int oppoAttack2Count = attack2Judge(j, i, oppositeColor, ownColor, oppoUpCount, oppoDownCount, oppoLeftCount, oppoRightCount, oppoRightUpCount, oppoLeftUpCount, oppoLeftDownCount, oppoRightDownCount);
					int myJumpAttack2Count = jumpAttack2Judge(j, i, ownColor, oppositeColor, myUpCount, myDownCount, myLeftCount, myRightCount, myRightUpCount, myLeftUpCount, myLeftDownCount, myRightDownCount);
					int oppoJumpAttack2Count = jumpAttack2Judge(j, i, oppositeColor, ownColor, oppoUpCount, oppoDownCount, oppoLeftCount, oppoRightCount, oppoRightUpCount, oppoLeftUpCount, oppoLeftDownCount, oppoRightDownCount);

					myAdvan[i][j] += myAlive2*myAlive2Count;
					oppoAdvan[i][j] += oppoAlive2*oppoAlive2Count;
					myAdvan[i][j] += myJumpAlive2*myJumpAlive2Count;
					oppoAdvan[i][j] += oppoJumpAlive2*oppoJumpAlive2Count;
					myAdvan[i][j] += myAttack2*myAttack2Count;
					oppoAdvan[i][j] += oppoAttack2*oppoAttack2Count;
					myAdvan[i][j] += myJumpAttack2*myJumpAttack2Count;
					oppoAdvan[i][j] += oppoJumpAttack2*oppoJumpAttack2Count;

					if (myAlive2Count == 0 && myJumpAlive2Count == 0 && myAttack2Count == 0 && myJumpAttack2Count == 0)
					{
						myAdvan[i][j] += myAnother2*another2Judge(j, i, ownColor, myUpCount, myDownCount, myLeftCount, myRightCount, myRightUpCount, myLeftUpCount, myLeftDownCount, myRightDownCount);
					}
					if (oppoAlive2Count == 0 && oppoJumpAlive2Count == 0 && oppoAttack2Count == 0 && oppoJumpAttack2Count == 0)
					{
						int a = another2Judge(j, i, oppositeColor, oppoUpCount, oppoDownCount, oppoLeftCount, oppoRightCount, oppoRightUpCount, oppoLeftUpCount, oppoLeftDownCount, oppoRightDownCount);
						oppoAdvan[i][j] += oppoAnother2*another2Judge(j, i, oppositeColor, oppoUpCount, oppoDownCount, oppoLeftCount, oppoRightCount, oppoRightUpCount, oppoLeftUpCount, oppoLeftDownCount, oppoRightDownCount);
					}

					int myAlive1Count = alive1Judge(j, i, ownColor, myUpCount, myDownCount, myLeftCount, myRightCount, myRightUpCount, myLeftUpCount, myLeftDownCount, myRightDownCount);
					int oppoAlive1Count = alive1Judge(j, i, oppositeColor, oppoUpCount, oppoDownCount, oppoLeftCount, oppoRightCount, oppoRightUpCount, oppoLeftUpCount, oppoLeftDownCount, oppoRightDownCount);

					myAdvan[i][j] += myAlive1*myAlive1Count;
					oppoAdvan[i][j] += oppoAlive1*oppoAlive1Count;

					if (myAlive1Count == 0)
					{
						myAdvan[i][j] += myAnother1*another1Judge(j, i, ownColor, myUpCount, myDownCount, myLeftCount, myRightCount, myRightUpCount, myLeftUpCount, myLeftDownCount, myRightDownCount);
					}
					if (oppoAlive1Count == 0)
					{
						int b = another1Judge(j, i, oppositeColor, oppoUpCount, oppoDownCount, oppoLeftCount, oppoRightCount, oppoRightUpCount, oppoLeftUpCount, oppoLeftDownCount, oppoRightDownCount);
						oppoAdvan[i][j] += oppoAnother1*another1Judge(j, i, oppositeColor, oppoUpCount, oppoDownCount, oppoLeftCount, oppoRightCount, oppoRightUpCount, oppoLeftUpCount, oppoLeftDownCount, oppoRightDownCount);
					}

					myAdvan[i][j] += (myUpCount + myDownCount + myLeftCount + myRightCount + myRightUpCount + myLeftUpCount + myLeftDownCount + myRightDownCount) * nothing * 2;
					oppoAdvan[i][j] += (oppoUpCount + oppoDownCount + oppoLeftCount + oppoRightCount + oppoRightUpCount + oppoLeftUpCount + oppoLeftDownCount + oppoRightDownCount) * nothing * 2;
				}
			}
		}
	}

	//int doubleAlive3 = 5000;
	//int die3Alive3 = 1000;
	//int die4 = 500;
}

void CalculateAdvantage(Compete *compete)
{
	int myAlive4 = 100000;
	int myAttack4 = 80000;
	int myJump4 = 77000;

	int myAlive3 = 50000;
	int myJumpAlive3 = 45000;

	int myAttack3 = 25000;
	int mySleep3 = 24000;
	int myJumpAttack3 = 24000;
	int myDie3 = 600;//是指一端封住，另一端间隔一空格后被封住（只能形成死4）
	int myAnother3 = 775;

	int myAlive2 = 19500;//双二必是可以走出双三的，所以alive3 > alive2 * 2 > sleep3
	int myJumpAlive2 = 18500;
	int myAttack2 = 8000;
	int myJumpAttack2 = 4000;
	int myAnother2 = 500;

	int myAlive1 = 1025;
	int myAnother1 = 225;

	int oppoAlive4 = 80000;
	int oppoAttack4 = 70000;
	int oppoJump4 = 65000;

	int oppoAlive3 = 45000;
	int oppoJumpAlive3 = 40000;

	int oppoAttack3 = 35000;
	int oppoSleep3 = 33000;
	int oppoJumpAttack3 = 33000;
	int oppoDie3 = 600;//是指一端封住，另一端间隔一空格后被封住（只能形成死4）
	int oppoAnother3 = 725;

	int oppoAlive2 = 19000;
	int oppoJumpAlive2 = 18000;
	int oppoAttack2 = 8000;
	int oppoJumpAttack2 = 4000;
	int oppoAnother2 = 425;

	int oppoAlive1 = 1000;
	int oppoAnother1 = 200;

	int nothing = 1;


	int myUpCount = 0;//↑
	int myDownCount = 0;//↓
	int myLeftCount = 0;//←
	int myRightCount = 0;//→
	int myRightUpCount = 0;//↗
	int myLeftUpCount = 0;//↖
	int myLeftDownCount = 0;//↙
	int myRightDownCount = 0;//↘

	int oppoUpCount = 0;//↑
	int oppoDownCount = 0;//↓
	int oppoLeftCount = 0;//←
	int oppoRightCount = 0;//→
	int oppoRightUpCount = 0;//↗
	int oppoLeftUpCount = 0;//↖
	int oppoLeftDownCount = 0;//↙
	int oppoRightDownCount = 0;//↘

							   //int tempCompeteCount = 0;
	for (int i = 0; i < ROWS; i++)
	{
		for (int j = 0; j < COLS; j++)
		{
			compete[i*ROWS + j].row = i;
			compete[i*ROWS + j].col = j;
			if (board[i][j].empty)
			{
				compete[i*ROWS + j].myAdvan = compete[i*ROWS + j].myAdvan + (14 - (abs(7 - i) + abs(7 - j))) * nothing;
				compete[i*ROWS + j].oppoAdvan = compete[i*ROWS + j].oppoAdvan + (14 - (abs(7 - i) + abs(7 - j))) * nothing;

				if (aroundExistChess(j, i))
				{
					aroundCount(j, i, ownColor, myUpCount, myDownCount, myLeftCount, myRightCount, myRightUpCount, myLeftUpCount, myLeftDownCount, myRightDownCount);
					aroundCount(j, i, oppositeColor, oppoUpCount, oppoDownCount, oppoLeftCount, oppoRightCount, oppoRightUpCount, oppoLeftUpCount, oppoLeftDownCount, oppoRightDownCount);
					int oppoAlive4Count = alive4Judge(j, i, oppositeColor, oppoUpCount, oppoDownCount, oppoLeftCount, oppoRightCount, oppoRightUpCount, oppoLeftUpCount, oppoLeftDownCount, oppoRightDownCount);
					int myAlive4Count = alive4Judge(j, i, ownColor, myUpCount, myDownCount, myLeftCount, myRightCount, myRightUpCount, myLeftUpCount, myLeftDownCount, myRightDownCount);
					//myAdvan[i][j] += alive4*alive4Judge(j, i, ownColor);
					//oppoAdvan[i][j] += alive4*alive4Judge(j, i, oppositeColor);
					compete[i*ROWS + j].myAdvan += myAlive4*myAlive4Count;
					compete[i*ROWS + j].oppoAdvan += oppoAlive4*oppoAlive4Count;

					if (myAlive4Count == 0)//无活4
					{
						compete[i*ROWS + j].myAdvan += myAttack4*attack4Judge(j, i, ownColor, myUpCount, myDownCount, myLeftCount, myRightCount, myRightUpCount, myLeftUpCount, myLeftDownCount, myRightDownCount);
						compete[i*ROWS + j].myAdvan += myJump4*jump4Judge(j, i, ownColor, myUpCount, myDownCount, myLeftCount, myRightCount, myRightUpCount, myLeftUpCount, myLeftDownCount, myRightDownCount);

					}
					if (oppoAlive4Count == 0)
					{
						compete[i*ROWS + j].oppoAdvan += oppoAttack4*attack4Judge(j, i, oppositeColor, oppoUpCount, oppoDownCount, oppoLeftCount, oppoRightCount, oppoRightUpCount, oppoLeftUpCount, oppoLeftDownCount, oppoRightDownCount);
						compete[i*ROWS + j].oppoAdvan += oppoJump4*jump4Judge(j, i, oppositeColor, oppoUpCount, oppoDownCount, oppoLeftCount, oppoRightCount, oppoRightUpCount, oppoLeftUpCount, oppoLeftDownCount, oppoRightDownCount);
					}

					int myAlive3Count = alive3Judge(j, i, ownColor, myUpCount, myDownCount, myLeftCount, myRightCount, myRightUpCount, myLeftUpCount, myLeftDownCount, myRightDownCount);
					int oppoAlive3Count = alive3Judge(j, i, oppositeColor, oppoUpCount, oppoDownCount, oppoLeftCount, oppoRightCount, oppoRightUpCount, oppoLeftUpCount, oppoLeftDownCount, oppoRightDownCount);
					int myAttack3Count = attack3Judge(j, i, ownColor, myUpCount, myDownCount, myLeftCount, myRightCount, myRightUpCount, myLeftUpCount, myLeftDownCount, myRightDownCount);
					int oppoAttack3Count = attack3Judge(j, i, oppositeColor, oppoUpCount, oppoDownCount, oppoLeftCount, oppoRightCount, oppoRightUpCount, oppoLeftUpCount, oppoLeftDownCount, oppoRightDownCount);
					int myJumpAlive3Count = jumpAlive3Judge(j, i, ownColor, oppositeColor, myUpCount, myDownCount, myLeftCount, myRightCount, myRightUpCount, myLeftUpCount, myLeftDownCount, myRightDownCount);
					int oppoJumpAlive3Count = jumpAlive3Judge(j, i, oppositeColor, ownColor, oppoUpCount, oppoDownCount, oppoLeftCount, oppoRightCount, oppoRightUpCount, oppoLeftUpCount, oppoLeftDownCount, oppoRightDownCount);
					int myJumpAttack3Count = jumpAttack3Judge(j, i, ownColor, oppositeColor, myUpCount, myDownCount, myLeftCount, myRightCount, myRightUpCount, myLeftUpCount, myLeftDownCount, myRightDownCount);
					int oppoJumpAttack3Count = jumpAttack3Judge(j, i, oppositeColor, ownColor, oppoUpCount, oppoDownCount, oppoLeftCount, oppoRightCount, oppoRightUpCount, oppoLeftUpCount, oppoLeftDownCount, oppoRightDownCount);
					int mySleep3Count = sleep3Judge1(j, i, ownColor, oppositeColor, myUpCount, myDownCount, myLeftCount, myRightCount, myRightUpCount, myLeftUpCount, myLeftDownCount, myRightDownCount) + sleep3Judge2(j, i, ownColor, oppositeColor, myUpCount, myDownCount, myLeftCount, myRightCount, myRightUpCount, myLeftUpCount, myLeftDownCount, myRightDownCount);
					int oppoSleep3Count = sleep3Judge1(j, i, oppositeColor, ownColor, oppoUpCount, oppoDownCount, oppoLeftCount, oppoRightCount, oppoRightUpCount, oppoLeftUpCount, oppoLeftDownCount, oppoRightDownCount) + sleep3Judge2(j, i, oppositeColor, ownColor, oppoUpCount, oppoDownCount, oppoLeftCount, oppoRightCount, oppoRightUpCount, oppoLeftUpCount, oppoLeftDownCount, oppoRightDownCount);
					int myDie3Count = die3Judge(j, i, ownColor, myUpCount, myDownCount, myLeftCount, myRightCount, myRightUpCount, myLeftUpCount, myLeftDownCount, myRightDownCount);
					int oppoDie3Count = die3Judge(j, i, oppositeColor, oppoUpCount, oppoDownCount, oppoLeftCount, oppoRightCount, oppoRightUpCount, oppoLeftUpCount, oppoLeftDownCount, oppoRightDownCount);
					compete[i*ROWS + j].myAdvan += myAlive3*myAlive3Count;
					compete[i*ROWS + j].oppoAdvan += oppoAlive3*oppoAlive3Count;
					compete[i*ROWS + j].myAdvan += myAttack3*myAttack3Count;
					compete[i*ROWS + j].oppoAdvan += oppoAttack3*oppoAttack3Count;
					compete[i*ROWS + j].myAdvan += myJumpAlive3*myJumpAlive3Count;
					compete[i*ROWS + j].oppoAdvan += oppoJumpAlive3*oppoJumpAlive3Count;
					compete[i*ROWS + j].myAdvan += myJumpAttack3*myJumpAttack3Count;
					compete[i*ROWS + j].oppoAdvan += oppoJumpAttack3*oppoJumpAttack3Count;
					compete[i*ROWS + j].myAdvan += mySleep3*mySleep3Count;
					compete[i*ROWS + j].oppoAdvan += oppoSleep3*oppoSleep3Count;
					compete[i*ROWS + j].myAdvan += myDie3*myDie3Count;
					compete[i*ROWS + j].oppoAdvan += oppoDie3*oppoDie3Count;
					if (myAlive3Count == 0 && myAttack3Count == 0 && myJumpAlive3Count == 0 && myJumpAttack3Count == 0 && mySleep3Count == 0 && myDie3Count == 0)
					{
						compete[i*ROWS + j].myAdvan += myAnother3*another3Judge(j, i, ownColor, myUpCount, myDownCount, myLeftCount, myRightCount, myRightUpCount, myLeftUpCount, myLeftDownCount, myRightDownCount);
					}
					if (oppoAlive3Count == 0 && oppoAttack3Count == 0 && oppoJumpAlive3Count == 0 && oppoJumpAttack3Count == 0 && oppoSleep3Count == 0 && oppoDie3Count == 0)
					{
						compete[i*ROWS + j].oppoAdvan += oppoAnother3*another3Judge(j, i, oppositeColor, oppoUpCount, oppoDownCount, oppoLeftCount, oppoRightCount, oppoRightUpCount, oppoLeftUpCount, oppoLeftDownCount, oppoRightDownCount);
					}

					int myAlive2Count = alive2Judge(j, i, ownColor, myUpCount, myDownCount, myLeftCount, myRightCount, myRightUpCount, myLeftUpCount, myLeftDownCount, myRightDownCount);
					int oppoAlive2Count = alive2Judge(j, i, oppositeColor, oppoUpCount, oppoDownCount, oppoLeftCount, oppoRightCount, oppoRightUpCount, oppoLeftUpCount, oppoLeftDownCount, oppoRightDownCount);
					int myJumpAlive2Count = jumpAlive2Judge(j, i, ownColor, oppositeColor, myUpCount, myDownCount, myLeftCount, myRightCount, myRightUpCount, myLeftUpCount, myLeftDownCount, myRightDownCount);
					int oppoJumpAlive2Count = jumpAlive2Judge(j, i, oppositeColor, ownColor, oppoUpCount, oppoDownCount, oppoLeftCount, oppoRightCount, oppoRightUpCount, oppoLeftUpCount, oppoLeftDownCount, oppoRightDownCount);
					int myAttack2Count = attack2Judge(j, i, ownColor, oppositeColor, myUpCount, myDownCount, myLeftCount, myRightCount, myRightUpCount, myLeftUpCount, myLeftDownCount, myRightDownCount);
					int oppoAttack2Count = attack2Judge(j, i, oppositeColor, ownColor, oppoUpCount, oppoDownCount, oppoLeftCount, oppoRightCount, oppoRightUpCount, oppoLeftUpCount, oppoLeftDownCount, oppoRightDownCount);
					int myJumpAttack2Count = jumpAttack2Judge(j, i, ownColor, oppositeColor, myUpCount, myDownCount, myLeftCount, myRightCount, myRightUpCount, myLeftUpCount, myLeftDownCount, myRightDownCount);
					int oppoJumpAttack2Count = jumpAttack2Judge(j, i, oppositeColor, ownColor, oppoUpCount, oppoDownCount, oppoLeftCount, oppoRightCount, oppoRightUpCount, oppoLeftUpCount, oppoLeftDownCount, oppoRightDownCount);

					compete[i*ROWS + j].myAdvan += myAlive2*myAlive2Count;
					compete[i*ROWS + j].oppoAdvan += oppoAlive2*oppoAlive2Count;
					compete[i*ROWS + j].myAdvan += myJumpAlive2*myJumpAlive2Count;
					compete[i*ROWS + j].oppoAdvan += oppoJumpAlive2*oppoJumpAlive2Count;
					compete[i*ROWS + j].myAdvan += myAttack2*myAttack2Count;
					compete[i*ROWS + j].oppoAdvan += oppoAttack2*oppoAttack2Count;
					compete[i*ROWS + j].myAdvan += myJumpAttack2*myJumpAttack2Count;
					compete[i*ROWS + j].oppoAdvan += oppoJumpAttack2*oppoJumpAttack2Count;

					if (myAlive2Count == 0 && myJumpAlive2Count == 0 && myAttack2Count == 0 && myJumpAttack2Count == 0)
					{
						compete[i*ROWS + j].myAdvan += myAnother2*another2Judge(j, i, ownColor, myUpCount, myDownCount, myLeftCount, myRightCount, myRightUpCount, myLeftUpCount, myLeftDownCount, myRightDownCount);
					}
					if (oppoAlive2Count == 0 && oppoJumpAlive2Count == 0 && oppoAttack2Count == 0 && oppoJumpAttack2Count == 0)
					{
						//int a = another2Judge(j, i, oppositeColor, oppoUpCount, oppoDownCount, oppoLeftCount, oppoRightCount, oppoRightUpCount, oppoLeftUpCount, oppoLeftDownCount, oppoRightDownCount);
						compete[i*ROWS + j].oppoAdvan += oppoAnother2*another2Judge(j, i, oppositeColor, oppoUpCount, oppoDownCount, oppoLeftCount, oppoRightCount, oppoRightUpCount, oppoLeftUpCount, oppoLeftDownCount, oppoRightDownCount);
					}

					int myAlive1Count = alive1Judge(j, i, ownColor, myUpCount, myDownCount, myLeftCount, myRightCount, myRightUpCount, myLeftUpCount, myLeftDownCount, myRightDownCount);
					int oppoAlive1Count = alive1Judge(j, i, oppositeColor, oppoUpCount, oppoDownCount, oppoLeftCount, oppoRightCount, oppoRightUpCount, oppoLeftUpCount, oppoLeftDownCount, oppoRightDownCount);

					compete[i*ROWS + j].myAdvan += myAlive1*myAlive1Count;
					compete[i*ROWS + j].oppoAdvan += oppoAlive1*oppoAlive1Count;

					if (myAlive1Count == 0)
					{
						compete[i*ROWS + j].myAdvan += myAnother1*another1Judge(j, i, ownColor, myUpCount, myDownCount, myLeftCount, myRightCount, myRightUpCount, myLeftUpCount, myLeftDownCount, myRightDownCount);
					}
					if (oppoAlive1Count == 0)
					{
						//int b = another1Judge(j, i, oppositeColor, oppoUpCount, oppoDownCount, oppoLeftCount, oppoRightCount, oppoRightUpCount, oppoLeftUpCount, oppoLeftDownCount, oppoRightDownCount);
						compete[i*ROWS + j].oppoAdvan += oppoAnother1*another1Judge(j, i, oppositeColor, oppoUpCount, oppoDownCount, oppoLeftCount, oppoRightCount, oppoRightUpCount, oppoLeftUpCount, oppoLeftDownCount, oppoRightDownCount);
					}

					compete[i*ROWS + j].myAdvan += (myUpCount + myDownCount + myLeftCount + myRightCount + myRightUpCount + myLeftUpCount + myLeftDownCount + myRightDownCount) * nothing * 2;
					compete[i*ROWS + j].oppoAdvan += (oppoUpCount + oppoDownCount + oppoLeftCount + oppoRightCount + oppoRightUpCount + oppoLeftUpCount + oppoLeftDownCount + oppoRightDownCount) * nothing * 2;
				}
			}
			//tempCompeteCount++;
			compete[i*ROWS + j].relativeAdvan = max(compete[i*ROWS + j].myAdvan, compete[i*ROWS + j].oppoAdvan);
		}
	}

	//int doubleAlive3 = 5000;
	//int die3Alive3 = 1000;
	//int die4 = 500;
}

void CalculateAdvantageAndWin(Compete *compete)
{
	int win = 10000000;
	int myAlive4 = 200000;
	int myAttack4 = 160000;
	int myJump4 = 16000;

	int myAlive3 = 50000;
	int myJumpAlive3 = 45000;

	int myAttack3 = 25000;
	int mySleep3 = 24000;
	int myJumpAttack3 = 24000;
	int myDie3 = 600;//是指一端封住，另一端间隔一空格后被封住（只能形成死4）
	int myAnother3 = 775;

	int myAlive2 = 19500;//双二必是可以走出双三的，所以alive3 > alive2 * 2 > sleep3
	int myJumpAlive2 = 14500;
	int myAttack2 = 8000;
	int myJumpAttack2 = 4000;
	int myAnother2 = 500;

	int myAlive1 = 1025;
	int myAnother1 = 225;

	int oppoAlive4 = 80000;
	int oppoAttack4 = 70000;
	int oppoJump4 = 65000;

	int oppoAlive3 = 45000;
	int oppoJumpAlive3 = 40000;

	int oppoAttack3 = 35000;
	int oppoSleep3 = 33000;
	int oppoJumpAttack3 = 33000;
	int oppoDie3 = 600;//是指一端封住，另一端间隔一空格后被封住（只能形成死4）
	int oppoAnother3 = 725;

	int oppoAlive2 = 19000;
	int oppoJumpAlive2 = 14000;
	int oppoAttack2 = 8000;
	int oppoJumpAttack2 = 4000;
	int oppoAnother2 = 425;

	int oppoAlive1 = 1000;
	int oppoAnother1 = 200;

	int nothing = 1;


	int myUpCount = 0;//↑
	int myDownCount = 0;//↓
	int myLeftCount = 0;//←
	int myRightCount = 0;//→
	int myRightUpCount = 0;//↗
	int myLeftUpCount = 0;//↖
	int myLeftDownCount = 0;//↙
	int myRightDownCount = 0;//↘

	int oppoUpCount = 0;//↑
	int oppoDownCount = 0;//↓
	int oppoLeftCount = 0;//←
	int oppoRightCount = 0;//→
	int oppoRightUpCount = 0;//↗
	int oppoLeftUpCount = 0;//↖
	int oppoLeftDownCount = 0;//↙
	int oppoRightDownCount = 0;//↘

							   //int tempCompeteCount = 0;
	for (int i = 0; i < ROWS; i++)
	{
		for (int j = 0; j < COLS; j++)
		{
			compete[i*ROWS + j].row = i;
			compete[i*ROWS + j].col = j;

			aroundCount(j, i, ownColor, myUpCount, myDownCount, myLeftCount, myRightCount, myRightUpCount, myLeftUpCount, myLeftDownCount, myRightDownCount);
			aroundCount(j, i, oppositeColor, oppoUpCount, oppoDownCount, oppoLeftCount, oppoRightCount, oppoRightUpCount, oppoLeftUpCount, oppoLeftDownCount, oppoRightDownCount);

			if (board[i][j].empty)
			{
				compete[i*ROWS + j].myAdvan = compete[i*ROWS + j].myAdvan + (14 - (abs(7 - i) + abs(7 - j))) * nothing;
				compete[i*ROWS + j].oppoAdvan = compete[i*ROWS + j].oppoAdvan + (14 - (abs(7 - i) + abs(7 - j))) * nothing;

				if (aroundExistChess(j, i))
				{
					aroundCount(j, i, ownColor, myUpCount, myDownCount, myLeftCount, myRightCount, myRightUpCount, myLeftUpCount, myLeftDownCount, myRightDownCount);
					aroundCount(j, i, oppositeColor, oppoUpCount, oppoDownCount, oppoLeftCount, oppoRightCount, oppoRightUpCount, oppoLeftUpCount, oppoLeftDownCount, oppoRightDownCount);
					int oppoAlive4Count = alive4Judge(j, i, oppositeColor, oppoUpCount, oppoDownCount, oppoLeftCount, oppoRightCount, oppoRightUpCount, oppoLeftUpCount, oppoLeftDownCount, oppoRightDownCount);
					int myAlive4Count = alive4Judge(j, i, ownColor, myUpCount, myDownCount, myLeftCount, myRightCount, myRightUpCount, myLeftUpCount, myLeftDownCount, myRightDownCount);
					//myAdvan[i][j] += alive4*alive4Judge(j, i, ownColor);
					//oppoAdvan[i][j] += alive4*alive4Judge(j, i, oppositeColor);
					compete[i*ROWS + j].myAdvan += myAlive4*myAlive4Count;
					compete[i*ROWS + j].oppoAdvan += oppoAlive4*oppoAlive4Count;

					if (myAlive4Count == 0)//无活4
					{
						compete[i*ROWS + j].myAdvan += myAttack4*attack4Judge(j, i, ownColor, myUpCount, myDownCount, myLeftCount, myRightCount, myRightUpCount, myLeftUpCount, myLeftDownCount, myRightDownCount);
						compete[i*ROWS + j].myAdvan += myJump4*jump4Judge(j, i, ownColor, myUpCount, myDownCount, myLeftCount, myRightCount, myRightUpCount, myLeftUpCount, myLeftDownCount, myRightDownCount);

					}
					if (oppoAlive4Count == 0)
					{
						compete[i*ROWS + j].oppoAdvan += oppoAttack4*attack4Judge(j, i, oppositeColor, oppoUpCount, oppoDownCount, oppoLeftCount, oppoRightCount, oppoRightUpCount, oppoLeftUpCount, oppoLeftDownCount, oppoRightDownCount);
						compete[i*ROWS + j].oppoAdvan += oppoJump4*jump4Judge(j, i, oppositeColor, oppoUpCount, oppoDownCount, oppoLeftCount, oppoRightCount, oppoRightUpCount, oppoLeftUpCount, oppoLeftDownCount, oppoRightDownCount);
					}

					int myAlive3Count = alive3Judge(j, i, ownColor, myUpCount, myDownCount, myLeftCount, myRightCount, myRightUpCount, myLeftUpCount, myLeftDownCount, myRightDownCount);
					int oppoAlive3Count = alive3Judge(j, i, oppositeColor, oppoUpCount, oppoDownCount, oppoLeftCount, oppoRightCount, oppoRightUpCount, oppoLeftUpCount, oppoLeftDownCount, oppoRightDownCount);
					int myAttack3Count = attack3Judge(j, i, ownColor, myUpCount, myDownCount, myLeftCount, myRightCount, myRightUpCount, myLeftUpCount, myLeftDownCount, myRightDownCount);
					int oppoAttack3Count = attack3Judge(j, i, oppositeColor, oppoUpCount, oppoDownCount, oppoLeftCount, oppoRightCount, oppoRightUpCount, oppoLeftUpCount, oppoLeftDownCount, oppoRightDownCount);
					int myJumpAlive3Count = jumpAlive3Judge(j, i, ownColor, oppositeColor, myUpCount, myDownCount, myLeftCount, myRightCount, myRightUpCount, myLeftUpCount, myLeftDownCount, myRightDownCount);
					int oppoJumpAlive3Count = jumpAlive3Judge(j, i, oppositeColor, ownColor, oppoUpCount, oppoDownCount, oppoLeftCount, oppoRightCount, oppoRightUpCount, oppoLeftUpCount, oppoLeftDownCount, oppoRightDownCount);
					int myJumpAttack3Count = jumpAttack3Judge(j, i, ownColor, oppositeColor, myUpCount, myDownCount, myLeftCount, myRightCount, myRightUpCount, myLeftUpCount, myLeftDownCount, myRightDownCount);
					int oppoJumpAttack3Count = jumpAttack3Judge(j, i, oppositeColor, ownColor, oppoUpCount, oppoDownCount, oppoLeftCount, oppoRightCount, oppoRightUpCount, oppoLeftUpCount, oppoLeftDownCount, oppoRightDownCount);
					int mySleep3Count = sleep3Judge1(j, i, ownColor, oppositeColor, myUpCount, myDownCount, myLeftCount, myRightCount, myRightUpCount, myLeftUpCount, myLeftDownCount, myRightDownCount) + sleep3Judge2(j, i, ownColor, oppositeColor, myUpCount, myDownCount, myLeftCount, myRightCount, myRightUpCount, myLeftUpCount, myLeftDownCount, myRightDownCount);
					int oppoSleep3Count = sleep3Judge1(j, i, oppositeColor, ownColor, oppoUpCount, oppoDownCount, oppoLeftCount, oppoRightCount, oppoRightUpCount, oppoLeftUpCount, oppoLeftDownCount, oppoRightDownCount) + sleep3Judge2(j, i, oppositeColor, ownColor, oppoUpCount, oppoDownCount, oppoLeftCount, oppoRightCount, oppoRightUpCount, oppoLeftUpCount, oppoLeftDownCount, oppoRightDownCount);
					int myDie3Count = die3Judge(j, i, ownColor, myUpCount, myDownCount, myLeftCount, myRightCount, myRightUpCount, myLeftUpCount, myLeftDownCount, myRightDownCount);
					int oppoDie3Count = die3Judge(j, i, oppositeColor, oppoUpCount, oppoDownCount, oppoLeftCount, oppoRightCount, oppoRightUpCount, oppoLeftUpCount, oppoLeftDownCount, oppoRightDownCount);
					compete[i*ROWS + j].myAdvan += myAlive3*myAlive3Count;
					compete[i*ROWS + j].oppoAdvan += oppoAlive3*oppoAlive3Count;
					compete[i*ROWS + j].myAdvan += myAttack3*myAttack3Count;
					compete[i*ROWS + j].oppoAdvan += oppoAttack3*oppoAttack3Count;
					compete[i*ROWS + j].myAdvan += myJumpAlive3*myJumpAlive3Count;
					compete[i*ROWS + j].oppoAdvan += oppoJumpAlive3*oppoJumpAlive3Count;
					compete[i*ROWS + j].myAdvan += myJumpAttack3*myJumpAttack3Count;
					compete[i*ROWS + j].oppoAdvan += oppoJumpAttack3*oppoJumpAttack3Count;
					compete[i*ROWS + j].myAdvan += mySleep3*mySleep3Count;
					compete[i*ROWS + j].oppoAdvan += oppoSleep3*oppoSleep3Count;
					compete[i*ROWS + j].myAdvan += myDie3*myDie3Count;
					compete[i*ROWS + j].oppoAdvan += oppoDie3*oppoDie3Count;
					if (myAlive3Count == 0 && myAttack3Count == 0 && myJumpAlive3Count == 0 && myJumpAttack3Count == 0 && mySleep3Count == 0 && myDie3Count == 0)
					{
						compete[i*ROWS + j].myAdvan += myAnother3*another3Judge(j, i, ownColor, myUpCount, myDownCount, myLeftCount, myRightCount, myRightUpCount, myLeftUpCount, myLeftDownCount, myRightDownCount);
					}
					if (oppoAlive3Count == 0 && oppoAttack3Count == 0 && oppoJumpAlive3Count == 0 && oppoJumpAttack3Count == 0 && oppoSleep3Count == 0 && oppoDie3Count == 0)
					{
						compete[i*ROWS + j].oppoAdvan += oppoAnother3*another3Judge(j, i, oppositeColor, oppoUpCount, oppoDownCount, oppoLeftCount, oppoRightCount, oppoRightUpCount, oppoLeftUpCount, oppoLeftDownCount, oppoRightDownCount);
					}

					int myAlive2Count = alive2Judge(j, i, ownColor, myUpCount, myDownCount, myLeftCount, myRightCount, myRightUpCount, myLeftUpCount, myLeftDownCount, myRightDownCount);
					int oppoAlive2Count = alive2Judge(j, i, oppositeColor, oppoUpCount, oppoDownCount, oppoLeftCount, oppoRightCount, oppoRightUpCount, oppoLeftUpCount, oppoLeftDownCount, oppoRightDownCount);
					int myJumpAlive2Count = jumpAlive2Judge(j, i, ownColor, oppositeColor, myUpCount, myDownCount, myLeftCount, myRightCount, myRightUpCount, myLeftUpCount, myLeftDownCount, myRightDownCount);
					int oppoJumpAlive2Count = jumpAlive2Judge(j, i, oppositeColor, ownColor, oppoUpCount, oppoDownCount, oppoLeftCount, oppoRightCount, oppoRightUpCount, oppoLeftUpCount, oppoLeftDownCount, oppoRightDownCount);
					int myAttack2Count = attack2Judge(j, i, ownColor, oppositeColor, myUpCount, myDownCount, myLeftCount, myRightCount, myRightUpCount, myLeftUpCount, myLeftDownCount, myRightDownCount);
					int oppoAttack2Count = attack2Judge(j, i, oppositeColor, ownColor, oppoUpCount, oppoDownCount, oppoLeftCount, oppoRightCount, oppoRightUpCount, oppoLeftUpCount, oppoLeftDownCount, oppoRightDownCount);
					int myJumpAttack2Count = jumpAttack2Judge(j, i, ownColor, oppositeColor, myUpCount, myDownCount, myLeftCount, myRightCount, myRightUpCount, myLeftUpCount, myLeftDownCount, myRightDownCount);
					int oppoJumpAttack2Count = jumpAttack2Judge(j, i, oppositeColor, ownColor, oppoUpCount, oppoDownCount, oppoLeftCount, oppoRightCount, oppoRightUpCount, oppoLeftUpCount, oppoLeftDownCount, oppoRightDownCount);

					compete[i*ROWS + j].myAdvan += myAlive2*myAlive2Count;
					compete[i*ROWS + j].oppoAdvan += oppoAlive2*oppoAlive2Count;
					compete[i*ROWS + j].myAdvan += myJumpAlive2*myJumpAlive2Count;
					compete[i*ROWS + j].oppoAdvan += oppoJumpAlive2*oppoJumpAlive2Count;
					compete[i*ROWS + j].myAdvan += myAttack2*myAttack2Count;
					compete[i*ROWS + j].oppoAdvan += oppoAttack2*oppoAttack2Count;
					compete[i*ROWS + j].myAdvan += myJumpAttack2*myJumpAttack2Count;
					compete[i*ROWS + j].oppoAdvan += oppoJumpAttack2*oppoJumpAttack2Count;

					if (myAlive2Count == 0 && myJumpAlive2Count == 0 && myAttack2Count == 0 && myJumpAttack2Count == 0)
					{
						compete[i*ROWS + j].myAdvan += myAnother2*another2Judge(j, i, ownColor, myUpCount, myDownCount, myLeftCount, myRightCount, myRightUpCount, myLeftUpCount, myLeftDownCount, myRightDownCount);
					}
					if (oppoAlive2Count == 0 && oppoJumpAlive2Count == 0 && oppoAttack2Count == 0 && oppoJumpAttack2Count == 0)
					{
						//int a = another2Judge(j, i, oppositeColor, oppoUpCount, oppoDownCount, oppoLeftCount, oppoRightCount, oppoRightUpCount, oppoLeftUpCount, oppoLeftDownCount, oppoRightDownCount);
						compete[i*ROWS + j].oppoAdvan += oppoAnother2*another2Judge(j, i, oppositeColor, oppoUpCount, oppoDownCount, oppoLeftCount, oppoRightCount, oppoRightUpCount, oppoLeftUpCount, oppoLeftDownCount, oppoRightDownCount);
					}

					int myAlive1Count = alive1Judge(j, i, ownColor, myUpCount, myDownCount, myLeftCount, myRightCount, myRightUpCount, myLeftUpCount, myLeftDownCount, myRightDownCount);
					int oppoAlive1Count = alive1Judge(j, i, oppositeColor, oppoUpCount, oppoDownCount, oppoLeftCount, oppoRightCount, oppoRightUpCount, oppoLeftUpCount, oppoLeftDownCount, oppoRightDownCount);

					compete[i*ROWS + j].myAdvan += myAlive1*myAlive1Count;
					compete[i*ROWS + j].oppoAdvan += oppoAlive1*oppoAlive1Count;

					if (myAlive1Count == 0)
					{
						compete[i*ROWS + j].myAdvan += myAnother1*another1Judge(j, i, ownColor, myUpCount, myDownCount, myLeftCount, myRightCount, myRightUpCount, myLeftUpCount, myLeftDownCount, myRightDownCount);
					}
					if (oppoAlive1Count == 0)
					{
						//int b = another1Judge(j, i, oppositeColor, oppoUpCount, oppoDownCount, oppoLeftCount, oppoRightCount, oppoRightUpCount, oppoLeftUpCount, oppoLeftDownCount, oppoRightDownCount);
						compete[i*ROWS + j].oppoAdvan += oppoAnother1*another1Judge(j, i, oppositeColor, oppoUpCount, oppoDownCount, oppoLeftCount, oppoRightCount, oppoRightUpCount, oppoLeftUpCount, oppoLeftDownCount, oppoRightDownCount);
					}

					compete[i*ROWS + j].myAdvan += (myUpCount + myDownCount + myLeftCount + myRightCount + myRightUpCount + myLeftUpCount + myLeftDownCount + myRightDownCount) * nothing * 2;
					compete[i*ROWS + j].oppoAdvan += (oppoUpCount + oppoDownCount + oppoLeftCount + oppoRightCount + oppoRightUpCount + oppoLeftUpCount + oppoLeftDownCount + oppoRightDownCount) * nothing * 2;
				}
			}
			/*else
			{
			compete[i*ROWS + j].myAdvan += win*winJudge(j, i, ownColor, myUpCount, myDownCount, myLeftCount, myRightCount, myRightUpCount, myLeftUpCount, myLeftDownCount, myRightDownCount);
			compete[i*ROWS + j].oppoAdvan += win*winJudge(j, i, oppositeColor, oppoUpCount, oppoDownCount, oppoLeftCount, oppoRightCount, oppoRightUpCount, oppoLeftUpCount, oppoLeftDownCount, oppoRightDownCount);
			}*/
			//tempCompeteCount++;
			compete[i*ROWS + j].relativeAdvan = max(compete[i*ROWS + j].myAdvan, compete[i*ROWS + j].oppoAdvan);
		}
	}

	//int doubleAlive3 = 5000;
	//int die3Alive3 = 1000;
	//int die4 = 500;
}

int CalculateAdvantageAndWinWholeSquare()
{
	int myAdvan = 0;
	int oppoAdvan = 0;

	int win = 200000000;
	int myAlive4 = 6000000;
	int myAttack4 = 5800000;
	int myJump4 = 5800000;

	int myAlive3 = 700000;
	int myJumpAlive3 = 700000;

	int myAttack3 = 85000;
	int mySleep3 = 80000;
	int myJumpAttack3 = 80000;
	int myDie3 = 600;//是指一端封住，另一端间隔一空格后被封住（只能形成死4）
	int myAnother3 = 775;

	int myAlive2 = 110000;//双二必是可以走出双三的，所以alive3 > alive2 * 2 > sleep3
	int myJumpAlive2 = 85000;
	int myAttack2 = 10000;
	int myJumpAttack2 = 7000;
	int myAnother2 = 500;

	int myAlive1 = 2000;
	int myAnother1 = 200;

	int oppoAlive4 = 2500000;
	int oppoAttack4 = 1000000;
	int oppoJump4 = 1000000;

	int oppoAlive3 = 550000;//oppoAlive3 * 2 > oppoAttack4
	int oppoJumpAlive3 = 550000;//oppoJumpAlive3 * 2 > oppoAttack4

	int oppoAttack3 = 90000;
	int oppoSleep3 = 85000;
	int oppoJumpAttack3 = 85000;
	int oppoDie3 = 600;//是指一端封住，另一端间隔一空格后被封住（只能形成死4）
	int oppoAnother3 = 775;

	int oppoAlive2 = 100000;
	int oppoJumpAlive2 = 90000;
	int oppoAttack2 = 10000;
	int oppoJumpAttack2 = 7000;
	int oppoAnother2 = 500;

	int oppoAlive1 = 2025;
	int oppoAnother1 = 200;

	int nothing = 1;


	int myUpCount = 0;//↑
	int myDownCount = 0;//↓
	int myLeftCount = 0;//←
	int myRightCount = 0;//→
	int myRightUpCount = 0;//↗
	int myLeftUpCount = 0;//↖
	int myLeftDownCount = 0;//↙
	int myRightDownCount = 0;//↘

	int oppoUpCount = 0;//↑
	int oppoDownCount = 0;//↓
	int oppoLeftCount = 0;//←
	int oppoRightCount = 0;//→
	int oppoRightUpCount = 0;//↗
	int oppoLeftUpCount = 0;//↖
	int oppoLeftDownCount = 0;//↙
	int oppoRightDownCount = 0;//↘

							   //int tempCompeteCount = 0;
	for (int i = 0; i < ROWS; i++)
	{
		for (int j = 0; j < COLS; j++)
		{
			if (board[i][j].empty)
			{
				aroundCount(j, i, ownColor, myUpCount, myDownCount, myLeftCount, myRightCount, myRightUpCount, myLeftUpCount, myLeftDownCount, myRightDownCount);
				aroundCount(j, i, oppositeColor, oppoUpCount, oppoDownCount, oppoLeftCount, oppoRightCount, oppoRightUpCount, oppoLeftUpCount, oppoLeftDownCount, oppoRightDownCount);

				myAdvan = myAdvan + (14 - (abs(7 - i) + abs(7 - j))) * nothing;
				oppoAdvan = oppoAdvan + (14 - (abs(7 - i) + abs(7 - j))) * nothing;

				if (aroundExistChess(j, i))
				{
					aroundCount(j, i, ownColor, myUpCount, myDownCount, myLeftCount, myRightCount, myRightUpCount, myLeftUpCount, myLeftDownCount, myRightDownCount);
					aroundCount(j, i, oppositeColor, oppoUpCount, oppoDownCount, oppoLeftCount, oppoRightCount, oppoRightUpCount, oppoLeftUpCount, oppoLeftDownCount, oppoRightDownCount);
					int oppoAlive4Count = alive4Judge(j, i, oppositeColor, oppoUpCount, oppoDownCount, oppoLeftCount, oppoRightCount, oppoRightUpCount, oppoLeftUpCount, oppoLeftDownCount, oppoRightDownCount);
					int myAlive4Count = alive4Judge(j, i, ownColor, myUpCount, myDownCount, myLeftCount, myRightCount, myRightUpCount, myLeftUpCount, myLeftDownCount, myRightDownCount);
					//myAdvan[i][j] += alive4*alive4Judge(j, i, ownColor);
					//oppoAdvan[i][j] += alive4*alive4Judge(j, i, oppositeColor);
					myAdvan += myAlive4*myAlive4Count / 2;
					oppoAdvan += oppoAlive4*oppoAlive4Count / 2;

					if (myAlive4Count == 0)//无活4
					{
						myAdvan += myAttack4*attack4Judge(j, i, ownColor, myUpCount, myDownCount, myLeftCount, myRightCount, myRightUpCount, myLeftUpCount, myLeftDownCount, myRightDownCount);
						myAdvan += myJump4*jump4Judge(j, i, ownColor, myUpCount, myDownCount, myLeftCount, myRightCount, myRightUpCount, myLeftUpCount, myLeftDownCount, myRightDownCount);

					}
					if (oppoAlive4Count == 0)
					{
						oppoAdvan += oppoAttack4*attack4Judge(j, i, oppositeColor, oppoUpCount, oppoDownCount, oppoLeftCount, oppoRightCount, oppoRightUpCount, oppoLeftUpCount, oppoLeftDownCount, oppoRightDownCount);
						oppoAdvan += oppoJump4*jump4Judge(j, i, oppositeColor, oppoUpCount, oppoDownCount, oppoLeftCount, oppoRightCount, oppoRightUpCount, oppoLeftUpCount, oppoLeftDownCount, oppoRightDownCount);
					}

					int myAlive3Count = alive3Judge(j, i, ownColor, myUpCount, myDownCount, myLeftCount, myRightCount, myRightUpCount, myLeftUpCount, myLeftDownCount, myRightDownCount);
					int oppoAlive3Count = alive3Judge(j, i, oppositeColor, oppoUpCount, oppoDownCount, oppoLeftCount, oppoRightCount, oppoRightUpCount, oppoLeftUpCount, oppoLeftDownCount, oppoRightDownCount);
					int myAttack3Count = attack3Judge(j, i, ownColor, myUpCount, myDownCount, myLeftCount, myRightCount, myRightUpCount, myLeftUpCount, myLeftDownCount, myRightDownCount);
					int oppoAttack3Count = attack3Judge(j, i, oppositeColor, oppoUpCount, oppoDownCount, oppoLeftCount, oppoRightCount, oppoRightUpCount, oppoLeftUpCount, oppoLeftDownCount, oppoRightDownCount);
					int myJumpAlive3Count = jumpAlive3Judge(j, i, ownColor, oppositeColor, myUpCount, myDownCount, myLeftCount, myRightCount, myRightUpCount, myLeftUpCount, myLeftDownCount, myRightDownCount);
					int oppoJumpAlive3Count = jumpAlive3Judge(j, i, oppositeColor, ownColor, oppoUpCount, oppoDownCount, oppoLeftCount, oppoRightCount, oppoRightUpCount, oppoLeftUpCount, oppoLeftDownCount, oppoRightDownCount);
					int myJumpAttack3Count = jumpAttack3Judge(j, i, ownColor, oppositeColor, myUpCount, myDownCount, myLeftCount, myRightCount, myRightUpCount, myLeftUpCount, myLeftDownCount, myRightDownCount);
					int oppoJumpAttack3Count = jumpAttack3Judge(j, i, oppositeColor, ownColor, oppoUpCount, oppoDownCount, oppoLeftCount, oppoRightCount, oppoRightUpCount, oppoLeftUpCount, oppoLeftDownCount, oppoRightDownCount);
					int mySleep3Count = sleep3Judge1(j, i, ownColor, oppositeColor, myUpCount, myDownCount, myLeftCount, myRightCount, myRightUpCount, myLeftUpCount, myLeftDownCount, myRightDownCount) + sleep3Judge2(j, i, ownColor, oppositeColor, myUpCount, myDownCount, myLeftCount, myRightCount, myRightUpCount, myLeftUpCount, myLeftDownCount, myRightDownCount) / 2;
					int oppoSleep3Count = sleep3Judge1(j, i, oppositeColor, ownColor, oppoUpCount, oppoDownCount, oppoLeftCount, oppoRightCount, oppoRightUpCount, oppoLeftUpCount, oppoLeftDownCount, oppoRightDownCount) + sleep3Judge2(j, i, oppositeColor, ownColor, oppoUpCount, oppoDownCount, oppoLeftCount, oppoRightCount, oppoRightUpCount, oppoLeftUpCount, oppoLeftDownCount, oppoRightDownCount) / 2;
					int myDie3Count = die3Judge(j, i, ownColor, myUpCount, myDownCount, myLeftCount, myRightCount, myRightUpCount, myLeftUpCount, myLeftDownCount, myRightDownCount);
					int oppoDie3Count = die3Judge(j, i, oppositeColor, oppoUpCount, oppoDownCount, oppoLeftCount, oppoRightCount, oppoRightUpCount, oppoLeftUpCount, oppoLeftDownCount, oppoRightDownCount);
					myAdvan += myAlive3*myAlive3Count / 2;
					oppoAdvan += oppoAlive3*oppoAlive3Count / 2;
					myAdvan += myAttack3*myAttack3Count;
					oppoAdvan += oppoAttack3*oppoAttack3Count;
					myAdvan += myJumpAlive3*myJumpAlive3Count / 2;
					oppoAdvan += oppoJumpAlive3*oppoJumpAlive3Count / 2;
					myAdvan += myJumpAttack3*myJumpAttack3Count;
					oppoAdvan += oppoJumpAttack3*oppoJumpAttack3Count;
					myAdvan += mySleep3*mySleep3Count;
					oppoAdvan += oppoSleep3*oppoSleep3Count;
					myAdvan += myDie3*myDie3Count;
					oppoAdvan += oppoDie3*oppoDie3Count;
					if (myAlive3Count == 0 && myAttack3Count == 0 && myJumpAlive3Count == 0 && myJumpAttack3Count == 0 && mySleep3Count == 0 && myDie3Count == 0)
					{
						myAdvan += myAnother3*another3Judge(j, i, ownColor, myUpCount, myDownCount, myLeftCount, myRightCount, myRightUpCount, myLeftUpCount, myLeftDownCount, myRightDownCount);
					}
					if (oppoAlive3Count == 0 && oppoAttack3Count == 0 && oppoJumpAlive3Count == 0 && oppoJumpAttack3Count == 0 && oppoSleep3Count == 0 && oppoDie3Count == 0)
					{
						oppoAdvan += oppoAnother3*another3Judge(j, i, oppositeColor, oppoUpCount, oppoDownCount, oppoLeftCount, oppoRightCount, oppoRightUpCount, oppoLeftUpCount, oppoLeftDownCount, oppoRightDownCount);
					}

					int myAlive2Count = alive2Judge(j, i, ownColor, myUpCount, myDownCount, myLeftCount, myRightCount, myRightUpCount, myLeftUpCount, myLeftDownCount, myRightDownCount) / 2;
					int oppoAlive2Count = alive2Judge(j, i, oppositeColor, oppoUpCount, oppoDownCount, oppoLeftCount, oppoRightCount, oppoRightUpCount, oppoLeftUpCount, oppoLeftDownCount, oppoRightDownCount) / 2;
					int myJumpAlive2Count = jumpAlive2Judge(j, i, ownColor, oppositeColor, myUpCount, myDownCount, myLeftCount, myRightCount, myRightUpCount, myLeftUpCount, myLeftDownCount, myRightDownCount);
					int oppoJumpAlive2Count = jumpAlive2Judge(j, i, oppositeColor, ownColor, oppoUpCount, oppoDownCount, oppoLeftCount, oppoRightCount, oppoRightUpCount, oppoLeftUpCount, oppoLeftDownCount, oppoRightDownCount);
					int myAttack2Count = attack2Judge(j, i, ownColor, oppositeColor, myUpCount, myDownCount, myLeftCount, myRightCount, myRightUpCount, myLeftUpCount, myLeftDownCount, myRightDownCount);
					int oppoAttack2Count = attack2Judge(j, i, oppositeColor, ownColor, oppoUpCount, oppoDownCount, oppoLeftCount, oppoRightCount, oppoRightUpCount, oppoLeftUpCount, oppoLeftDownCount, oppoRightDownCount);
					int myJumpAttack2Count = jumpAttack2Judge(j, i, ownColor, oppositeColor, myUpCount, myDownCount, myLeftCount, myRightCount, myRightUpCount, myLeftUpCount, myLeftDownCount, myRightDownCount);
					int oppoJumpAttack2Count = jumpAttack2Judge(j, i, oppositeColor, ownColor, oppoUpCount, oppoDownCount, oppoLeftCount, oppoRightCount, oppoRightUpCount, oppoLeftUpCount, oppoLeftDownCount, oppoRightDownCount);

					myAdvan += myAlive2*myAlive2Count;
					oppoAdvan += oppoAlive2*oppoAlive2Count;
					myAdvan += myJumpAlive2*myJumpAlive2Count;
					oppoAdvan += oppoJumpAlive2*oppoJumpAlive2Count;
					myAdvan += myAttack2*myAttack2Count;
					oppoAdvan += oppoAttack2*oppoAttack2Count;
					myAdvan += myJumpAttack2*myJumpAttack2Count;
					oppoAdvan += oppoJumpAttack2*oppoJumpAttack2Count;

					if (myAlive2Count == 0 && myJumpAlive2Count == 0 && myAttack2Count == 0 && myJumpAttack2Count == 0)
					{
						myAdvan += myAnother2*another2Judge(j, i, ownColor, myUpCount, myDownCount, myLeftCount, myRightCount, myRightUpCount, myLeftUpCount, myLeftDownCount, myRightDownCount);
					}
					if (oppoAlive2Count == 0 && oppoJumpAlive2Count == 0 && oppoAttack2Count == 0 && oppoJumpAttack2Count == 0)
					{
						//int a = another2Judge(j, i, oppositeColor, oppoUpCount, oppoDownCount, oppoLeftCount, oppoRightCount, oppoRightUpCount, oppoLeftUpCount, oppoLeftDownCount, oppoRightDownCount);
						oppoAdvan += oppoAnother2*another2Judge(j, i, oppositeColor, oppoUpCount, oppoDownCount, oppoLeftCount, oppoRightCount, oppoRightUpCount, oppoLeftUpCount, oppoLeftDownCount, oppoRightDownCount);
					}

					int myAlive1Count = alive1Judge(j, i, ownColor, myUpCount, myDownCount, myLeftCount, myRightCount, myRightUpCount, myLeftUpCount, myLeftDownCount, myRightDownCount) / 4;
					int oppoAlive1Count = alive1Judge(j, i, oppositeColor, oppoUpCount, oppoDownCount, oppoLeftCount, oppoRightCount, oppoRightUpCount, oppoLeftUpCount, oppoLeftDownCount, oppoRightDownCount) / 4;

					myAdvan += myAlive1*myAlive1Count;
					oppoAdvan += oppoAlive1*oppoAlive1Count;

					if (myAlive1Count == 0)
					{
						myAdvan += myAnother1*another1Judge(j, i, ownColor, myUpCount, myDownCount, myLeftCount, myRightCount, myRightUpCount, myLeftUpCount, myLeftDownCount, myRightDownCount);
					}
					if (oppoAlive1Count == 0)
					{
						//int b = another1Judge(j, i, oppositeColor, oppoUpCount, oppoDownCount, oppoLeftCount, oppoRightCount, oppoRightUpCount, oppoLeftUpCount, oppoLeftDownCount, oppoRightDownCount);
						oppoAdvan += oppoAnother1*another1Judge(j, i, oppositeColor, oppoUpCount, oppoDownCount, oppoLeftCount, oppoRightCount, oppoRightUpCount, oppoLeftUpCount, oppoLeftDownCount, oppoRightDownCount);
					}

					myAdvan += (myUpCount + myDownCount + myLeftCount + myRightCount + myRightUpCount + myLeftUpCount + myLeftDownCount + myRightDownCount) * nothing * 2;
					oppoAdvan += (oppoUpCount + oppoDownCount + oppoLeftCount + oppoRightCount + oppoRightUpCount + oppoLeftUpCount + oppoLeftDownCount + oppoRightDownCount) * nothing * 2;
				}
			}
			else
			{
				myAdvan += win*winJudge(j, i, ownColor, myUpCount, myDownCount, myLeftCount, myRightCount, myRightUpCount, myLeftUpCount, myLeftDownCount, myRightDownCount);
				oppoAdvan += win*winJudge(j, i, oppositeColor, oppoUpCount, oppoDownCount, oppoLeftCount, oppoRightCount, oppoRightUpCount, oppoLeftUpCount, oppoLeftDownCount, oppoRightDownCount);
			}
			//tempCompeteCount++;
		}
	}

	//int doubleAlive3 = 5000;
	//int die3Alive3 = 1000;
	//int die4 = 500;

	return myAdvan - oppoAdvan;
}

void saveChessBoard()
{

}

void loopStep()
{
	Square tempBoard[ROWS][COLS];
	for (int i = 0; i < ROWS; i++)
	{
		for (int j = 0; j < COLS; j++)
		{
			tempBoard[i][j].color = board[i][j].color;
			tempBoard[i][j].empty = board[i][j].empty;

		}
	}

	Compete tempCompete[MaxLoopCount][ROWS*COLS];
	memset(tempCompete, 0, sizeof(Compete) * ROWS * COLS * MaxLoopCount);

	int advantage[MaxLoopCount][MaxTopStep];
	memset(advantage, 0, sizeof(int) * MaxLoopCount * MaxTopStep);

	CalculateAdvantageAndWin(tempCompete[0]);
	sort(tempCompete[0]);
	for (int i = 0; i < MaxTopStep; i++)
	{
		board[tempCompete[0][i].row][tempCompete[0][i].col].empty = false;
		board[tempCompete[0][i].row][tempCompete[0][i].col].color = ownColor;

		if ((stepCount + 1) % 5 == 0 && ownColor == 0)
		{
			board[chessOrder[((stepCount + 1) / 5 - 1) * 2][0]][chessOrder[((stepCount + 1) / 5 - 1) * 2][1]].color = -1;
			board[chessOrder[((stepCount + 1) / 5 - 1) * 2][0]][chessOrder[((stepCount + 1) / 5 - 1) * 2][1]].empty = true;

			//tempCompete[0][MaxTopStep - 1].col = chessOrder[((stepCount + 1) / 5 - 1) * 2][1];
			//tempCompete[0][MaxTopStep - 1].row = chessOrder[((stepCount + 1) / 5 - 1) * 2][0];
		}
		else if ((stepCount) % 5 == 0 && ownColor == 1)
		{
			board[chessOrder[((stepCount) / 5 - 1) * 2 + 1][0]][chessOrder[((stepCount) / 5 - 1) * 2 + 1][1]].color = -1;
			board[chessOrder[((stepCount) / 5 - 1) * 2 + 1][0]][chessOrder[((stepCount) / 5 - 1) * 2 + 1][1]].empty = true;

			//tempCompete[0][MaxTopStep - 1].col = chessOrder[((stepCount) / 5 - 1) * 2 + 1][1];
			//tempCompete[0][MaxTopStep - 1].row = chessOrder[((stepCount) / 5 - 1) * 2 + 1][0];
		}

		memset(tempCompete[1], 0, sizeof(Compete) * ROWS * COLS);
		CalculateAdvantageAndWin(tempCompete[1]);
		sort(tempCompete[1]);

		int min = 1000000000;
		for (int j = 0; j < MaxTopStep; j++)
		{
			board[tempCompete[1][j].row][tempCompete[1][j].col].empty = false;
			board[tempCompete[1][j].row][tempCompete[1][j].col].color = oppositeColor;

			if ((stepCount + 1) % 5 == 0 && ownColor == 0)
			{
				board[chessOrder[((stepCount + 1) / 5 - 1) * 2 + 1][0]][chessOrder[((stepCount + 1) / 5 - 1) * 2 + 1][1]].color = -1;
				board[chessOrder[((stepCount + 1) / 5 - 1) * 2 + 1][0]][chessOrder[((stepCount + 1) / 5 - 1) * 2 + 1][1]].empty = true;
			}
			else if ((stepCount + 1) % 5 == 0 && ownColor == 1)
			{
				board[chessOrder[((stepCount + 1) / 5 - 1) * 2][0]][chessOrder[((stepCount + 1) / 5 - 1) * 2][1]].color = -1;
				board[chessOrder[((stepCount + 1) / 5 - 1) * 2][0]][chessOrder[((stepCount + 1) / 5 - 1) * 2][1]].empty = true;
			}

			int n = CalculateAdvantageAndWinWholeSquare();
			if (min > CalculateAdvantageAndWinWholeSquare())
			{
				min = CalculateAdvantageAndWinWholeSquare();
			}

			board[tempCompete[1][j].row][tempCompete[1][j].col].empty = true;
			board[tempCompete[1][j].row][tempCompete[1][j].col].color = -1;

			if ((stepCount + 1) % 5 == 0 && ownColor == 0)
			{
				board[chessOrder[((stepCount + 1) / 5 - 1) * 2 + 1][0]][chessOrder[((stepCount + 1) / 5 - 1) * 2 + 1][1]].color = oppositeColor;
				board[chessOrder[((stepCount + 1) / 5 - 1) * 2 + 1][0]][chessOrder[((stepCount + 1) / 5 - 1) * 2 + 1][1]].empty = false;
			}
			else if ((stepCount + 1) % 5 == 0 && ownColor == 1)
			{
				board[chessOrder[((stepCount + 1) / 5 - 1) * 2][0]][chessOrder[((stepCount + 1) / 5 - 1) * 2][1]].color = oppositeColor;
				board[chessOrder[((stepCount + 1) / 5 - 1) * 2][0]][chessOrder[((stepCount + 1) / 5 - 1) * 2][1]].empty = false;
			}
		}

		advantage[0][i] = min;

		board[tempCompete[0][i].row][tempCompete[0][i].col].empty = true;
		board[tempCompete[0][i].row][tempCompete[0][i].col].color = -1;

		if ((stepCount + 1) % 5 == 0 && ownColor == 0)
		{
			board[chessOrder[((stepCount + 1) / 5 - 1) * 2][0]][chessOrder[((stepCount + 1) / 5 - 1) * 2][1]].color = ownColor;
			board[chessOrder[((stepCount + 1) / 5 - 1) * 2][0]][chessOrder[((stepCount + 1) / 5 - 1) * 2][1]].empty = false;
		}
		else if ((stepCount) % 5 == 0 && ownColor == 1)
		{
			board[chessOrder[((stepCount) / 5 - 1) * 2 + 1][0]][chessOrder[((stepCount) / 5 - 1) * 2 + 1][1]].color = ownColor;
			board[chessOrder[((stepCount) / 5 - 1) * 2 + 1][0]][chessOrder[((stepCount) / 5 - 1) * 2 + 1][1]].empty = false;
		}
	}

	int MaxNumber = 0;
	for (int i = 1; i < MaxTopStep; i++)
	{
		if (advantage[0][MaxNumber] < advantage[0][i])
		{
			MaxNumber = i;
		}
	}

	putDown(tempCompete[0][MaxNumber].row, tempCompete[0][MaxNumber].col);

	for (int i = 0; i < ROWS; i++)//复原棋盘
	{
		for (int j = 0; j < COLS; j++)
		{
			board[i][j].color = tempBoard[i][j].color;
			board[i][j].empty = tempBoard[i][j].empty;

		}
	}
}

void OneStep()
{
	if (!Start)
	{
		int myAdvan[ROWS][COLS];
		int oppoAdvan[ROWS][COLS];
		memset(myAdvan, 0, sizeof(int)*ROWS*COLS);
		memset(oppoAdvan, 0, sizeof(int)*ROWS*COLS);

		CalculateAdvantage(myAdvan, oppoAdvan);

		int maxMyRow = 0;
		int maxMyCol = 0;
		int maxOppoRow = 0;
		int maxOppoCol = 0;
		for (int i = 0; i < ROWS; i++)
		{
			for (int j = 0; j < COLS; j++)
			{
				if (myAdvan[i][j] > myAdvan[maxMyRow][maxMyCol])
				{
					maxMyRow = i;
					maxMyCol = j;
				}
				if (oppoAdvan[i][j] > oppoAdvan[maxOppoRow][maxOppoCol])
				{
					maxOppoRow = i;
					maxOppoCol = j;
				}
			}
		}

		if (maxMyRow == maxOppoRow && maxMyCol == maxOppoCol)
		{
			putDown(maxMyRow, maxMyCol);
		}
		else if (myAdvan[maxMyRow][maxMyCol] > oppoAdvan[maxOppoRow][maxOppoCol])
		{
			cout << "my chance!!!!!!!!!!!!!!!!!" << endl;
			//cout << "putDown!!!!!" << maxMyRow << " " <<maxMyCol << endl;
			putDown(maxMyRow, maxMyCol);
		}
		else if (myAdvan[maxMyRow][maxMyCol] < oppoAdvan[maxOppoRow][maxOppoCol])
		{
			cout << "opposite chance!!!!!!!!!!!!!!!!!" << endl;
			//cout << "putDown!!!!!" << maxOppoRow << " " <<maxOppoCol << endl;
			putDown(maxOppoRow, maxOppoCol);
		}
		else if (myAdvan[maxMyRow][maxMyCol] == oppoAdvan[maxOppoRow][maxOppoCol] && myAdvan[maxMyRow][maxMyCol] > 100)
		{
			putDown(maxMyRow, maxMyCol);
		}
		else
		{
			int r = -1, c = -1;
			// printf("%s\n", lastMsg());
			while (!(r >= 0 && r < ROWS && c >= 0 && c < COLS && board[r][c].empty))
			{
				cout << "Random!!!!!!!!!!!!!!!" << endl;
				r = random(15);
				c = random(15);
				// System.out.println("Rand " + r + " " + c);
			}
			// saveChessBoard();
			putDown(r, c);
		}
	}
	else
	{
		if (board[7][7].empty)
		{
			putDown(7, 7);
		}
		else
		{
			putDown(7, 8);
		}
	}
}

void step()
{
	/*cout << stepCount << endl;
	for (int i = 0; i < 10; i++)
	{
	cout << chessOrder[i][0] << "  " << chessOrder[i][1] << endl;
	}*/
	if (!Start)
	{
		loopStep();
	}
	else
	{
		if (board[7][7].empty)
		{
			putDown(7, 7);
		}
		else
		{
			putDown(7, 8);
		}
	}
	//OneStep();
	/*if (!Start)
	{
	int myAdvan[ROWS][COLS];
	int oppoAdvan[ROWS][COLS];
	memset(myAdvan, 0, sizeof(int)*ROWS*COLS);
	memset(oppoAdvan, 0, sizeof(int)*ROWS*COLS);

	CalculateAdvantage(myAdvan, oppoAdvan);

	int maxMyRow = 0;
	int maxMyCol = 0;
	int maxOppoRow = 0;
	int maxOppoCol = 0;
	for (int i = 0; i < ROWS; i++)
	{
	for (int j = 0; j < COLS; j++)
	{
	if (myAdvan[i][j] > myAdvan[maxMyRow][maxMyCol])
	{
	maxMyRow = i;
	maxMyCol = j;
	}
	if (oppoAdvan[i][j] > oppoAdvan[maxOppoRow][maxOppoCol])
	{
	maxOppoRow = i;
	maxOppoCol = j;
	}
	}
	}

	if (maxMyRow == maxOppoRow && maxMyCol == maxOppoCol)
	{
	putDown(maxMyRow, maxMyCol);
	}
	else if (myAdvan[maxMyRow][maxMyCol] > oppoAdvan[maxOppoRow][maxOppoCol])
	{
	cout << "my chance!!!!!!!!!!!!!!!!!" << endl;
	//cout << "putDown!!!!!" << maxMyRow << " " <<maxMyCol << endl;
	putDown(maxMyRow, maxMyCol);
	}
	else if (myAdvan[maxMyRow][maxMyCol] < oppoAdvan[maxOppoRow][maxOppoCol])
	{
	cout << "opposite chance!!!!!!!!!!!!!!!!!" << endl;
	//cout << "putDown!!!!!" << maxOppoRow << " " <<maxOppoCol << endl;
	putDown(maxOppoRow, maxOppoCol);
	}
	else if (myAdvan[maxMyRow][maxMyCol] == oppoAdvan[maxOppoRow][maxOppoCol] && myAdvan[maxMyRow][maxMyCol] > 100)
	{
	putDown(maxMyRow, maxMyCol);
	}
	else
	{
	int r = -1, c = -1;
	// printf("%s\n", lastMsg());
	while (!(r >= 0 && r < ROWS && c >= 0 && c < COLS && board[r][c].empty))
	{
	cout << "Random!!!!!!!!!!!!!!!" << endl;
	r = random(15);
	c = random(15);
	// System.out.println("Rand " + r + " " + c);
	}
	// saveChessBoard();
	putDown(r, c);
	}
	}
	else
	{
	if (board[7][7].empty)
	{
	putDown(7, 7);
	}
	else
	{
	putDown(7, 8);
	}
	}


	*/
	stepCount++;
	Start = false;
}
/*int r = -1, c = -1;
// printf("%s\n", lastMsg());
while (!(r >= 0 && r < ROWS && c >= 0 && c < COLS && board[r][c].empty))
{
r = random(15);
c = random(15);
for (int i = 0; i < ROWS; i++)
{
for (int j = 0; j < COLS; j++)
{
if (!board[i][j].empty)
{
cout << "             " << board[i][j].color << endl;
}
}
}
// System.out.println("Rand " + r + " " + c);
}
// saveChessBoard();
putDown(r, c);*/

/*if (downCount >= 4)
{
attack4Count++;
}*/
/*if (rightCount >= 4)
{
attack4Count++;
}*/
/*if (rightUpCount >= 4)
{
attack4Count++;
}*/
/*if (rightDownCount >= 4)
{
attack4Count++;
}*/

/*int tempCompeteCount = 0;
for (int i = 0; i < ROWS; i++)
{
for (int j = 0; j < COLS; j++)
{
tempCompete[tempCompeteCount].col = j;
tempCompete[tempCompeteCount].row = i;
tempCompete[tempCompeteCount].myAdvan = myAdvan[i][j];
tempCompete[tempCompeteCount].oppoAdvan = oppoAdvan[i][j];
if (myAdvan[i][j] >= oppoAdvan[i][j])
{
tempCompete[tempCompeteCount].relativeAdvan = myAdvan[i][j];
}
else
{
tempCompete[tempCompeteCount].relativeAdvan = oppoAdvan[i][j];

}
tempCompeteCount++;
}
}*/

/*
int maxMyRow = 0;
int maxMyCol = 0;
int maxOppoRow = 0;
int maxOppoCol = 0;
for (int i = 0; i < ROWS; i++)
{
for (int j = 0; j < COLS; j++)
{
if (tempCompete[i*ROWS + j].myAdvan > tempCompete[maxMyRow*ROWS + maxMyCol].myAdvan)
{
maxMyRow = i;
maxMyCol = j;
}
if (tempCompete[i*ROWS + j].oppoAdvan > tempCompete[maxOppoRow*ROWS + maxOppoCol].oppoAdvan)
{
maxOppoRow = i;
maxOppoCol = j;
}
}
}

if (maxMyRow == maxOppoRow && maxMyCol == maxOppoCol)
{
putDown(maxMyRow, maxMyCol);
}
else if (tempCompete[maxMyRow*ROWS + maxMyCol].myAdvan > tempCompete[maxOppoRow*ROWS + maxOppoCol].oppoAdvan)
{
cout << "my chance!!!!!!!!!!!!!!!!!" << endl;
//cout << "putDown!!!!!" << maxMyRow << " " <<maxMyCol << endl;
putDown(maxMyRow, maxMyCol);
}
else if (tempCompete[maxMyRow*ROWS + maxMyCol].myAdvan < tempCompete[maxOppoRow*ROWS + maxOppoCol].oppoAdvan)
{
cout << "opposite chance!!!!!!!!!!!!!!!!!" << endl;
//cout << "putDown!!!!!" << maxOppoRow << " " <<maxOppoCol << endl;
putDown(maxOppoRow, maxOppoCol);
}
else if (tempCompete[maxMyRow*ROWS + maxMyCol].myAdvan == tempCompete[maxOppoRow*ROWS + maxOppoCol].oppoAdvan && tempCompete[maxMyRow*ROWS + maxMyCol].myAdvan > 100)
{
putDown(maxMyRow, maxMyCol);
}
else
{
int r = -1, c = -1;
// printf("%s\n", lastMsg());
while (!(r >= 0 && r < ROWS && c >= 0 && c < COLS && board[r][c].empty))
{
cout << "Random!!!!!!!!!!!!!!!" << endl;
r = random(15);
c = random(15);
// System.out.println("Rand " + r + " " + c);
}
// saveChessBoard();
putDown(r, c);
}
*/
/*void loopStep()
{
Compete *tempCompete = new Compete[ROWS*COLS];
memset(tempCompete, 0, sizeof(Compete) * ROWS*COLS);
CalculateAdvantage(tempCompete);

Compete *compete = new Compete[1000000];
memset(compete, 0, sizeof(Compete) * 1000000);

Square tempBoard[ROWS][COLS];
for (int i = 0; i < ROWS; i++)
{
for (int j = 0; j < COLS; j++)
{
tempBoard[i][j].color = board[i][j].color;
tempBoard[i][j].empty = board[i][j].empty;

}
}

sort(tempCompete);//只排列需要数目的
for (int i = 0; i < MaxTopStep; i++)
{
compete[i].col = tempCompete[i].col;
compete[i].row = tempCompete[i].row;
//compete[i].myAdvan = tempCompete[i].myAdvan;
//compete[i].oppoAdvan = tempCompete[i].oppoAdvan;
compete[i].relativeAdvan = tempCompete[i].relativeAdvan;
}

int startNumber = 0;
int endNumber = MaxTopStep;

for (int i = 0; i < MaxLoopCount; i++)
{
memset(tempCompete, 0, sizeof(Compete) * ROWS*COLS);
int hypoRow = -1;
int hypoCol = -1;

for (int j = startNumber; j < endNumber; j++)
{
tempBoard[compete[j].row][compete[j].col].color = ownColor;
tempBoard[compete[j].row][compete[j].col].empty = false;
}


for (int j = startNumber; j < endNumber; j++)
{

}

if (startNumber == 0)
{
startNumber = MaxTopStep;
}
else
{
startNumber *= MaxTopStep;
}

endNumber *= MaxTopStep;

}
}*/