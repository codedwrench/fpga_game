#include <stdio.h>
#include "includes.h"
#include <alt_types.h>
#include <os/alt_sem.h>
#include "graphicslib.h"
#include "gamelib.h"
#include "address_map_nios2.h"
#define BUF_SIZE 5000000			// about 10 seconds of buffer (@ 48K samples/sec)
#define BUF_THRESHOLD 96			// 75% of 128 word buffer

volatile short* pixel_buffer 	= (short*)	0x08000000;
volatile char* character_buffer	= (char*)	0x09000000;
volatile short* buffer_register	= (short*)	0x10003060;
volatile short* dma_control		= (short*)	0x1000306C;
volatile int* audio_ptr			= (int*)	0x10003040;			// audio port address
volatile int* RED_LED_ptr		= (int*)	RED_LED_BASE;		// RED LED address
volatile int* JTAG_UART_ptr 	= (int*)	JTAG_UART_BASE;		// JTAG UART address
int left_buffer[BUF_SIZE];										// left speaker
int right_buffer[BUF_SIZE];										// right speaker
int fifospace, leftdata, rightdata;

alt_u16 doors[MAX_DOORS][2];
alt_u16 buttons[MAX_BUTTONS][2];

Player players[MAX_PLAYERS];
Level level;
Level* level_ptr = &level;
Button* button_ptr = &level.buttons[0];

/* Definition of Task Stacks */
#define   TASK_STACKSIZE       2048
OS_STK    PlayerTask_STK[MAX_PLAYERS][TASK_STACKSIZE];
OS_STK    ControlsTask_STK[TASK_STACKSIZE];
OS_STK    LevelTask_STK[TASK_STACKSIZE];
OS_STK    InitLevelTask_STK[TASK_STACKSIZE];
ALT_SEM(display)
ALT_SEM(audio)
ALT_SEM(player)
ALT_SEM(button)
ALT_SEM(level_sem)

/* Definition of Task Priorities */
#define LEVEL_PRIORITY			6
#define INITLEVEL_PRIORITY		5
#define PLAYER_PRIORITY			8
#define CONTROLS_PRIORITY		7

void playTone(int height, int time)
{
	signed long high = 2147483392;
	signed long low = -2147483648;
	int  buffer_index = 0;
	fifospace = 0;
	int i = 0;
	for( i = 0; i<time*10000;i++)
	{

		if(buffer_index < height)
		{
			*(audio_ptr + 2) = high;
			*(audio_ptr + 3) = high;
		}
		else if (buffer_index > height)
		{
			*(audio_ptr + 2) = low;
			*(audio_ptr + 3) = low;
		}
		if(buffer_index == 1+height*2)
		{
			buffer_index = 0;
		}


		++buffer_index;

	}
}
alt_8 getButton(Button* btn_ptr, alt_u16 x, alt_u16 y)
{
	ALT_SEM_PEND(button, 0);
	for (btn_ptr = &level.buttons[0]; btn_ptr != &level.buttons[MAX_BUTTONS-1]; btn_ptr++)
	{
		if (!btn_ptr->pressed && (btn_ptr->x > 0 && btn_ptr->y > 0))
		{
			if ((x == btn_ptr->x || x == btn_ptr->x+BUTTON_SIZE) && (y == btn_ptr->y || y == btn_ptr->y+BUTTON_SIZE))
			{
				btn_ptr->pressed = 1;
				ALT_SEM_POST(button);
				return btn_ptr->door;
			}
		}
	}
	ALT_SEM_POST(button);
	return -1;

}

void PlayerTask(void* pdata)
{
	ALT_SEM_PEND(player, 0);

	alt_u8 pNum = (alt_u8*) pdata;
	alt_u8 willCollide = 0;
	alt_8 btnPressed = -1;
	alt_u16 x, y;

	// Draw initial playerbox
	ALT_SEM_PEND(display, 0);
	drawBox(pixel_buffer, PLAYER_COLOR/(pNum+1), players[pNum].x, players[pNum].y, PLAYER_SIZE, PLAYER_SIZE);
	ALT_SEM_POST(display);

	while(1)
	{
		waitForVSync(buffer_register, dma_control);
		OSTimeDly(PLAYER_SPEED);

		if(players[pNum].yDir == DOWN)
		{
			y = players[pNum].y+PLAYER_SIZE+2;
			for (x = players[pNum].x-1; x < players[pNum].x + PLAYER_SIZE+2; x++)
			{
				switch(level.map[x][y])
				{
				case WALL:
				case WALL_CRATE:
				case WALL_INVIS:
				case DOOR:
					btnPressed = -1;
					willCollide = 1;
					break;
				case BUTTON:
					btnPressed = getButton(button_ptr, x, y);
					willCollide = 0;
					break;
				default:
					btnPressed = -1;
					willCollide = 0;
					break;
				}
				if (willCollide || btnPressed >= 0)
					break;
			}
			if (!willCollide)
				players[pNum].y++;
			if (btnPressed >= 0)
			{
				level_ptr->doors[btnPressed].open = 1;
			}
		}
		else if(players[pNum].yDir == UP)
		{
			y = players[pNum].y-2;
			for (x = players[pNum].x-1; x < players[pNum].x + PLAYER_SIZE+2; x++)
			{
				switch(level.map[x][y])
				{
				case WALL:
				case WALL_CRATE:
				case WALL_INVIS:
				case DOOR:
					btnPressed = -1;
					willCollide = 1;
					break;
				case BUTTON:
					btnPressed = getButton(button_ptr, x, y);
					willCollide = 0;
					break;
				default:
					btnPressed = -1;
					willCollide = 0;
					break;
				}
				if (willCollide || btnPressed >= 0)
					break;
			}
			if (!willCollide)
				players[pNum].y--;
			if (btnPressed >= 0)
				level_ptr->doors[btnPressed].open = 1;
		}
		if(players[pNum].xDir == RIGHT)
		{
			x = players[pNum].x+PLAYER_SIZE+2;
			for (y = players[pNum].y-1; y < players[pNum].y + PLAYER_SIZE+2; y++)
			{
				switch(level.map[x][y])
				{
				case WALL:
				case WALL_CRATE:
				case WALL_INVIS:
				case DOOR:
					btnPressed = -1;
					willCollide = 1;
					break;
				case BUTTON:
					btnPressed = getButton(button_ptr, x, y);
					willCollide = 0;
					break;
				default:
					btnPressed = -1;
					willCollide = 0;
					break;
				}
				if (willCollide || btnPressed >= 0)
					break;
			}
			if (!willCollide)
				players[pNum].x++;
			if (btnPressed >= 0)
				level_ptr->doors[btnPressed].open = 1;
		}
		else if(players[pNum].xDir == LEFT)
		{
			x = players[pNum].x-2;
			for (y = players[pNum].y-1; y < players[pNum].y + PLAYER_SIZE+2; y++)
			{
				switch(level.map[x][y])
				{
				case WALL:
				case WALL_CRATE:
				case WALL_INVIS:
				case DOOR:
					btnPressed = -1;
					willCollide = 1;
					break;
				case BUTTON:
					btnPressed = getButton(button_ptr, x, y);
					willCollide = 0;
					break;
				default:
					btnPressed = -1;
					willCollide = 0;
					break;
				}
				if (willCollide || btnPressed >= 0)
					break;
			}
			if (!willCollide)
				players[pNum].x--;
			if (btnPressed >= 0)
				level_ptr->doors[btnPressed].open = 1;
		}

		// Draw player
		ALT_SEM_PEND(display, 0);
		drawRect(pixel_buffer, BG_COLOR, players[pNum].x-1, players[pNum].y-1, PLAYER_SIZE+2, PLAYER_SIZE+2);
		drawBox(pixel_buffer, PLAYER_COLOR/(pNum+1), players[pNum].x, players[pNum].y, PLAYER_SIZE, PLAYER_SIZE);
		ALT_SEM_POST(display);

		ALT_SEM_POST(player);
	}
}
void ControlsTask(void* pdata)
{
	int data, i;
	char cmd[3];
	alt_u8 pNum;
	alt_8 action;

	for (i = 0; i < sizeof(cmd)/sizeof(cmd[0]); i++)
		cmd[i] = 0;
	i = 0;

	/* read and echo characters */
	while(1)
	{
		data = *(JTAG_UART_ptr);		 		// read the JTAG_UART data register
		if (data & 0x00008000)					// check RVALID to see if there is new data
		{
			data = data & 0x000000FF;			// the data is in the least significant byte
			/* echo the character */

			while ((char) data == '1' || (char) data == '2')
			{
				OSTimeDly(1);

				cmd[0] = data;
				data = *(JTAG_UART_ptr);		 		// read the JTAG_UART data register
				if (data & 0x00008000)					// check RVALID to see if there is new data
				{
					data = data & 0x000000FF;			// the data is in the least significant byte
					while (1)
					{
						OSTimeDly(1);

						cmd[1] = data;
						data = *(JTAG_UART_ptr);		 		// read the JTAG_UART data register
						if (data & 0x00008000)					// check RVALID to see if there is new data
						{
							data = data & 0x000000FF;			// the data is in the least significant byte
							while ((char) data == 'r' || (char) data == 'p')
							{
								OSTimeDly(1);

								cmd[2] = data;
								break;
							}
						}
						break;
					}
				}
				break;
			}
			if (cmd[0] != 0 && cmd[1] != 0 && cmd[2] != 0)
			{
				ALT_SEM_PEND(player, 0);
				pNum = cmd[0] - '0' - 1;

				switch(cmd[1])
				{
				case 'g':
					if(cmd[2] != 'r')
					{
						if (pNum == 0 || pNum == 1)
							players[pNum].yDir = DOWN;
					}
					else
						if (pNum == 0 || pNum == 1)
							players[pNum].yDir = NONE;
					break;
				case 'h':
					if(cmd[2] != 'r')
					{
						if (pNum == 0 || pNum == 1)
							players[pNum].yDir = UP;
					}
					else
						if (pNum == 0 || pNum == 1)
							players[pNum].yDir = NONE;
					break;
				case 'l':
					if(cmd[2] != 'r')
					{
						if (pNum == 0 || pNum == 1)
							players[pNum].xDir = LEFT;
					}
					else
						if (pNum == 0 || pNum == 1)
							players[pNum].xDir = NONE;
					break;
				case 'r':
					if(cmd[2] != 'r')
					{
						if (pNum == 0 || pNum == 1)
							players[pNum].xDir = RIGHT;
					}
					else
					{
						if (pNum == 0 || pNum == 1)
							players[pNum].xDir = NONE;
					}
					break;
				case '2':
					if(cmd[2] != 'r')
					{
						action = cmd[1] - '0';
						if (pNum == 0 || pNum == 1)
							players[pNum].action = action;
					}
					else
					{
						if (pNum == 0 || pNum == 1)
							players[pNum].action = -1;
					}
					break;
				default:
					break;
				}

				for (i = 0; i < sizeof(cmd)/sizeof(cmd[0]); i++)
					cmd[i] = 0;
				i = 0;

			}
			ALT_SEM_POST(player);

		}
		OSTimeDly(1);
	}
}
void LevelTask(void* pdata)
{
	OSTimeDly(100);

	ALT_SEM_PEND(display, 0);
	// Draw splitscreen wall
	drawCollision(pixel_buffer, level_ptr, 1, WALL, WALL_COLOR, 1,
			SCREEN_WIDTH/2 - SPLITSCREEN_WIDTH/2,							// x1
			0,																// y1
			SPLITSCREEN_WIDTH,												// width
			SCREEN_HEIGHT/2 - DOOR_SIZE-1 - WALL_SIZE/2);									// height
	OSTimeDly(5);
	drawCollision(pixel_buffer, level_ptr, 1, WALL, WALL_COLOR, 1,
			SCREEN_WIDTH/2 - SPLITSCREEN_WIDTH/2,
			SCREEN_HEIGHT/2 + DOOR_SIZE+2,
			SPLITSCREEN_WIDTH,
			SCREEN_HEIGHT/2 - DOOR_SIZE-1 - WALL_SIZE/2);
	OSTimeDly(5);

	// Left field walls
	drawCollision(pixel_buffer, level_ptr, 1, WALL, WALL_COLOR, 1,
			0,
			SCREEN_HEIGHT-1-20-WALL_SIZE,
			SCREEN_WIDTH/2 - SPLITSCREEN_WIDTH/2 -18,
			WALL_SIZE);
	OSTimeDly(5);
	drawCollision(pixel_buffer, level_ptr, 1, WALL, WALL_COLOR, 1,
			SCREEN_WIDTH/2 - SPLITSCREEN_WIDTH/2 -22,
			SCREEN_HEIGHT/2 - WALL_SIZE/2,
			WALL_SIZE,
			79);
	OSTimeDly(5);
	drawCollision(pixel_buffer, level_ptr, 1, WALL, WALL_COLOR, 1,
			SCREEN_WIDTH/2 - SPLITSCREEN_WIDTH/2 -17,
			SCREEN_HEIGHT/2 - WALL_SIZE/2,
			21,
			WALL_SIZE);
	OSTimeDly(5);
	drawCollision(pixel_buffer, level_ptr, 1, WALL, WALL_COLOR, 1,
			SCREEN_WIDTH/2 - SPLITSCREEN_WIDTH/2 -17,
			SCREEN_HEIGHT/2 + WALL_SIZE/2 + DOOR_SIZE,
			40,
			WALL_SIZE);
	OSTimeDly(5);
	drawCollision(pixel_buffer, level_ptr, 1, WALL, WALL_COLOR, 1,
			SCREEN_WIDTH/2 - SPLITSCREEN_WIDTH/2 - 65,
			SCREEN_HEIGHT/2 - WALL_SIZE/2 + 39,
			25,
			WALL_SIZE);
	OSTimeDly(5);
	drawCollision(pixel_buffer, level_ptr, 1, WALL, WALL_COLOR, 1,
			SCREEN_WIDTH/2 - SPLITSCREEN_WIDTH/2 - 70,
			SCREEN_HEIGHT/2 - WALL_SIZE/2 + 39,
			WALL_SIZE,
			57);
	OSTimeDly(5);
	drawCollision(pixel_buffer, level_ptr, 1, WALL, WALL_COLOR, 1,
			SCREEN_WIDTH/2 - SPLITSCREEN_WIDTH/2 - 44,
			SCREEN_HEIGHT/2 - WALL_SIZE/2,
			WALL_SIZE,
			38);
	OSTimeDly(5);
	drawCollision(pixel_buffer, level_ptr, 1, WALL, WALL_COLOR, 1,
			SCREEN_WIDTH/2 - SPLITSCREEN_WIDTH/2 - 101,
			SCREEN_HEIGHT/2 - DOOR_SIZE - WALL_SIZE/2 - 5,
			100,
			WALL_SIZE);
	OSTimeDly(5);
	drawCollision(pixel_buffer, level_ptr, 1, WALL, WALL_COLOR, 1,
			SCREEN_WIDTH/2 - SPLITSCREEN_WIDTH/2 - 101,
			SCREEN_HEIGHT/2 - DOOR_SIZE - WALL_SIZE/2,
			WALL_SIZE,
			70);
	OSTimeDly(5);
	drawCollision(pixel_buffer, level_ptr, 1, WALL, WALL_COLOR, 1,
			SCREEN_WIDTH/2 - SPLITSCREEN_WIDTH/2 - 132,
			SCREEN_HEIGHT/2 + 53,
			35,
			WALL_SIZE);
	OSTimeDly(5);
	drawCollision(pixel_buffer, level_ptr, 1, WALL, WALL_COLOR, 1,
			0,
			SCREEN_HEIGHT/2 + 25,
			35,
			WALL_SIZE);
	OSTimeDly(5);
	drawCollision(pixel_buffer, level_ptr, 1, WALL, WALL_COLOR, 1,
			31,
			SCREEN_HEIGHT/2 - DOOR_SIZE - WALL_SIZE/2 - 5,
			WALL_SIZE,
			31);
	OSTimeDly(5);
	drawCollision(pixel_buffer, level_ptr, 1, WALL, WALL_COLOR, 1,
			36,
			SCREEN_HEIGHT/2 + 4,
			20,
			WALL_SIZE);
	OSTimeDly(5);
	drawCollision(pixel_buffer, level_ptr, 1, WALL, WALL_COLOR, 1,
			17,
			SCREEN_HEIGHT/2 - 50,
			120,
			WALL_SIZE);
	OSTimeDly(5);
	drawCollision(pixel_buffer, level_ptr, 1, WALL, WALL_COLOR, 1,
			110,
			SCREEN_HEIGHT/2 - 75,
			WALL_SIZE,
			25);
	OSTimeDly(5);
	drawCollision(pixel_buffer, level_ptr, 1, WALL, WALL_COLOR, 1,
			115,
			SCREEN_HEIGHT/2 - 75,
			42,
			WALL_SIZE);
	OSTimeDly(5);
	drawCollision(pixel_buffer, level_ptr, 1, WALL, WALL_COLOR, 1,
			0,
			SCREEN_HEIGHT/2 - 75,
			80,
			WALL_SIZE);
	OSTimeDly(5);
	drawCollision(pixel_buffer, level_ptr, 1, WALL, WALL_COLOR, 1,
			0,
			20,
			80,
			WALL_SIZE);
	OSTimeDly(5);
	drawCollision(pixel_buffer, level_ptr, 1, WALL, WALL_COLOR, 1,
			98,
			20,
			60,
			WALL_SIZE);
	OSTimeDly(5);
	// Outer bounds
	drawCollision(pixel_buffer, level_ptr, 0, WALL_INVIS, WALL_INVIS, 0,
			0,
			0,
			SCREEN_WIDTH-1,
			SCREEN_HEIGHT-1);
	OSTimeDly(5);

	ALT_SEM_POST(display);

	while(1)
	{
		waitForVSync(buffer_register, dma_control);
		OSTimeDly(100);

		// Draw level
		ALT_SEM_PEND(display, 0);

		// Crate wall
		drawCollision(pixel_buffer, level_ptr, 0, WALL_CRATE, WALL_CRATE_COLOR, 1,
				SCREEN_WIDTH/2 - SPLITSCREEN_WIDTH/2,
				SCREEN_HEIGHT/2 - DOOR_SIZE - WALL_SIZE/2,
				SPLITSCREEN_WIDTH,
				DOOR_SIZE-1);
		drawCollision(pixel_buffer, level_ptr, 0, WALL_CRATE, WALL_CRATE_COLOR, 1,
				SCREEN_WIDTH/2 - SPLITSCREEN_WIDTH/2,
				SCREEN_HEIGHT/2 + WALL_SIZE/2,
				SPLITSCREEN_WIDTH,
				DOOR_SIZE-1);

		// Draw buttons and their respective doors
//		ALT_SEM_PEND(level_sem, 0);
		drawBtnAndDoor(pixel_buffer, level_ptr, 0, 1);
		drawBtnAndDoor(pixel_buffer, level_ptr, 1, 0);
		drawBtnAndDoor(pixel_buffer, level_ptr, 2, 1);
//		drawBtnAndDoor(pixel_buffer, level_ptr, 3, 1);
//		drawBtnAndDoor(pixel_buffer, level_ptr, 4, 1);
//		drawBtnAndDoor(pixel_buffer, level_ptr, 5, 1);
//		drawBtnAndDoor(pixel_buffer, level_ptr, 6, 1);
//		drawBtnAndDoor(pixel_buffer, level_ptr, 7, 1);
		drawBtnAndDoor(pixel_buffer, level_ptr, 8, 1);
//		drawBtnAndDoor(pixel_buffer, level_ptr, 9, 1);
//		drawBtnAndDoor(pixel_buffer, level_ptr, 10, 0);
//		drawBtnAndDoor(pixel_buffer, level_ptr, 11, 0);
//		drawBtnAndDoor(pixel_buffer, level_ptr, 12, 0);
//		drawBtnAndDoor(pixel_buffer, level_ptr, 13, 0);
//		drawBtnAndDoor(pixel_buffer, level_ptr, 14, 0);
//		drawBtnAndDoor(pixel_buffer, level_ptr, 15, 0);
//		drawBtnAndDoor(pixel_buffer, level_ptr, 16, 0);
//		drawBtnAndDoor(pixel_buffer, level_ptr, 17, 0);
//		drawBtnAndDoor(pixel_buffer, level_ptr, 18, 0);
//		drawBtnAndDoor(pixel_buffer, level_ptr, 19, 0);
//		ALT_SEM_POST(level_sem);
		ALT_SEM_POST(display);
	}
}
int fillDoor(alt_u8 door ,alt_u16 x,alt_u16 y)
{
	level_ptr->doors[door].x = x;
	level_ptr->doors[door].y  = y;
	OSTimeDly(2);
	if(level_ptr->doors[door].x != x || level_ptr->doors[door].y != y)
	{
		return 1;
	}
	return 0;
}
int fillButton(alt_u8 button ,alt_u16 x,alt_u16 y)
{
	level_ptr->buttons[button].x = x;
	level_ptr->buttons[button].y  = y;
	OSTimeDly(2);
	if(level_ptr->buttons[button].x != x || level_ptr->buttons[button].y != y)
	{
		return 1;
	}
	return 0;
}
void InitLevelTask(void* pdata)
{
	alt_u8 err = 0;
    ALT_SEM_PEND(level_sem, 0);
	int i;
	for (i = 0; i < MAX_DOORS; i++)
	{
		level_ptr->doors[i].btn = i;
		level_ptr->doors[i].open = 0;
		level_ptr->doors[i].opening = 1;
		level_ptr->buttons[i].door = i;
		level_ptr->buttons[i].pressed = 0;
		level_ptr->buttons[i].pressing = 1;

	}
	err = fillDoor(0, 141, 215);
	err = fillDoor(1, 136, 198);
	err = fillDoor(2, 119, 157);
	err = fillDoor(3,40,50);
	err = fillDoor(4,50,50);
	err = fillDoor(5,60,50);
	err = fillDoor(6,70,50);
	err = fillDoor(7,80,50);
	err = fillDoor(8,0,70);
	err = fillDoor(9,81,20);
	err = fillDoor(10,10,70);
	err = fillDoor(11,20,70);
	err = fillDoor(12,30,70);
	err = fillDoor(13,40,70);
	err = fillDoor(14,50,70);
	err = fillDoor(15,60,70);
	err = fillDoor(16,70,70);
	err = fillDoor(17,80,70);
	err = fillDoor(18,90,70);
	err = fillDoor(19,100,70);

	err = fillButton(0, 170, 10);
	err = fillButton(1, 180, 10);
	err = fillButton(2, 190, 10);
	err = fillButton(3, 200, 10);
	err = fillButton(4, 210, 10);
	err = fillButton(5, 220, 10);
	err = fillButton(6, 230, 10);
	err = fillButton(7, 240, 10);
	err = fillButton(8, 250, 10);
	err = fillButton(9, 260, 30);
	err = fillButton(10,10,30);
	err = fillButton(11,20,30);
	err = fillButton(12,30,30);
	err = fillButton(13,40,30);
	err = fillButton(14,50,30);
	err = fillButton(15,60,30);
	err = fillButton(16,70,30);
	err = fillButton(17,80,30);
	err = fillButton(18,90,30);
	err = fillButton(19,100,30);

    ALT_SEM_POST(level_sem);
	int x = 12345;
	OSTaskDel(OS_PRIO_SELF);
}
int main(void)
{

	OSInit();

	int err = ALT_SEM_CREATE(&display, 1);
	if(err != 0)
		printf("Semaphore not created\n");
	 err = ALT_SEM_CREATE(&audio, 1);
	if(err != 0)
		printf("Semaphore not created\n");
	err = ALT_SEM_CREATE(&player, 1);
	if(err!= 0)
		printf("Semaphore not created\n");
	err = ALT_SEM_CREATE(&button, 1);
	if(err!= 0)
		printf("Semaphore not created\n");
	err = ALT_SEM_CREATE(&level_sem, 1);
	if(err!= 0)
		printf("Semaphore not created\n");

	*(dma_control) &= (1<<2); //Enable DMA controller

	// Init players
	Player p1 = { SCREEN_WIDTH/4, SCREEN_HEIGHT-PLAYER_SIZE-PLAYER_SIZE/2, "TestGuy1\0", NONE };
	Player p2 = { SCREEN_WIDTH/4*3, SCREEN_HEIGHT-PLAYER_SIZE-PLAYER_SIZE/2, NONE, "TestGuy2\0", NONE };
	players[0] = p1;
	players[1] = p2;

	fillScreen(pixel_buffer, 0x00F0);
	waitForVSync(buffer_register, dma_control);

	OSTaskCreateExt(LevelTask,
			0,
			(void *)&LevelTask_STK[TASK_STACKSIZE-1],
			LEVEL_PRIORITY,
			LEVEL_PRIORITY,
			LevelTask_STK,
			TASK_STACKSIZE,
			NULL,
			0);

	alt_u8 pNum;
	for (pNum = 0; pNum < MAX_PLAYERS; pNum++)
	{
		OSTaskCreateExt(PlayerTask,
			pNum,
			(void *)&PlayerTask_STK[pNum][TASK_STACKSIZE-1],
			PLAYER_PRIORITY + pNum,
			PLAYER_PRIORITY + pNum,
			PlayerTask_STK[pNum],
			TASK_STACKSIZE,
			NULL,
			0);
	}
//	OSTaskCreateExt(task2,
//			pixel_buffer,
//			(void *)&task2_stk[TASK_STACKSIZE-1],
//			TASK2_PRIORITY,
//			TASK2_PRIORITY,
//			task2_stk,
//			TASK_STACKSIZE,
//			NULL,
//			0);

	OSTaskCreateExt(ControlsTask,
			0,
			(void *)&ControlsTask_STK[TASK_STACKSIZE-1],
			CONTROLS_PRIORITY,
			CONTROLS_PRIORITY,
			ControlsTask_STK,
			TASK_STACKSIZE,
			NULL,
			0);

	OSTaskCreateExt(InitLevelTask,
			0,
			(void *)&InitLevelTask_STK[TASK_STACKSIZE-1],
			INITLEVEL_PRIORITY,
			INITLEVEL_PRIORITY,
			InitLevelTask_STK,
			TASK_STACKSIZE,
			NULL,
			0);

	OSStart();
	return 0;
}
