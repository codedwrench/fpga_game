#ifndef PLAYER_H
#define PLAYER_H
#include "main.h"
void movePlayer(alt_u8 pNum, alt_u8 dir);
void PlayerTask(void* pdata);
void ControlsTask(void* pdata);
#endif
