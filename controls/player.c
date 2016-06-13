#include "player.h"
void movePlayer(alt_u8 pNum, alt_u8 dir)
{
	alt_u8 willCollide = 0;
	alt_8 btnPressed = -1;
	alt_u16 x, y, p, p0, p1;
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
		p0 = 0; //make sure these are not used unitialized
		p1 = 0;
		x = 0;
		y = 0;
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
			if(buttons[i].coords[0] != 0) //also redraw the buttons
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
void PlayerTask(void* pdata)
{
	int pNum = (int) pdata;

	// Draw initial playerbox
	ALT_SEM_PEND(display, 0);
	fillRect(pixel_buffer, PLAYER_COLOR/(pNum+1), players[pNum].x, players[pNum].y, PLAYER_SIZE, PLAYER_SIZE);
	ALT_SEM_POST(display);

	while(1)
	{
		waitForVSync(buffer_register, dma_control);
		OSTimeDly(20);
		//move the player when one of the direction buttons is pushed
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
		fillRect(pixel_buffer, PLAYER_COLOR/(pNum+1), players[pNum].x, players[pNum].y, PLAYER_SIZE, PLAYER_SIZE);
		ALT_SEM_POST(display);
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
				//parse the console output and tell the program to perform an action based on that
				pNum = cmd[0] - '0' - 1; //check if it is a command for player 1 or player 2
				switch(cmd[1])
				{
				case 'g': // g = down, h = up, l = left, r= right, 0-9 = buttons
					if(cmd[2] != 'r') //r = released, p = pressed
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
				case '4': //in all these cases we perform this action
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
				for (i = 0; i < sizeof(cmd)/sizeof(cmd[0]); i++) //fill the array with zeroes
					cmd[i] = 0;
				i = 0;
			}
		}
		OSTimeDly(1);
	}
}
