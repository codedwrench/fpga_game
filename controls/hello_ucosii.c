#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include "includes.h"
#include <alt_types.h>
#include <os/alt_sem.h>
#include "graphicslib.h"
#include "gamelib.h"
#include <Altera_UP_SD_Card_Avalon_Interface.h>
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
void movePlayer(alt_u8 pNum, alt_u8 dir)
{
	alt_u8 willCollide = 0;
	alt_8 btnPressed = -1;
	alt_u16 x, y, p, p0, p1;

	switch(dir)
	{
	case DOWN:
		p0 = players[pNum].x -1;
		p1 = players[pNum].x +PLAYER_SIZE +2;
		x = p0;
		y = players[pNum].y +PLAYER_SIZE +2;
		break;
	case UP:
		p0 = players[pNum].x -1;
		p1 = players[pNum].x +PLAYER_SIZE +2;
		x = p0;
		y = players[pNum].y +PLAYER_SIZE +2;
		break;
	case LEFT:
		p0 = players[pNum].y -1;
		p1 = players[pNum].y +PLAYER_SIZE +2;
		x = players[pNum].x -2;
		y = p0;
		break;
	case RIGHT:
		p0 = players[pNum].y -1;
		p1 = players[pNum].y +PLAYER_SIZE +2;
		x = players[pNum].x +PLAYER_SIZE +2;
		y = p0;
		break;
	default:
		break;
	}
	for (p = p0; p < p1; p++)
	{
		if (dir == UP || dir == DOWN)
			x = p;
		else if (dir == LEFT || dir == RIGHT)
			y = p;
//		switch(level.map[x][y])
//		{
//		case WALL:
//		case WALL_CRATE:
//		case WALL_INVIS:
//		case DOOR:
//			btnPressed = -1;
//			willCollide = 1;
//			break;
//		case BUTTON:
//			btnPressed = getButton(button_ptr, x, y);
//			willCollide = 0;
//			break;
//		default:
//			btnPressed = -1;
//			willCollide = 0;
//			break;
//		}
//		if (willCollide || btnPressed >= 0)
//			break;
	}
	if (!willCollide)
		switch(dir)
		{
		case UP:
			players[pNum].y--;
			break;
		case DOWN:
			players[pNum].y++;
			break;
		case LEFT:
			players[pNum].x--;
			break;
		case RIGHT:
			players[pNum].x++;
			break;
		default:
			break;
		}
	if (btnPressed >= 0)
	{
		level_ptr->doors[btnPressed].open = 1;
	}
}

void PlayerTask(void* pdata)
{
	ALT_SEM_PEND(player, 0);

	alt_u8 pNum = (alt_u8*) pdata;
	//	alt_u8 willCollide = 0;
	//	alt_8 btnPressed = -1;
	//	alt_u16 x, y;

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
			movePlayer(pNum, DOWN);
			//			y = players[pNum].y+PLAYER_SIZE+2;
			//			for (x = players[pNum].x-1; x < players[pNum].x + PLAYER_SIZE+2; x++)
			//			{
			//				switch(level.map[x][y])
			//				{
			//				case WALL:
			//				case WALL_CRATE:
			//				case WALL_INVIS:
			//				case DOOR:
			//					btnPressed = -1;
			//					willCollide = 1;
			//					break;
			//				case BUTTON:
			//					btnPressed = getButton(button_ptr, x, y);
			//					willCollide = 0;
			//					break;
			//				default:
			//					btnPressed = -1;
			//					willCollide = 0;
			//					break;
			//				}
			//				if (willCollide || btnPressed >= 0)
			//					break;
			//			}
			//			if (!willCollide)
			//				players[pNum].y++;
			//			if (btnPressed >= 0)
			//			{
			//				level_ptr->doors[btnPressed].open = 1;
			//			}
		}
		else if(players[pNum].yDir == UP)
		{
			movePlayer(pNum, UP);
			//			y = players[pNum].y-2;
			//			for (x = players[pNum].x-1; x < players[pNum].x + PLAYER_SIZE+2; x++)
			//			{
			//				switch(level.map[x][y])
			//				{
			//				case WALL:
			//				case WALL_CRATE:
			//				case WALL_INVIS:
			//				case DOOR:
			//					btnPressed = -1;
			//					willCollide = 1;
			//					break;
			//				case BUTTON:
			//					btnPressed = getButton(button_ptr, x, y);
			//					willCollide = 0;
			//					break;
			//				default:
			//					btnPressed = -1;
			//					willCollide = 0;
			//					break;
			//				}
			//				if (willCollide || btnPressed >= 0)
			//					break;
			//			}
			//			if (!willCollide)
			//				players[pNum].y--;
			//			if (btnPressed >= 0)
			//				level_ptr->doors[btnPressed].open = 1;
		}
		if(players[pNum].xDir == RIGHT)
		{
			movePlayer(pNum, RIGHT);
			//			x = players[pNum].x+PLAYER_SIZE+2;
			//			for (y = players[pNum].y-1; y < players[pNum].y + PLAYER_SIZE+2; y++)
			//			{
			//				switch(level.map[x][y])
			//				{
			//				case WALL:
			//				case WALL_CRATE:
			//				case WALL_INVIS:
			//				case DOOR:
			//					btnPressed = -1;
			//					willCollide = 1;
			//					break;
			//				case BUTTON:
			//					btnPressed = getButton(button_ptr, x, y);
			//					willCollide = 0;
			//					break;
			//				default:
			//					btnPressed = -1;
			//					willCollide = 0;
			//					break;
			//				}
			//				if (willCollide || btnPressed >= 0)
			//					break;
			//			}
			//			if (!willCollide)
			//				players[pNum].x++;
			//			if (btnPressed >= 0)
			//				level_ptr->doors[btnPressed].open = 1;
		}
		else if(players[pNum].xDir == LEFT)
		{
			movePlayer(pNum, LEFT);
			//			x = players[pNum].x-2;
			//			for (y = players[pNum].y-1; y < players[pNum].y + PLAYER_SIZE+2; y++)
			//			{
			//				switch(level.map[x][y])
			//				{
			//				case WALL:
			//				case WALL_CRATE:
			//				case WALL_INVIS:
			//				case DOOR:
			//					btnPressed = -1;
			//					willCollide = 1;
			//					break;
			//				case BUTTON:
			//					btnPressed = getButton(button_ptr, x, y);
			//					willCollide = 0;
			//					break;
			//				default:
			//					btnPressed = -1;
			//					willCollide = 0;
			//					break;
			//				}
			//				if (willCollide || btnPressed >= 0)
			//					break;
			//			}
			//			if (!willCollide)
			//				players[pNum].x--;
			//			if (btnPressed >= 0)
			//				level_ptr->doors[btnPressed].open = 1;
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

short int openSDFile(alt_up_sd_card_dev * sd_card,char name[])
{
	if (sd_card!=NULL){
		if (alt_up_sd_card_is_Present()){
			printf("An SD Card was found!\n");
		}
		else {
			printf("No SD Card Found. \n Exiting the program.");
			return -1;
		}

		if (alt_up_sd_card_is_FAT16()){
			printf("FAT-16 partiton found!\n");
		}
		else{
			printf("No FAT-16 partition found - Exiting!\n");
			return -1;
		}
	}
	return alt_up_sd_card_fopen(name,0);

}
int main (void){
	OSInit();
	level_ptr->map = malloc(sizeof(alt_u16)*4800);
	alt_up_sd_card_dev * sd_card;
	sd_card = alt_up_sd_card_open_dev("/dev/SD_Card");
	short int file = openSDFile(sd_card,"level.txt");
	if(file == -1)
	{
		printf("fucking kill me now\n");
	}
	fillScreen(pixel_buffer, 0x00F0);
	waitForVSync(buffer_register, dma_control);
	char pix =  alt_up_sd_card_read(file);
	int count = 0;
	int county= 0;
	while(pix != EOF)
	{
		if(pix == '1')
		{
			drawBox(pixel_buffer,0xFFFF,count*4,county*4,4,4);
			*(level_ptr->map+count*(county+1)) = WALL;
		}
		else if(pix == '\n')
		{
			county++;
			count =-1;
		}
		pix =  alt_up_sd_card_read(file);
		count++;
	}

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
	Player p1 = { SCREEN_WIDTH/4, SCREEN_HEIGHT-PLAYER_SIZE-PLAYER_SIZE/2-2, "TestGuy1\0", NONE };
	Player p2 = { SCREEN_WIDTH/4*3, SCREEN_HEIGHT-PLAYER_SIZE-PLAYER_SIZE/2-2, NONE, "TestGuy2\0", NONE };
	players[0] = p1;
	players[1] = p2;


	//Call find_files on the root directory
	alt_u8 pNum;

	OSTaskCreateExt(ControlsTask,
			0,
			(void *)&ControlsTask_STK[TASK_STACKSIZE-1],
			CONTROLS_PRIORITY,
			CONTROLS_PRIORITY,
			ControlsTask_STK,
			TASK_STACKSIZE,
			NULL,
			0);
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



	OSStart();

	return 0;
}
