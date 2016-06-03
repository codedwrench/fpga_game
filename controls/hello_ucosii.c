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


Door doors[MAX_DOORS];
Button buttons[MAX_DOORS];
Crate crates[MAX_CRATES];
Player players[MAX_PLAYERS];


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

void movePlayer(alt_u8 pNum, alt_u8 dir)
{
	alt_u8 err;
	alt_u8 willCollide = 0;
	alt_8 btnPressed = -1;
	alt_u16 x, y, p, p0, p1;
	alt_u16 color;
	alt_u16 pX, pY;
	alt_u8 i =0;

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
		y = players[pNum].y -2;
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
		if (*(pixel_buffer + (y << 9) + x) == -1 || *(pixel_buffer + (y << 9) + x) ==4095)
		{
			willCollide = 1;
			break;
		}
		else if (*(pixel_buffer + (y << 9) + x) == 3840)
		{
			willCollide = 0;
			for(i = 0;i<MAX_BUTTONS;i++)
			{
				if(((buttons[i].coords[0]*4)<=x+1 && (buttons[i].coords[0]*4)>= x-BUTTON_SIZE-1) && ((buttons[i].coords[1]*4)<=y) && ((buttons[i].coords[1]*4)>=y-BUTTON_SIZE)) //if the player is on the button
				{
					ALT_SEM_PEND(display,0);
					drawRect(pixel_buffer,BG_COLOR,buttons[i].coords[0]*4,buttons[i].coords[1]*4,BUTTON_SIZE,BUTTON_SIZE);
					if(doors[i].vert)
						drawBox(pixel_buffer,BG_COLOR,doors[i].coords[0]*4,(doors[i].coords[1]*4)+1,WALL_SIZE,DOOR_SIZE-2);
					else
						drawBox(pixel_buffer,BG_COLOR,doors[i].coords[0]*4,doors[i].coords[1]*4,DOOR_SIZE,WALL_SIZE);
					ALT_SEM_POST(display);
					break;
				}
			}
			break;
		}

//		*(pixel_buffer + (y << 9) + x) = 0xF000;
		if (willCollide || btnPressed >= 0)
			break;
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
}

void PlayerTask(void* pdata)
{
	ALT_SEM_PEND(player, 0);

	alt_u8 pNum = (alt_u8*) pdata;

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
		}
		else if(players[pNum].yDir == UP)
		{
			movePlayer(pNum, UP);
		}
		if(players[pNum].xDir == RIGHT)
		{
			movePlayer(pNum, RIGHT);
		}
		else if(players[pNum].xDir == LEFT)
		{
			movePlayer(pNum, LEFT);
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
void InitLevelTask(void* pdata)
{
	alt_u8 err;
	alt_sem_pend(level_sem,0);
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
		int doortrig[4] = {999,999,999,999};
		alt_u8 vert;
		while(pix != EOF)
		{
			if(pix == '1')
			{
				drawBox(pixel_buffer,WALL_COLOR,count*4,county*4,4,4);
			}
			else if(pix == '\n')
			{
				county++;
				count =-1;
			}
			else if(pix == '!' && vert == 0)
			{
				doortrig[0] = count;
				doortrig[1] = county;
			}
			else if(pix == '#')
			{
				drawRect(pixel_buffer,WALL_CRATE_COLOR,count*4,county*4+1,WALL_SIZE,DOOR_SIZE-2);
			}
			else if(pix == '+')
			{
				drawBox(pixel_buffer,CRATE_COLOR,count*4,county*4,PLAYER_SIZE,PLAYER_SIZE);
			}


			if(count -1 == doortrig[0] && county == doortrig[1] && pix != ' ')
			{
				drawBox(pixel_buffer,DOOR_COLOR,(count-1)*4,county*4,DOOR_SIZE,WALL_SIZE);
				doors[pix-50].coords[0] = count -1;
				doors[pix-50].coords[1] = county;
				doors[pix-50].vert = 0;
			}
			else if(count-1 == doortrig[0] && county == doortrig[1] && pix == ' ' )
			{
				vert = 1;
			}
			else if(vert == 1 && count == doortrig[0] && county-1 == doortrig[1] && pix != ' ')
			{
				drawBox(pixel_buffer,DOOR_COLOR,(count)*4,(county*4)-3,WALL_SIZE,DOOR_SIZE);
				doors[pix-50].coords[0] = count;
				doors[pix-50].coords[1] = county -1;
				doors[pix-50].vert = 1;
				vert = 0;
			}
			else if(pix > '1')
			{
				drawRect(pixel_buffer,BUTTON_COLOR,count*4,county*4,BUTTON_SIZE,BUTTON_SIZE);
				buttons[pix-50].coords[0] = count;
				buttons[pix-50].coords[1] = county;

			}


			pix =  alt_up_sd_card_read(file);
			count++;
		}
		ALT_SEM_POST(level_sem);
		OSTaskDel(OS_PRIO_SELF);
}




int main (void){
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
	Player p1 = { SCREEN_WIDTH/4, SCREEN_HEIGHT-PLAYER_SIZE-PLAYER_SIZE/2-2, "TestGuy1\0", NONE };
	Player p2 = { SCREEN_WIDTH/4*3, SCREEN_HEIGHT-PLAYER_SIZE-PLAYER_SIZE/2-2, NONE, "TestGuy2\0", NONE };
	players[0] = p1;
	players[1] = p2;


	//Call find_files on the root directory
	alt_u8 pNum;
	OSTaskCreateExt(InitLevelTask,
			0,
			(void *)&InitLevelTask_STK[TASK_STACKSIZE-1],
			INITLEVEL_PRIORITY,
			INITLEVEL_PRIORITY,
			InitLevelTask_STK,
			TASK_STACKSIZE,
			NULL,
			0);
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
