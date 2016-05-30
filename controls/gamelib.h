#ifndef GAMELIB
#define GAMELIB
#include <alt_types.h>
#include <stdlib.h>
#define SCREEN_WIDTH 320
#define SCREEN_HEIGHT 240
#define PLAYER_SIZE 8				// player size (width & height)
#define BUTTON_SIZE 8
#define WALL_WIDTH 4
#define DOOR_SIZE 16
#define SPLITSCREEN_WIDTH 6
#define BG_COLOR 0x00F0
#define WALL_COLOR 0xFFFF
#define WALL_CRATE_COLOR 0xFFF0
#define PLAYER_COLOR 0xFF00
#define PLAYER_SPEED 5 				// OSTimeDly in player task
#define MAX_BUTTONS 10

typedef struct Player {
	alt_u16 x, y;
	alt_u8* name;
} Player;

typedef struct Button {
	alt_u16 x, y;
	alt_u8 door;
	alt_u8 pressed;
} Button;

typedef struct Level {
	alt_u8 map[SCREEN_WIDTH][SCREEN_HEIGHT];
	Button buttons[MAX_BUTTONS];
} Level;

typedef enum ObjectType {
	GROUND,
	WALL,
	WALL_CRATE,
	WALL_INVIS,
	BUTTON,
	BUTTON_CRATE
};

#endif
