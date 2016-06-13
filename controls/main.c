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
void InitLevelTask(void* pdata)
{
	alt_sem_pend(level_sem,0);
	alt_up_sd_card_dev * sd_card;
	sd_card = alt_up_sd_card_open_dev("/dev/SD_Card");
	short int file = openSDFile(sd_card, "level.txt");
	short int scoresFile = openSDFile(sd_card, "scores.txt");
	scoresFile_ptr = &scoresFile;
	//See if we could successfully open the files
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

	//clear the screen, wait until that is done and then start parsing the level
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
		if(pix == '1') //if we encountered a wall, make a wall
		{
			fillRect(pixel_buffer,WALL_COLOR,count*4,county*4,4,4);
		}
		else if(pix == '\n') //if we encountered a linebreak, add 1 to the y coordinate
		{
			county++;
			count =-1;
		}
		else if((pix == '!'||pix == '"') && vert == 0) //these are spikes or doors
		{
			doortrig[0] = count; //save the door, because we will need this later, to combine the button with the door
			doortrig[1] = county;
			if(pix == '"') //are we spikes?
			{
				spikes = 1;
			}
			else
			{
				spikes = 0;
			}
		}
		else if(pix == '#') //we encountered a grate, so draw it
		{
			drawRect(pixel_buffer,WALL_CRATE_COLOR,count*4,county*4+1,WALL_SIZE,DOOR_SIZE-2);
		}
		else if(pix == '+') //we encountered a box
		{
			fillRect(pixel_buffer,CRATE_COLOR,count*4,county*4,PLAYER_SIZE,PLAYER_SIZE);
			crates[countcrate].coords[0] = count*4;
			crates[countcrate].coords[1] = county*4;
			countcrate++;
		}

		if(count -1 == doortrig[0] && county == doortrig[1] && pix != ' ') //if we have a door or spike to the left of us, and this spot is not empty, then we are going to make a spike or horizontal door
		{
			doors[pix-50].coords[0] = count -1; //save the door into the array
			doors[pix-50].coords[1] = county;
			doors[pix-50].vert = 0; //the door is not vertical
			doors[pix-50].spikes = spikes;
			alt_8 i = 0;
			if(doors[pix-50].spikes == 1) //if we have spikes we draw 8 brown lines
			{
				for(i=0;i<8;i++)
				{
					drawLine(pixel_buffer,SPIKE_COLOR,(count)*4+i*2-1,(county*4),(count)*4+i*2-1,(county*4)+4);
				}
			}
			else //otherwise we draw a door
			{
				fillRect(pixel_buffer,DOOR_COLOR,(count-1)*4+1,county*4,DOOR_SIZE,WALL_SIZE);
			}
		}
		else if(count-1 == doortrig[0] && county == doortrig[1] && pix == ' ' ) //if this spot is empty, we have a vertical door
		{
			vert = 1;
		}
		else if(vert == 1 && count == doortrig[0] && county-1 == doortrig[1] && pix != ' ') //if the door is vertical, the x value corrosponds with a door and the door is above us, and this spot is not empty, we are going to draw a vertical door
		{
			fillRect(pixel_buffer,DOOR_COLOR,(count)*4,(county*4)-3,WALL_SIZE,DOOR_SIZE);
			doors[pix-50].coords[0] = count;
			doors[pix-50].coords[1] = county -1;
			doors[pix-50].vert = 1;
			vert = 0;
		}
		else if(pix > '1' && pix < 'F') //if the button is between '1' and 'F' in the ascii table, we create a regular button
		{
			drawRect(pixel_buffer,BUTTON_COLOR,count*4,county*4,BUTTON_SIZE,BUTTON_SIZE);
			buttons[pix-50].coords[0] = count;
			buttons[pix-50].coords[1] = county;

		}
		else if(pix >= 'F') //if it is higher, we create a crate triggered button
		{
			drawRect(pixel_buffer,BUTTON_CRATE_COLOR,count*4,county*4,BUTTON_SIZE,BUTTON_SIZE);
			buttons[pix-50].coords[0] = count;
			buttons[pix-50].coords[1] = county;
		}


		pix =  alt_up_sd_card_read(file); //read the next character
		count++;
	}
	ALT_SEM_POST(level_sem);
	OSTaskDel(OS_PRIO_SELF);
}
void clearOldText()
{
	//clears old crap off the screen
	alt_u8 i;
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
}
void MainMenuTask(void* pdata)
{
	OSTimeDly(100);
	fillScreen(pixel_buffer, 0x00F0);

	// Init players
	Player p1 = { SCREEN_WIDTH/4, SCREEN_HEIGHT-PLAYER_SIZE-PLAYER_SIZE/2-2, NONE, NONE, NONE };
	Player p2 = { SCREEN_WIDTH/4*3, SCREEN_HEIGHT-PLAYER_SIZE-PLAYER_SIZE/2-2, NONE, NONE, NONE };
	players[0] = p1;
	players[1] = p2;

	clearOldText();
	drawRect(pixel_buffer,0xFFFF,SCREEN_WIDTH/2-50,SCREEN_HEIGHT/2-12,100,20);
	drawText(character_buffer,"Press X to start playing",40-12,29);
	fillRect(pixel_buffer,0xAAAA,SCREEN_WIDTH/2-50,SCREEN_HEIGHT/4-12,100,20);
	drawText(character_buffer,"SPLITRUNNERS",40-6,14);
	drawText(character_buffer,"(c) Jerko Lenstra & Rick de Bondt - 2016",40-20,44);

	//Do initialize the controls, otherwise we can't press X
	OSTaskCreateExt(ControlsTask,
			0,
			(void *)&ControlsTask_STK[TASK_STACKSIZE-1],
			CONTROLS_PRIORITY,
			CONTROLS_PRIORITY,
			ControlsTask_STK,
			TASK_STACKSIZE,
			NULL,
			0);
	int pNum;

	//Wait until player 1 presses X
	while(players[0].action != 2)
	{
		OSTimeDly(1);

	}
	//Clear the main menu text
	drawText(character_buffer,"                        ",40-12,29);
	drawText(character_buffer,"            ",40-6,14);
	drawText(character_buffer,"                                        ",40-20,44);

	//Start creating all the tasks we need for the level
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
				(void *)pNum,
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
	//Put the timer on 0 right before we start
	ALT_SEM_PEND(timer,0);
	min = 0;
	sec = 0;
	ALT_SEM_POST(timer);
	OSTaskDel(OS_PRIO_SELF);


}
void FinishTask(void *pdata)
{
	char tempName[4] = { 0 };
	sprintf(tempName, "AAA");
	char name[4] = { 0 };
	char scoreText[22] = { 0 };
	alt_u8 cursorPos = 0;
	alt_u8 cursorDrawn = 0;

	//Draw Highscores
	drawText(character_buffer, highScores[0][0], 3, 2);
	drawText(character_buffer, highScores[0][1], 10, 2);
	drawText(character_buffer, highScores[1][0], 3, 3);
	drawText(character_buffer, highScores[1][1], 10, 3);
	drawText(character_buffer, highScores[2][0], 3, 4);
	drawText(character_buffer, highScores[2][1], 10, 4);

	while (1)
	{
		//This task will start doing something when both finish flags have been set
		ALT_FLAG_PEND(finish_flag, FINISH_1 + FINISH_2, OS_FLAG_WAIT_SET_ALL, 0);
		ALT_SEM_PEND(timer, 0);

		//Delete the players
		OSTaskDel(PLAYER_PRIORITY);
		OSTaskDel(PLAYER_PRIORITY+1);

		//Draw the window
		fillRect(pixel_buffer, BG_COLOR + 1024, 100, 80, 120, 80);
		drawRect(pixel_buffer, 0, 99, 79, 122, 82);
		drawRect(pixel_buffer, 0xFFFF, 100, 80, 120, 80);

		if (checkScore() >= 0)
		{
			//If there is a highscore we tell the player he did it and we ask him to enter his name
			sprintf(scoreText, "New highscore! %.2d:%.2d!", min, sec);
			drawText(character_buffer, scoreText, 29, 22);
			drawText(character_buffer, "Enter your teamname below", 27, 24);
			drawText(character_buffer, "by using the DPAD.", 31, 25);
			drawText(character_buffer, "Press X to submit.", 31, 36);
			drawText(character_buffer, "(The game will restart)", 29, 37);

			while (1)
			{
				//Character selection
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
					//When X has been pressed, save the score
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
					//Make sure the white rectangle in the high score screen is always drawn
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
			//If the player did not reach the high score
			sprintf(scoreText, "No highscore, try again!");
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
