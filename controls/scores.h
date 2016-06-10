#include "main.h"
//High Scores
short int* scoresFile_ptr;

//Function definitions
short int saveScoresToSDFile(short int *f);
void setInitScore(short int* file);
void setScore(alt_8 pos, alt_u8 name[], short int* f);
alt_8 checkScore();
void loadScores(short int file);
void addPenalty(alt_u8 n,OS_EVENT * timer);
