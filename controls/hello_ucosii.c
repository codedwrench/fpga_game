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
volatile short* buffer_register	= (short*)	0x10003060;
volatile short* dma_control		= (short*)	0x1000306C;
volatile char* character_buffer	= (char*)	0x09000000;
volatile int* audio_ptr			= (int *)	0x10003040;			// audio port address
volatile int* RED_LED_ptr		= (int*)	RED_LED_BASE;		// RED LED address
volatile int* JTAG_UART_ptr 	= (int*)	JTAG_UART_BASE;		// JTAG UART address
int left_buffer[BUF_SIZE];										// left speaker
int right_buffer[BUF_SIZE];										// right speaker
int fifospace, leftdata, rightdata;

Player players[MAX_PLAYERS];
Level level;
Level* level_ptr = &level;
Button* button_ptr = &level.buttons[0];

/* Definition of Task Stacks */
#define   TASK_STACKSIZE       2048
OS_STK    PlayerTask_STK[MAX_PLAYERS][TASK_STACKSIZE];
OS_STK    ControlsTask_STK[TASK_STACKSIZE];
OS_STK    LevelTask_STK[TASK_STACKSIZE];
ALT_SEM(display)
ALT_SEM(audio)
ALT_SEM(player)
ALT_SEM(button)

/* Definition of Task Priorities */
#define LEVEL_PRIORITY			5
#define PLAYER_PRIORITY			7
#define CONTROLS_PRIORITY		6

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
				if (players[pNum].action == 2)
					level_ptr->doors[btnPressed].open = 1;
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
				if (players[pNum].action == 2)
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
				if (players[pNum].action == 2)
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
				if (players[pNum].action == 2)
					level_ptr->doors[btnPressed].open = 1;
		}

		// Draw player
		ALT_SEM_PEND(display, 0);
		drawRect(pixel_buffer, BG_COLOR, players[pNum].x-1, players[pNum].y-1, PLAYER_SIZE+2, PLAYER_SIZE+2);
		drawRect(pixel_buffer, PLAYER_COLOR/(pNum+1), players[pNum].x, players[pNum].y, PLAYER_SIZE, PLAYER_SIZE);
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
	int toggle = 1;
	alt_u16 c;

	level_ptr->doors[0].x = 20;
	level_ptr->doors[0].y = 100;
	level_ptr->doors[0].btn = 0;
	level_ptr->doors[0].open = 0;
	level_ptr->doors[1].x = 120;
	level_ptr->doors[1].y = 100;
	level_ptr->doors[1].btn = 1;
	level_ptr->doors[1].open = 0;
	level_ptr->buttons[0].x = 20;
	level_ptr->buttons[0].y = 20;
	level_ptr->buttons[0].door = 0;
	level_ptr->buttons[0].pressed = 0;
	level_ptr->buttons[1].x = 120;
	level_ptr->buttons[1].y = 20;
	level_ptr->buttons[1].door = 1;
	level_ptr->buttons[1].pressed = 0;

	while(1)
	{
		waitForVSync(buffer_register, dma_control);
		OSTimeDly(100);

		// Draw level
		ALT_SEM_PEND(display, 0);

		// Middle wall
		drawCollisionRect(pixel_buffer, level_ptr, WALL, WALL_COLOR, 1,
				SCREEN_WIDTH/2 - SPLITSCREEN_WIDTH/2,							// x1
				0,																// y1
				SPLITSCREEN_WIDTH,												// width
				SCREEN_HEIGHT/2 - DOOR_SIZE-1);									// height
		drawCollisionRect(pixel_buffer, level_ptr, WALL, WALL_COLOR, 1,
				SCREEN_WIDTH/2 - SPLITSCREEN_WIDTH/2,
				SCREEN_HEIGHT/2 + DOOR_SIZE,
				SPLITSCREEN_WIDTH,
				SCREEN_HEIGHT/2 - DOOR_SIZE-1);

		// Middle crate wall
		drawCollisionRect(pixel_buffer, level_ptr, WALL_CRATE, WALL_CRATE_COLOR, 1,
				SCREEN_WIDTH/2 - SPLITSCREEN_WIDTH/2,
				SCREEN_HEIGHT/2 - DOOR_SIZE,
				SPLITSCREEN_WIDTH,
				DOOR_SIZE-1);
		drawCollisionRect(pixel_buffer, level_ptr, WALL_CRATE, WALL_CRATE_COLOR, 1,
				SCREEN_WIDTH/2 - SPLITSCREEN_WIDTH/2,
				SCREEN_HEIGHT/2,
				SPLITSCREEN_WIDTH,
				DOOR_SIZE-1);

		drawCollisionRect(pixel_buffer, level_ptr, WALL, WALL_COLOR, 1,
				0,
				SCREEN_HEIGHT-1-20-WALL_SIZE,
				SCREEN_WIDTH/2 - SPLITSCREEN_WIDTH/2 -1 -DOOR_SIZE,
				WALL_SIZE);

		if (toggle)
		{
			c = 0xFF00;
			toggle = 0;
		}
		else
		{
			c = 0x00FF;
			toggle = 1;
		}
		drawBtnAndDoor(pixel_buffer, level_ptr, 0);
		drawBtnAndDoor(pixel_buffer, level_ptr, 1);

		// Outer bounds
		drawCollisionRect(pixel_buffer, level_ptr, WALL_INVIS, WALL_INVIS, 0,
				0,
				0,
				SCREEN_WIDTH-1,
				SCREEN_HEIGHT-1);
		ALT_SEM_POST(display);
	}
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

	*(dma_control) &= (1<<2); //Enable DMA controller

	// Init players
	Player p1 = { SCREEN_WIDTH/4, 10, NONE, "TestGuy1\0" };
	Player p2 = { SCREEN_WIDTH/4*3, 10, NONE, "TestGuy2\0" };
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

	OSStart();
	return 0;
}
