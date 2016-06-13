#include "collisions.h"
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
