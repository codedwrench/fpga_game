#ifndef SCORES_H
#define SCORES_H
#include "main.h"
//High Scores
short int* scoresFile_ptr;

//Function definitions
short int saveScoresToSDFile(short int *f);
void setInitScore(short int* file);
void setScore(alt_8 pos, char name[], short int* f);
alt_8 checkScore();
void loadScores(short int file);
void addPenalty(alt_u8 n,OS_EVENT * timer);

//Task definitions
void DrawTimerTask(void *pdata);
void TimerTask(void *pdata);
#endif
