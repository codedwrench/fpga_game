#ifndef MAIN_H
#define MAIN_H

//Includes
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include "includes.h"
#include <alt_types.h>
#include <os/alt_sem.h>
#include <os/alt_flag.h>
#include "graphicslib.h"
#include "gamelib.h"
#include "scores.h"
#include <Altera_UP_SD_Card_Avalon_Interface.h>
#include "address_map_nios2.h"

//Finish Flag definitions
#define FINISH_1 0x01
#define FINISH_2 0x02

//Stack definitions
#define TASK_STACKSIZE              2048
#define INITLEVEL_PRIORITY		5
#define TIMER_PRIORITY			    9
#define MAINMENU_PRIORITY		12
#define CONTROLS_PRIORITY		6
#define PLAYER_PRIORITY			7
#define DRAWTIMER_PRIORITY	10
#define FINISH_PRIORITY			    11

//Stacks in main.c
extern OS_STK PlayerTask_STK[MAX_PLAYERS][TASK_STACKSIZE];
extern OS_STK ControlsTask_STK[TASK_STACKSIZE];
extern OS_STK TimerTask_STK[TASK_STACKSIZE];
extern OS_STK DrawTimerTask_STK[TASK_STACKSIZE];
extern OS_STK InitLevelTask_STK[TASK_STACKSIZE];
extern OS_STK FinishTask_STK[TASK_STACKSIZE];
extern OS_STK MainMenuTask_STK[TASK_STACKSIZE];

//Variables in main.c
extern volatile short* pixel_buffer;
extern volatile char* character_buffer;
extern volatile short* buffer_register;
extern volatile short* dma_control;
extern volatile int* JTAG_UART_ptr;
extern alt_u8 sec, min;
extern Door doors[MAX_DOORS];
extern Button buttons[MAX_DOORS];
extern Crate crates[MAX_CRATES];
extern Player players[MAX_PLAYERS];
extern char highScores[3][2][6];
extern short int* scoresFile_ptr;

//Function definitions
void movePlayer(alt_u8 pNum, alt_u8 dir);
short int openSDFile(alt_up_sd_card_dev* sd_card, char name[]);
void DrawTimerTask(void* pdata);
void TimerTask(void* pdata);
void PlayerTask(void* pdata);
void ControlsTask(void* pdata);
void InitLevelTask(void* pdata);
void MainMenuTask(void* pdata);
void FinishTask(void* pdata);

#endif
