#include <stdio.h>
#include "includes.h"
#include <alt_types.h>
#include <os/alt_sem.h>
#include "graphicslib.h"
#include "gamelib.h"
#include "address_map_nios2.h"
#define BUF_SIZE 5000000			// about 10 seconds of buffer (@ 48K samples/sec)
#define BUF_THRESHOLD 96			// 75% of 128 word buffer

volatile short* pixel_buffer_start = (short*) 0x08000000;
volatile short* buffer_register = (short*) 0x10003060;
volatile short* dma_control = (short*) 0x1000306C;
volatile char* character_buffer = (char*) 0x09000000;
volatile int* audio_ptr = (int *) 0x10003040;				// audio port address
volatile int* RED_LED_ptr 		= (int*) RED_LED_BASE;		// RED LED address
volatile int* JTAG_UART_ptr 	= (int*) JTAG_UART_BASE;	// JTAG UART address
int left_buffer[BUF_SIZE];									// left speaker
int right_buffer[BUF_SIZE];									// right speaker
int fifospace, leftdata, rightdata;
unsigned char p1x, p1y, p2x, p2y;

Player player1 = { SCREEN_WIDTH/4, 10, "TestGuy1\0" };
Player player2 = { SCREEN_WIDTH/4*3, 10, "TestGuy2\0" };
Player* player1_ptr = &player1;
Player* player2_ptr = &player2;
Level level;
Level* level_ptr = &level;

volatile Button* button_ptr = &level.buttons[0];

/* Definition of Task Stacks */
#define   TASK_STACKSIZE       2048
OS_STK    task1_stk[TASK_STACKSIZE];
OS_STK    task2_stk[TASK_STACKSIZE];
OS_STK    getcontrols_stk[TASK_STACKSIZE];
OS_STK    draw_level_stk[TASK_STACKSIZE];
ALT_SEM(display)
ALT_SEM(audio)
ALT_SEM(player)
ALT_SEM(button)

/* Definition of Task Priorities */
#define DRAW_LEVEL_PRIORITY		5
#define TASK1_PRIORITY			6
#define TASK2_PRIORITY			7
#define GETCONTROLS_PRIORITY	8


void playtone(int height, int time)
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
Button* getButton(Button* btn_ptr, alt_u16 x, alt_u16 y)
{
	ALT_SEM_PEND(button, 0);
//	for (btn_ptr = &level.buttons[0]; btn_ptr != &level.buttons[MAX_BUTTONS-1]; btn_ptr++)
//	{
		if (!btn_ptr->pressed && (btn_ptr->x > 0 && btn_ptr->y > 0))
		{
			if (x == btn_ptr->x || x == btn_ptr->x+BUTTON_SIZE ||
					y == btn_ptr->y || y == btn_ptr->y+BUTTON_SIZE)
			{
				btn_ptr->pressed = 1;
				ALT_SEM_POST(button);
				return btn_ptr;
			}
		}
//	}
	ALT_SEM_POST(button);
	return btn_ptr;
}
void task1(void* pdata)
{
	ALT_SEM_PEND(player, 0);

//	OSTimeDly(1);
	volatile short * pixel_ctrl_ptr = (volatile short *) pdata;

	// Draw initial playerbox
	ALT_SEM_PEND(display, 0);
	drawBox(pixel_ctrl_ptr, PLAYER_COLOR, player1.x, player1.y, PLAYER_SIZE, PLAYER_SIZE);
	ALT_SEM_POST(display);

	int willCollide = 0;
	int x, y;
	char TestString[40];
//	Button btn = { 0, 0, 0, 0};
	while(1)
	{
		waitForVSync(buffer_register, dma_control);
		OSTimeDly(PLAYER_SPEED);

		if(p1y == 1)
		{
			y = player1.y+PLAYER_SIZE+2;
			for (x = player1.x-1; x < player1.x + PLAYER_SIZE+2; x++)
			{
				switch(level.map[x][y])
				{
				case WALL:
//				case WALL_CRATE:
				case WALL_INVIS:
					willCollide = 1;
					break;
				case BUTTON:
					button_ptr = getButton(button_ptr, x, y);
					willCollide = 0;
					break;
				default:
					willCollide = 0;
					break;
				}
				if (willCollide)
					break;
			}
			if (!willCollide)
				player1.y++;
			/*else	// Draw player collision
			{
				ALT_SEM_PEND(display, 0);
				drawRect(pixel_ctrl_ptr, 0xF000, player1.x - 1, player1.y + PLAYER_SIZE +2, PLAYER_SIZE+2, 0);
				ALT_SEM_POST(display);
			}*/
		}
		else if(p1y == 2)
		{
			y = player1.y-2;
			for (x = player1.x-1; x < player1.x + PLAYER_SIZE+2; x++)
			{
				switch(level.map[x][y])
				{
				case WALL:
//				case WALL_CRATE:
				case WALL_INVIS:
					willCollide = 1;
					break;
				case BUTTON:
					button_ptr = getButton(button_ptr, x, y);
					willCollide = 0;
					break;
				default:
					willCollide = 0;
					break;
				}
				if (willCollide)
					break;
			}
			if (!willCollide)
				player1.y--;
			/*else	// Draw player collision
			{
				ALT_SEM_PEND(display, 0);
				drawRect(pixel_ctrl_ptr, 0xF000, player1.x - 1, player1.y - 2, PLAYER_SIZE+2, 0);
				ALT_SEM_POST(display);
			}*/
		}
		if(p1x == 1)
		{
			x = player1.x+PLAYER_SIZE+2;
			for (y = player1.y-1; y < player1.y + PLAYER_SIZE+2; y++)
			{
				switch(level.map[x][y])
				{
				case WALL:
//				case WALL_CRATE:
				case WALL_INVIS:
					willCollide = 1;
					break;
				case BUTTON:
					button_ptr = getButton(button_ptr, x, y);
					willCollide = 0;
					break;
				default:
					willCollide = 0;
					break;
				}
				if (willCollide)
					break;
			}
			if (!willCollide)
				player1.x++;
			/*else	// Draw player collision
			{
				ALT_SEM_PEND(display, 0);
				drawRect(pixel_ctrl_ptr, 0xF000, player1.x + PLAYER_SIZE + 2, player1.y-1, 0, PLAYER_SIZE+2);
				ALT_SEM_POST(display);
			}*/
		}
		else if(p1x == 2)
		{
			x = player1.x-2;
			for (y = player1.y-1; y < player1.y + PLAYER_SIZE+2; y++)
			{
				switch(level.map[x][y])
				{
				case WALL:
//				case WALL_CRATE:
				case WALL_INVIS:
					willCollide = 1;
					break;
				case BUTTON:
					button_ptr = getButton(button_ptr, x, y);
					willCollide = 0;
					break;
				default:
					willCollide = 0;
					break;
				}
				if (willCollide)
					break;
			}
			if (!willCollide)
				player1.x--;
//			if (button_ptr->pressed == 1)
//			{
//				sprintf(TestString, "Door %i already open", button_ptr->door);
//				drawText(character_buffer, TestString, 0, 0);
//			}
			/*else	// Draw player collision
			{
				ALT_SEM_PEND(display, 0);
				drawRect(pixel_ctrl_ptr, 0xF000, player1.x - 2, player1.y-1, 0, PLAYER_SIZE+2);
				ALT_SEM_POST(display);
			}*/
		}

		// Draw player
		ALT_SEM_PEND(display, 0);
		drawRect(pixel_ctrl_ptr, BG_COLOR, player1.x-1, player1.y-1, PLAYER_SIZE+2, PLAYER_SIZE+2);
		drawRect(pixel_ctrl_ptr, PLAYER_COLOR, player1.x, player1.y, PLAYER_SIZE, PLAYER_SIZE);
		ALT_SEM_POST(display);

		ALT_SEM_POST(player);
	}
}
void task2(void* pdata)
{
	ALT_SEM_PEND(player, 0);

//	OSTimeDly(1);
	volatile short * pixel_ctrl_ptr = (volatile short *) pdata;

	// Draw initial playerbox
	ALT_SEM_PEND(display, 0);
	drawBox(pixel_ctrl_ptr, PLAYER_COLOR/2, player2.x, player2.y, PLAYER_SIZE, PLAYER_SIZE);
	ALT_SEM_POST(display);

	int willCollide = 0;
	int x, y;
//	Button btn;
	while(1)
	{
		waitForVSync(buffer_register, dma_control);
		OSTimeDly(PLAYER_SPEED);

		if(p2y == 1)
		{
			y = player2.y+PLAYER_SIZE+2;
			for (x = player2.x-1; x < player2.x + PLAYER_SIZE+2; x++)
			{
				switch(level.map[x][y])
				{
				case WALL:
				case WALL_CRATE:
				case WALL_INVIS:
					willCollide = 1;
					break;
				case BUTTON:
					button_ptr = getButton(button_ptr, x, y);
					break;
				default:
					willCollide = 0;
					break;
				}
				if (willCollide)
					break;
			}
			if (!willCollide)
				player2.y++;
			else	// Draw player collision
			{
				ALT_SEM_PEND(display, 0);
				drawRect(pixel_ctrl_ptr, 0xF000, player2.x - 1, player2.y + PLAYER_SIZE +2, PLAYER_SIZE+2, 0);
				ALT_SEM_POST(display);
			}
		}
		else if(p2y == 2)
		{
			y = player2.y-2;
			for (x = player2.x-1; x < player2.x + PLAYER_SIZE+2; x++)
			{
				switch(level.map[x][y])
				{
				case WALL:
				case WALL_CRATE:
				case WALL_INVIS:
					willCollide = 1;
					break;
				case BUTTON:
					button_ptr = getButton(button_ptr, x, y);
					break;
				default:
					willCollide = 0;
					break;
				}
				if (willCollide)
					break;
			}
			if (!willCollide)
				player2.y--;
			else	// Draw player collision
			{
				ALT_SEM_PEND(display, 0);
				drawRect(pixel_ctrl_ptr, 0xF000, player2.x - 1, player2.y - 2, PLAYER_SIZE+2, 0);
				ALT_SEM_POST(display);
			}
		}
		if(p2x == 1)
		{
			x = player2.x+PLAYER_SIZE+2;
			for (y = player2.y-1; y < player2.y + PLAYER_SIZE+2; y++)
			{
				switch(level.map[x][y])
				{
				case WALL:
				case WALL_CRATE:
				case WALL_INVIS:
					willCollide = 1;
					break;
				case BUTTON:
					button_ptr = getButton(button_ptr, x, y);
					break;
				default:
					willCollide = 0;
					break;
				}
				if (willCollide)
					break;
			}
			if (!willCollide)
				player2.x++;
			else	// Draw player collision
			{
				ALT_SEM_PEND(display, 0);
				drawRect(pixel_ctrl_ptr, 0xF000, player2.x + PLAYER_SIZE + 2, player2.y-1, 0, PLAYER_SIZE+2);
				ALT_SEM_POST(display);
			}
		}
		else if(p2x == 2)
		{
			x = player2.x-2;
			for (y = player2.y-1; y < player2.y + PLAYER_SIZE+2; y++)
			{
				switch(level.map[x][y])
				{
				case WALL:
				case WALL_CRATE:
				case WALL_INVIS:
					willCollide = 1;
					break;
				case BUTTON:
					button_ptr = getButton(button_ptr, x, y);
					break;
				default:
					willCollide = 0;
					break;
				}
				if (willCollide)
					break;
			}
			if (!willCollide)
				player2.x--;
			else	// Draw player collision
			{
				ALT_SEM_PEND(display, 0);
				drawRect(pixel_ctrl_ptr, 0xF000, player2.x - 2, player2.y-1, 0, PLAYER_SIZE+2);
				ALT_SEM_POST(display);
			}
		}

		// Draw player
		ALT_SEM_PEND(display, 0);
		drawRect(pixel_ctrl_ptr, BG_COLOR, player2.x-1, player2.y-1, PLAYER_SIZE+2, PLAYER_SIZE+2);
		drawRect(pixel_ctrl_ptr, PLAYER_COLOR/2, player2.x, player2.y, PLAYER_SIZE, PLAYER_SIZE);
		ALT_SEM_POST(display);

		ALT_SEM_POST(player);
	}
}
void getcontrols(void* pdata)
{
	int data, i;
	char cmd[3];

	for (i = 0; i < sizeof(cmd)/sizeof(cmd[0]); i++)
	{
		cmd[i] = 0;
	}
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
				ALT_SEM_PEND(player,0);


				switch(cmd[1])
				{
				case 'g':
					if(cmd[2] != 'r')
					{
						if (cmd[0] == '1')
							p1y = 1;
						else
							p2y = 1;
//						p1y = 1;
					}
					else
					{
						if (cmd[0] == '1')
							p1y = 0;
						else
							p2y = 0;
//						p1y = 0;
					}
					break;
				case 'h':
					if(cmd[2] != 'r')
					{
						if (cmd[0] == '1')
							p1y = 2;
						else
							p2y = 2;
//						p1y = 2;
					}
					else
					{
						if (cmd[0] == '1')
							p1y = 0;
						else
							p2y = 0;
//						p1y = 0;
					}
					break;
				case 'l':
					if(cmd[2] != 'r')
					{
						if (cmd[0] == '1')
							p1x = 2;
						else
							p2x = 2;
//						p1x = 2;
					}
					else
					{
						if (cmd[0] == '1')
							p1x = 0;
						else
							p2x = 0;
//						p1x = 0;
					}
					break;
				case 'r':
					if(cmd[2] != 'r')
					{
						if (cmd[0] == '1')
							p1x = 1;
						else
							p2x = 1;
//						p1x = 1;
					}
					else
					{
						if (cmd[0] == '1')
							p1x = 0;
						else
							p2x = 0;
//						p1x = 0;
					}
					break;
				default:

					break;


				}

				for (i = 0; i < sizeof(cmd)/sizeof(cmd[0]); i++)
				{
					cmd[i] = 0;
				}
				i = 0;

			}
			ALT_SEM_POST(player);

		}
		OSTimeDly(1);
	}
}
void draw_level(void* pdata)
{
//	OSTimeDly(1);
	volatile short * pixel_ctrl_ptr = (volatile short *) pdata;

	int toggle = 1;
	alt_u16 c;
	while(1)
	{
		waitForVSync(buffer_register, dma_control);
		OSTimeDly(100);

		// Draw level
		ALT_SEM_PEND(display, 0);

		// Middle wall
		drawCollisionRect(pixel_ctrl_ptr, level_ptr, WALL, WALL_COLOR, 1,
				SCREEN_WIDTH/2 - SPLITSCREEN_WIDTH/2,							// x1
				0,																// y1
				SPLITSCREEN_WIDTH,												// width
				SCREEN_HEIGHT/2 - DOOR_SIZE-1);									// height
		drawCollisionRect(pixel_ctrl_ptr, level_ptr, WALL, WALL_COLOR, 1,
				SCREEN_WIDTH/2 - SPLITSCREEN_WIDTH/2,
				SCREEN_HEIGHT/2 + DOOR_SIZE,
				SPLITSCREEN_WIDTH,
				SCREEN_HEIGHT/2 - DOOR_SIZE-1);

		// Middle crate wall
		drawCollisionRect(pixel_ctrl_ptr, level_ptr, WALL_CRATE, WALL_CRATE_COLOR, 1,
				SCREEN_WIDTH/2 - SPLITSCREEN_WIDTH/2,
				SCREEN_HEIGHT/2 - DOOR_SIZE,
				SPLITSCREEN_WIDTH,
				DOOR_SIZE-1);
		drawCollisionRect(pixel_ctrl_ptr, level_ptr, WALL_CRATE, WALL_CRATE_COLOR, 1,
				SCREEN_WIDTH/2 - SPLITSCREEN_WIDTH/2,
				SCREEN_HEIGHT/2,
				SPLITSCREEN_WIDTH,
				DOOR_SIZE-1);

		drawCollisionRect(pixel_ctrl_ptr, level_ptr, WALL, WALL_COLOR, 1,
				0,
				SCREEN_HEIGHT-1-20-WALL_WIDTH,
				SCREEN_WIDTH/2 - SPLITSCREEN_WIDTH/2 -1 -DOOR_SIZE,
				WALL_WIDTH);

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
		drawButton(pixel_ctrl_ptr, level_ptr, 0, BUTTON, c, 50, 50);
		drawButton(pixel_ctrl_ptr, level_ptr, 1, BUTTON, c, 100, 100);
		// Outer bounds
		drawCollisionRect(pixel_ctrl_ptr, level_ptr, WALL_INVIS, WALL_INVIS, 0,
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
	volatile short* pixel_ctrl_ptr = pixel_buffer_start;
	volatile char* char_ctrl_ptr = character_buffer;

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

//	clearscreen(pixel_ctrl_ptr);
	fillScreen(pixel_ctrl_ptr, 0x00F0);

	waitForVSync(buffer_register, dma_control);

	OSTaskCreateExt(draw_level,
			pixel_ctrl_ptr,
			(void *)&draw_level_stk[TASK_STACKSIZE-1],
			DRAW_LEVEL_PRIORITY,
			DRAW_LEVEL_PRIORITY,
			draw_level_stk,
			TASK_STACKSIZE,
			NULL,
			0);

	OSTaskCreateExt(task1,
			pixel_ctrl_ptr,
			(void *)&task1_stk[TASK_STACKSIZE-1],
			TASK1_PRIORITY,
			TASK1_PRIORITY,
			task1_stk,
			TASK_STACKSIZE,
			NULL,
			0);


	OSTaskCreateExt(task2,
			pixel_ctrl_ptr,
			(void *)&task2_stk[TASK_STACKSIZE-1],
			TASK2_PRIORITY,
			TASK2_PRIORITY,
			task2_stk,
			TASK_STACKSIZE,
			NULL,
			0);

	OSTaskCreateExt(getcontrols,
			char_ctrl_ptr,
			(void *)&getcontrols_stk[TASK_STACKSIZE-1],
			GETCONTROLS_PRIORITY,
			GETCONTROLS_PRIORITY,
			getcontrols_stk,
			TASK_STACKSIZE,
			NULL,
			0);

	OSStart();
	return 0;
}

