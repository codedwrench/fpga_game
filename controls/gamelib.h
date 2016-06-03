#ifndef GAMELIB
#define GAMELIB
#include <alt_types.h>
#include <stdlib.h>
#define SCREEN_WIDTH 320
#define SCREEN_HEIGHT 240
#define MAX_PLAYERS 2
#define PLAYER_SIZE 8				// player size (width & height)
#define BUTTON_SIZE 8
#define WALL_SIZE 4
#define DOOR_SIZE 16
#define SPLITSCREEN_WIDTH 4
#define BG_COLOR 0x00F0
#define WALL_COLOR 0xFFFF
#define WALL_CRATE_COLOR 0xFFF0
#define CRATE_COLOR 0xF000
#define PLAYER_COLOR 0xFF00
#define BUTTON_COLOR 0x0F00
#define DOOR_COLOR 0x0FFF
#define PLAYER_SPEED 5 				// OSTimeDly in player task
#define MAX_BUTTONS 20
#define MAX_DOORS 20
#define MAX_CRATES 1

typedef struct Player {
	alt_u16 x, y;
	alt_u8 yDir, xDir;
	alt_8 action;
	alt_u8* name;
} Player;

typedef struct Button {
	alt_u8 coords[2];
} Button;

typedef struct Door {
	alt_u8 coords[2];
	alt_u8 vert;
} Door;
typedef struct Crate {
	alt_u8 coords[2];
} Crate;

typedef enum ObjectType {
	GROUND,
	WALL,
	WALL_CRATE,
	WALL_INVIS,
	BUTTON,
	BUTTON_CRATE,
	DOOR
} ObjectType;

typedef enum Direction {
	NONE,
	UP,
	DOWN,
	LEFT,
	RIGHT
} Direction;

#endif
