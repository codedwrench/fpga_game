#include "main.h"

//Pointers to hardware
volatile short * pixel_buffer 	    = (short*)PIXEL_BUFFER
volatile char * character_buffer	= (char*)CHARACTER_BUFFER;
volatile short * buffer_register	= (short*)BUFFER_REGISTER;
volatile short * dma_control		= (short*)DMA_CONTROL;
volatile int * JTAG_UART_ptr 	= (int*)	JTAG_UART;		// JTAG UART address

//Stack Definitions
OS_STK PlayerTask_STK[MAX_PLAYERS][TASK_STACKSIZE];
OS_STK ControlsTask_STK[TASK_STACKSIZE];
OS_STK TimerTask_STK[TASK_STACKSIZE];
OS_STK DrawTimerTask_STK[TASK_STACKSIZE];
OS_STK InitLevelTask_STK[TASK_STACKSIZE];
OS_STK FinishTask_STK[TASK_STACKSIZE];
OS_STK MainMenuTask_STK[TASK_STACKSIZE];

//Semaphores and Flags
ALT_SEM(display)
ALT_SEM(audio)
ALT_SEM(player)
ALT_SEM(button)
ALT_SEM(level_sem)
ALT_SEM(timer)
ALT_FLAG_GRP(finish_flag)

//In-Game objects
Door doors[MAX_DOORS];
Button buttons[MAX_DOORS];
Crate crates[MAX_CRATES];
Player players[MAX_PLAYERS];

//Timer
alt_u8 sec, min;

alt_u8 getArea(alt_u16 x, alt_u8 y,alt_u8 w,alt_u8 h, alt_u16 collisionX,alt_u8 collisionY)
{
	if(collisionX <= x+1 && collisionX >= x-w && collisionY <= y && collisionY >= y-h)
	{
		return 1;
	}
	return 0;
}
alt_u8 checkWallAndDoor(alt_u16 x,alt_u8 y)
{
	if(getPixel(pixel_buffer,x,y) == WALL_COLOR ||getPixel(pixel_buffer,x,y) == DOOR_COLOR)
		return 1;
	return 0;
}
alt_u8 moveCrate(alt_u16 x,alt_u8 y,alt_u16 *crateX, alt_u16 *crateY,alt_u8 dir)
{
	if(*crateX == x && dir == RIGHT && !checkWallAndDoor(*crateX + PLAYER_SIZE+1,*crateY))//player is left of crate
	{
		if(getPixel(pixel_buffer,*crateX+PLAYER_SIZE+1,*crateY) == WALL_CRATE_COLOR) //teleport the box if it hits a grate
		{
			fillRect(pixel_buffer,BG_COLOR,*crateX,*crateY,PLAYER_SIZE,PLAYER_SIZE); //remove the crate from te old position
			*crateX += 17;
			fillRect(pixel_buffer,CRATE_COLOR,*crateX,*crateY,PLAYER_SIZE,PLAYER_SIZE); //redraw it at the new position
		}
		else //otherwise just draw it one pixel further
		{
			fillRect(pixel_buffer,BG_COLOR,*crateX,*crateY,PLAYER_SIZE,PLAYER_SIZE);
			fillRect(pixel_buffer,CRATE_COLOR,*crateX+1,*crateY,PLAYER_SIZE,PLAYER_SIZE);
			*crateX += 1;
		}
	}
	else if(*crateX+PLAYER_SIZE==x && dir == LEFT && !checkWallAndDoor(*crateX-1,*crateY))//player is right of crate
	{
		if(getPixel(pixel_buffer,*crateX-1,*crateY) ==  WALL_CRATE_COLOR) //same as before but the other way around
		{
			fillRect(pixel_buffer,BG_COLOR,*crateX,*crateY,PLAYER_SIZE,PLAYER_SIZE);
			*crateX -= 17;
			fillRect(pixel_buffer,CRATE_COLOR,*crateX,*crateY,PLAYER_SIZE,PLAYER_SIZE);
		}
		else
		{
			fillRect(pixel_buffer,BG_COLOR,*crateX,*crateY,PLAYER_SIZE,PLAYER_SIZE);
			fillRect(pixel_buffer,CRATE_COLOR,*crateX-1,*crateY,PLAYER_SIZE,PLAYER_SIZE);
			*crateX -= 1;
		}
	}
	else if(*crateY == y && dir == DOWN && !checkWallAndDoor(*crateX,*crateY + PLAYER_SIZE + 1))//player is above crate
	{
		fillRect(pixel_buffer,BG_COLOR,*crateX,*crateY,PLAYER_SIZE,PLAYER_SIZE); //remove the old crate
		fillRect(pixel_buffer,CRATE_COLOR,*crateX,*crateY+1,PLAYER_SIZE,PLAYER_SIZE); //place new crate 1 lower
		*crateY+= 1;
	}
	else if(*crateY + PLAYER_SIZE == y && dir == UP && !checkWallAndDoor(*crateX,*crateY-1))//player is below crate
	{
		fillRect(pixel_buffer,BG_COLOR,*crateX,*crateY,PLAYER_SIZE,PLAYER_SIZE); //remove the old crate
		fillRect(pixel_buffer,CRATE_COLOR,*crateX,*crateY-1,PLAYER_SIZE,PLAYER_SIZE);
		*crateY -= 1;
	}
	else
	{
		return 1;
	}
	return 0;

}
void createDoor(alt_u8 doornumber)
{
	alt_u16 doorX;
	alt_u8 doorY;
	doorX = doors[doornumber].coords[0]*4;
	doorY = doors[doornumber].coords[1]*4;
	if(doors[doornumber].vert) //Draw door vertically
		fillRect(pixel_buffer,BG_COLOR,doorX,doorY+1,WALL_SIZE,DOOR_SIZE-2);
	else //Draw door horizontally
		fillRect(pixel_buffer,BG_COLOR,doorX+1,doorY,DOOR_SIZE,WALL_SIZE);
}
void handleCollisions(alt_u16 x, alt_u8 y,alt_u8 dir, alt_u8 pNum, alt_u8 *willCollide)
{
	alt_u8 i;
	alt_u16 buttonX;
	alt_u8 buttonY;
	alt_u16 *crateX;
	alt_u16 *crateY;
	alt_u16 collisionColor= getPixel(pixel_buffer,x,y);
	if (checkWallAndDoor(x,y)||collisionColor == WALL_CRATE_COLOR) //player bumped against a wall, door or grate
	{
		*willCollide = 1;
	}
	else if (collisionColor == BUTTON_COLOR) //player has stepped on a button
	{
		*willCollide = 0; //player does not hit the button
		for(i = 0;i<MAX_BUTTONS;i++) //check for all buttons if the player stepped on it
		{
			buttonX = buttons[i].coords[0]*4; //readability
			buttonY = buttons[i].coords[1]*4;
			if(getArea(x,y,BUTTON_SIZE,BUTTON_SIZE,buttonX,buttonY)) //if the player is on the button
			{
				drawRect(pixel_buffer,BG_COLOR,buttonX,buttonY,BUTTON_SIZE,BUTTON_SIZE); //Remove Button
				createDoor(i);
				break;
			}
		}
	}
	else if (getPixel(pixel_buffer,x,y) == CRATE_COLOR)
	{
		for(i =0;i<MAX_CRATES;i++)
		{
			crateX = &crates[i].coords[0];
			crateY = &crates[i].coords[1];
			*willCollide = moveCrate(x,y,crateX,crateY,dir);
			alt_u8 cnt;
			for(cnt = 0; cnt < MAX_BUTTONS;cnt++)
			{
				buttonX = buttons[cnt].coords[0]*4;
				buttonY = buttons[cnt].coords[1]*4;
				//if(*crateX >= buttonX - BUTTON_SIZE && *crateX <= *buttonY + 4 && *crateY >= *buttonY-4 && *crateY <= *buttonY+4)
				if(getArea(*crateX,*crateY,BUTTON_SIZE,BUTTON_SIZE,buttonX,buttonY))
				{
					buttons[cnt].coords[0]= 0; //i can't change this through the variable, as it is not a pointer
					fillRect(pixel_buffer,CRATE_COLOR,*crateX,*crateY,PLAYER_SIZE,PLAYER_SIZE); //draw the crate first, so it won't get behind stuff
					createDoor(cnt);
				}
			}
		}
	}
	else if(collisionColor == SPIKE_COLOR)
	{
		addPenalty(2,timer);
	}
	else if (collisionColor == 0)
	{
		*willCollide = 1; //if the player hits the timer, set the player's finish flag and collide
		if (pNum == 0)
			ALT_FLAG_POST(finish_flag, FINISH_1, OS_FLAG_SET);
		else if (pNum == 1)
			ALT_FLAG_POST(finish_flag, FINISH_2, OS_FLAG_SET);

	}

}
void movePlayer(alt_u8 pNum, alt_u8 dir)
{
	alt_u8 willCollide = 0;
	alt_8 btnPressed = -1;
	alt_u16 x, y, p, p0, p1;
	p0 = 0; //make sure p0 is not used unitialized
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
	for (p = p0; p < p1; p++) //Do this for every X or Y value , depending on the direction
	{
		if (dir == UP || dir == DOWN) //Put p in X or Y depending on the direction
			x = p;
		else if (dir == LEFT || dir == RIGHT)
			y = p;

		handleCollisions(x,y,dir,pNum,&willCollide);

		for(i = 0;i<MAX_CRATES;i++) //redraw crates and button if player moves
		{
			fillRect(pixel_buffer,CRATE_COLOR,crates[i].coords[0],crates[i].coords[1],PLAYER_SIZE,PLAYER_SIZE);
		}
		for(i = 20;i<MAX_BUTTONS;i++)
		{
			if(buttons[i].coords[0] != 0)
			{
				drawRect(pixel_buffer,BUTTON_CRATE_COLOR,buttons[i].coords[0]*4,buttons[i].coords[1]*4,BUTTON_SIZE,BUTTON_SIZE);
			}
		}
		if (willCollide || btnPressed >= 0)
			break;
	}
	if (!willCollide) //actually move the player
	{
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
}
short int openSDFile(alt_up_sd_card_dev* sd_card, char name[])
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
void DrawTimerTask(void *pdata)
{
	char timerStr[6];
	// Clear char buffer
	sprintf(timerStr, "         ");
	drawText(character_buffer, timerStr, 36, 2);

	// Draw timer background
	ALT_SEM_PEND(display, 0);
	fillRect(pixel_buffer, 0, 145, 5, 30, 8);
	ALT_SEM_POST(display);

	while (1)
	{
		ALT_SEM_PEND(timer, 0);
		sprintf(timerStr, "%.2d:%.2d", min, sec);
		drawText(character_buffer, timerStr, 37, 2);
		ALT_SEM_POST(timer);
		OSTimeDly(100);
	}
}
void TimerTask(void *pdata)
{
	min = sec = 0;
	while (1)
	{
		ALT_SEM_PEND(timer, 0);
		sec++;
		if (sec > 59)
		{
			sec = 0;
			min = (min + 1) % 60;
		}
		ALT_SEM_POST(timer);
		OSTimeDlyHMSM(0, 0, 1, 0);
	}
}
void PlayerTask(void* pdata)
{
	//	ALT_SEM_PEND(player, 0);

	alt_u8 pNum = (alt_u8*) pdata;

	// Draw initial playerbox
	ALT_SEM_PEND(display, 0);
	fillRect(pixel_buffer, PLAYER_COLOR/(pNum+1), players[pNum].x, players[pNum].y, PLAYER_SIZE, PLAYER_SIZE);
	ALT_SEM_POST(display);

	//	ALT_SEM_POST(player);
	while(1)
	{
		waitForVSync(buffer_register, dma_control);
		//		OSTimeDly(PLAYER_SPEED);
		//		OSTimeDly(100);
		OSTimeDly(20);

		//		ALT_SEM_PEND(player, 0);
		if(players[pNum].yDir == DOWN)
		{
			movePlayer(pNum, DOWN);
		}
		else if(players[pNum].yDir == UP)
		{
			movePlayer(pNum, UP);
			//			OSTimeDly(1);
		}
		if(players[pNum].xDir == RIGHT)
		{
			movePlayer(pNum, RIGHT);
			//			OSTimeDly(1);
		}
		else if(players[pNum].xDir == LEFT)
		{
			movePlayer(pNum, LEFT);
			//			OSTimeDly(1);
		}


		// Draw player
		ALT_SEM_PEND(display, 0);
		drawRect(pixel_buffer, BG_COLOR, players[pNum].x-1, players[pNum].y-1, PLAYER_SIZE+2, PLAYER_SIZE+2);
		fillRect(pixel_buffer, PLAYER_COLOR/(pNum+1), players[pNum].x, players[pNum].y, PLAYER_SIZE, PLAYER_SIZE);
		ALT_SEM_POST(display);

		//		ALT_SEM_POST(player);
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
				//				ALT_SEM_PEND(player, 0);
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
				case '1':
				case '2':
				case '3':
				case '4':
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
			//			ALT_SEM_POST(player);

		}
		OSTimeDly(1);
	}
}
void InitLevelTask(void* pdata)
{
	alt_sem_pend(level_sem,0);
	alt_up_sd_card_dev * sd_card;
	sd_card = alt_up_sd_card_open_dev("/dev/SD_Card");
	short int file = openSDFile(sd_card, "level.txt");
	short int scoresFile = openSDFile(sd_card, "scores.txt");
	scoresFile_ptr = &scoresFile;

	if(file == -1)
	{
		printf("Something unexpected happened\n");
	}
	if (scoresFile == -1)
	{
		printf("Can't open scores file\n");
		scoresFile = alt_up_sd_card_fopen("scores.txt", 1);
		scoresFile_ptr = &scoresFile;
		setInitScore(scoresFile_ptr);
	}

	loadScores(scoresFile);

	fillScreen(pixel_buffer, 0x00F0);
	waitForVSync(buffer_register, dma_control);
	char pix =  alt_up_sd_card_read(file);
	int count = 0;
	int county = 0;
	int countcrate = 0;
	int doortrig[4] = {999,999,999,999};
	alt_u8 vert = 0;
	alt_u8 spikes= 0;
	while(pix != EOF)
	{
		if(pix == '1')
		{
			fillRect(pixel_buffer,WALL_COLOR,count*4,county*4,4,4);
		}
		else if(pix == '\n')
		{
			county++;
			count =-1;
		}
		else if((pix == '!'||pix == '"') && vert == 0)
		{
			doortrig[0] = count;
			doortrig[1] = county;
			if(pix == '"')
			{
				spikes = 1;
			}
			else
			{
				spikes = 0;
			}
		}
		else if(pix == '#')
		{
			drawRect(pixel_buffer,WALL_CRATE_COLOR,count*4,county*4+1,WALL_SIZE,DOOR_SIZE-2);
		}
		else if(pix == '+')
		{
			fillRect(pixel_buffer,CRATE_COLOR,count*4,county*4,PLAYER_SIZE,PLAYER_SIZE);
			crates[countcrate].coords[0] = count*4;
			crates[countcrate].coords[1] = county*4;
		}


		if(count -1 == doortrig[0] && county == doortrig[1] && pix != ' ')
		{
			doors[pix-50].coords[0] = count -1;
			doors[pix-50].coords[1] = county;
			doors[pix-50].vert = 0;
			doors[pix-50].spikes = spikes;
			alt_8 i = 0;
			if(doors[pix-50].spikes == 1)
			{
				for(i=0;i<8;i++)
				{
					drawLine(pixel_buffer,SPIKE_COLOR,(count)*4+i*2-1,(county*4),(count)*4+i*2-1,(county*4)+4);
				}
			}
			else
			{
				fillRect(pixel_buffer,DOOR_COLOR,(count-1)*4+1,county*4,DOOR_SIZE,WALL_SIZE);
			}
		}
		else if(count-1 == doortrig[0] && county == doortrig[1] && pix == ' ' )
		{
			vert = 1;
		}
		else if(vert == 1 && count == doortrig[0] && county-1 == doortrig[1] && pix != ' ')
		{
			fillRect(pixel_buffer,DOOR_COLOR,(count)*4,(county*4)-3,WALL_SIZE,DOOR_SIZE);
			doors[pix-50].coords[0] = count;
			doors[pix-50].coords[1] = county -1;
			doors[pix-50].vert = 1;
			vert = 0;
		}
		else if(pix > '1' && pix < 'F')
		{
			drawRect(pixel_buffer,BUTTON_COLOR,count*4,county*4,BUTTON_SIZE,BUTTON_SIZE);
			buttons[pix-50].coords[0] = count;
			buttons[pix-50].coords[1] = county;

		}
		else if(pix >= 'F')
		{
			drawRect(pixel_buffer,BUTTON_CRATE_COLOR,count*4,county*4,BUTTON_SIZE,BUTTON_SIZE);
			buttons[pix-50].coords[0] = count;
			buttons[pix-50].coords[1] = county;
		}


		pix =  alt_up_sd_card_read(file);
		count++;
	}
	ALT_SEM_POST(level_sem);
	OSTaskDel(OS_PRIO_SELF);
}
void MainMenuTask(void* pdata)
{
	OSTimeDly(100);
	alt_u8 i;
	fillScreen(pixel_buffer, 0x00F0);

	// Init players
	Player p1 = { SCREEN_WIDTH/4, SCREEN_HEIGHT-PLAYER_SIZE-PLAYER_SIZE/2-2, NONE, NONE, NONE };
	Player p2 = { SCREEN_WIDTH/4*3, SCREEN_HEIGHT-PLAYER_SIZE-PLAYER_SIZE/2-2, NONE, NONE, NONE };
	players[0] = p1;
	players[1] = p2;

	for(i=2;i<5;i++)
	{
		drawText(character_buffer,"                ",0,i);
	}
	drawText(character_buffer,"      ",37,2);
	drawText(character_buffer, "                        ", 28, 22);
	drawText(character_buffer, "                          ", 27, 24);
	drawText(character_buffer, "                   ", 31, 36);
	drawText(character_buffer, "                  ", 31, 25);
	drawText(character_buffer, " ", 31, 31);
	drawText(character_buffer, " ", 39, 31);
	drawText(character_buffer, " ", 47, 31);
	drawText(character_buffer, "                       ", 29, 37);
	drawText(character_buffer, "          ", 35, 27);
	drawText(character_buffer, "          ", 42, 27);
	drawText(character_buffer, "          ", 35, 29);
	drawText(character_buffer, "          ", 42, 29);
	drawText(character_buffer, "          ", 35, 31);
	drawText(character_buffer, "          ", 42, 31);
	drawRect(pixel_buffer,0xFFFF,SCREEN_WIDTH/2-50,SCREEN_HEIGHT/2-12,100,20);
	drawText(character_buffer,"Press X to start playing",40-12,29);
	fillRect(pixel_buffer,0xAAAA,SCREEN_WIDTH/2-50,SCREEN_HEIGHT/4-12,100,20);
	drawText(character_buffer,"SPLITRUNNERS",40-6,14);
	drawText(character_buffer,"(c) Jerko Lenstra & Rick de Bondt - 2016",40-20,44);
	OSTaskCreateExt(ControlsTask,
			0,
			(void *)&ControlsTask_STK[TASK_STACKSIZE-1],
			CONTROLS_PRIORITY,
			CONTROLS_PRIORITY,
			ControlsTask_STK,
			TASK_STACKSIZE,
			NULL,
			0);
	alt_u8 pNum;
	while(players[0].action != 2)
	{
		OSTimeDly(1);

	}
	drawText(character_buffer,"                        ",40-12,29);
	drawText(character_buffer,"            ",40-6,14);
	drawText(character_buffer,"                                        ",40-20,44);
	OSTaskCreateExt(TimerTask,
			0,
			(void *)&TimerTask_STK[TASK_STACKSIZE-1],
			TIMER_PRIORITY,
			TIMER_PRIORITY,
			TimerTask_STK,
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
	OSTaskCreateExt(InitLevelTask,
			0,
			(void *)&InitLevelTask_STK[TASK_STACKSIZE-1],
			INITLEVEL_PRIORITY,
			INITLEVEL_PRIORITY,
			InitLevelTask_STK,
			TASK_STACKSIZE,
			NULL,
			0);
	OSTaskCreateExt(DrawTimerTask,
			0,
			(void *)&DrawTimerTask_STK[TASK_STACKSIZE-1],
			DRAWTIMER_PRIORITY,
			DRAWTIMER_PRIORITY,
			DrawTimerTask_STK,
			TASK_STACKSIZE,
			NULL,
			0);
	OSTaskCreateExt(FinishTask,
			0,
			(void *)&FinishTask_STK[TASK_STACKSIZE-1],
			FINISH_PRIORITY,
			FINISH_PRIORITY,
			FinishTask_STK,
			TASK_STACKSIZE,
			NULL,
			0);
	ALT_SEM_PEND(timer,0);
	min = 0;
	sec = 0;
	ALT_SEM_POST(timer);
	OSTaskDel(OS_PRIO_SELF);


}
void FinishTask(void *pdata)
{
	alt_u8 tempName[4] = { 0 };
	sprintf(tempName, "AAA");
	alt_u8 name[4] = { 0 };
	alt_u8 scoreText[22] = { 0 };
	alt_u8 cursorPos = 0;
	alt_u8 cursorDrawn = 0;
	alt_8 test = -1;

	drawText(character_buffer, highScores[0][0], 3, 2);
	drawText(character_buffer, highScores[0][1], 10, 2);
	drawText(character_buffer, highScores[1][0], 3, 3);
	drawText(character_buffer, highScores[1][1], 10, 3);
	drawText(character_buffer, highScores[2][0], 3, 4);
	drawText(character_buffer, highScores[2][1], 10, 4);

	while (1)
	{
		ALT_FLAG_PEND(finish_flag, FINISH_1 + FINISH_2, OS_FLAG_WAIT_SET_ALL, 0);
		ALT_SEM_PEND(timer, 0);

		OSTaskDel(PLAYER_PRIORITY);
		OSTaskDel(PLAYER_PRIORITY+1);

		fillRect(pixel_buffer, BG_COLOR + 1024, 100, 80, 120, 80);
		drawRect(pixel_buffer, 0, 99, 79, 122, 82);
		drawRect(pixel_buffer, 0xFFFF, 100, 80, 120, 80);

		test = checkScore();
		if (checkScore() >= 0)
		{
			sprintf(scoreText, "New highscore! %.2d:%.2d!", min, sec);
			drawText(character_buffer, scoreText, 29, 22);
			drawText(character_buffer, "Enter your teamname below", 27, 24);
			drawText(character_buffer, "by using the DPAD.", 31, 25);
			drawText(character_buffer, "Press X to submit.", 31, 36);
			drawText(character_buffer, "(The game will restart)", 29, 37);

			while (1)
			{
				OSTimeDly(1);
				sprintf(name, "%c", tempName[0]);
				drawText(character_buffer, name, 31, 31);
				sprintf(name, "%c", tempName[1]);
				drawText(character_buffer, name, 39, 31);
				sprintf(name, "%c", tempName[2]);
				drawText(character_buffer, name, 47, 31);

				if (players[0].xDir == LEFT && cursorPos > 0)
				{
					cursorPos--;
					players[0].xDir = NONE;
					cursorDrawn = 0;
				}
				else if (players[0].xDir == RIGHT && cursorPos < 2)
				{
					cursorPos++;
					players[0].xDir = NONE;
					cursorDrawn = 0;
				}
				if (players[0].yDir == UP)
				{
					if (tempName[cursorPos] >= 'Z')
						tempName[cursorPos] = 'A';
					else
						tempName[cursorPos]++;
					players[0].yDir = NONE;
				}
				else if (players[0].yDir == DOWN)
				{
					if (tempName[cursorPos] <= 'A')
						tempName[cursorPos] = 'Z';
					else
						tempName[cursorPos]--;
					players[0].yDir = NONE;
				}
				if (players[0].action == 2)
				{
					sprintf(name, tempName);
					setScore(checkScore(), name, scoresFile_ptr);

					players[0].action = -1;
					// Delete all tasks and restart

					alt_u8 i;
					for (i = INITLEVEL_PRIORITY; i < MAINMENU_PRIORITY; i++)
						if (i != FINISH_PRIORITY)
							OSTaskDel(i);
					OSTaskCreateExt(MainMenuTask,
							0,
							(void *)&MainMenuTask_STK[TASK_STACKSIZE-1],
							MAINMENU_PRIORITY,
							MAINMENU_PRIORITY,
							MainMenuTask_STK,
							TASK_STACKSIZE,
							NULL,
							0);
					OSTimeDlyHMSM(0, 0, 0, 500);
					ALT_SEM_POST(timer);
					ALT_FLAG_PEND(finish_flag, FINISH_1 + FINISH_2, OS_FLAG_WAIT_SET_ALL + OS_FLAG_CONSUME, 0);
					OSTaskDel(OS_PRIO_SELF);
				}
				if (!cursorDrawn)
				{
					fillRect(pixel_buffer, 0, 120, 116, 16, 16);
					fillRect(pixel_buffer, 0, 152, 116, 16, 16);
					fillRect(pixel_buffer, 0, 184, 116, 16, 16);
					drawRect(pixel_buffer, 0xFFFF, 120 + (32*cursorPos), 116, 16, 16);
					cursorDrawn = 1;
				}
			}
		}
		else
		{
			sprintf(scoreText, "No highscore, try again!", min, sec);
			drawText(character_buffer, scoreText, 28, 22);
			drawText(character_buffer, "These are the highscores:", 28, 24);
			drawText(character_buffer, highScores[0][0], 35, 27);
			drawText(character_buffer, highScores[0][1], 42, 27);
			drawText(character_buffer, highScores[1][0], 35, 29);
			drawText(character_buffer, highScores[1][1], 42, 29);
			drawText(character_buffer, highScores[2][0], 35, 31);
			drawText(character_buffer, highScores[2][1], 42, 31);
			drawText(character_buffer, "Press X to restart.", 31, 36);

			while (1)
			{
				if (players[0].action == 2)
				{
					players[0].action = -1;
					// Delete all tasks and restart

					alt_u8 i;
					for (i = INITLEVEL_PRIORITY; i < MAINMENU_PRIORITY; i++)
						if (i != FINISH_PRIORITY)
							OSTaskDel(i);
					OSTaskCreateExt(MainMenuTask,
							0,
							(void *)&MainMenuTask_STK[TASK_STACKSIZE-1],
							MAINMENU_PRIORITY,
							MAINMENU_PRIORITY,
							MainMenuTask_STK,
							TASK_STACKSIZE,
							NULL,
							0);
					OSTimeDlyHMSM(0, 0, 0, 500);
					ALT_SEM_POST(timer);
					ALT_FLAG_PEND(finish_flag, FINISH_1 + FINISH_2, OS_FLAG_WAIT_SET_ALL + OS_FLAG_CONSUME, 0);
					OSTaskDel(OS_PRIO_SELF);
				}
			}
		}
		OSTimeDly(50);
	}
}
int main (void)
{
	OSInit();

	int err = ALT_SEM_CREATE(&display, 1);
	if(err != 0)
		printf("Semaphore not created\n");
	err = ALT_SEM_CREATE(&audio, 1);
	if(err != 0)
		printf("Semaphore not created\n");
	err = ALT_SEM_CREATE(&player, 1);
	if(err != 0)
		printf("Semaphore not created\n");
	err = ALT_SEM_CREATE(&timer, 1);
	if(err != 0)
		printf("Semaphore not created\n");
	err = ALT_SEM_CREATE(&level_sem, 1);
	if(err != 0)
		printf("Semaphore not created\n");
	err = ALT_FLAG_CREATE(&finish_flag, 0x00);
	if (err != 0)
		printf("Flag not created\n");

	*(dma_control) &= (1<<2); //Enable DMA controller

	//Call find_files on the root directory
	OSTaskCreateExt(MainMenuTask,
			0,
			(void *)&MainMenuTask_STK[TASK_STACKSIZE-1],
			MAINMENU_PRIORITY,
			MAINMENU_PRIORITY,
			MainMenuTask_STK,
			TASK_STACKSIZE,
			NULL,
			0);

	OSStart();

	return 0;
}
