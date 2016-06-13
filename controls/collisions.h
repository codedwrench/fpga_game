#ifndef COLLISION_H
#define COLLISION_H
#include "main.h"
alt_u8 getArea(alt_u16 x, alt_u8 y,alt_u8 w,alt_u8 h, alt_u16 collisionX,alt_u8 collisionY);
alt_u8 checkWallAndDoor(alt_u16 x,alt_u8 y);
alt_u8 moveCrate(alt_u16 x,alt_u8 y,alt_u16 *crateX, alt_u16 *crateY,alt_u8 dir);
void createDoor(alt_u8 doornumber);
void handleCollisions(alt_u16 x, alt_u8 y,alt_u8 dir, alt_u8 pNum, alt_u8 *willCollide);
#endif
