#include "graphicslib.h"

void clearscreen(volatile short * pixel_ctrl_ptr)
{
	alt_u8 row,col;
	for(row = 0;row<=240;row++){
		for(col=0;col <= 160;col++)
		{
			*(pixel_ctrl_ptr + (row << 9) + col) = 0;
			*(pixel_ctrl_ptr + (row << 9) + col+160) = 0;
		}
	}
}
void drawbox(volatile short * pixel_ctrl_ptr, alt_u16 x0,alt_u16 y0,alt_u8 sizex,alt_u8 sizey,alt_u16 color)
{
	alt_u8 row;
	alt_u16 col;
	for(row = y0;row<=y0+sizey;row++){
		for(col=x0;col <= x0+sizex;col++)
		{
			*(pixel_ctrl_ptr + (row << 9) + col) = color;
		}
	}
}
void drawrectangle(volatile short * pixel_ctrl_ptr, alt_u16 x0,alt_u16 y0,alt_u8 sizex,alt_u8 sizey,alt_u16 color)
{
	drawline(pixel_ctrl_ptr,x0,y0,x0+sizex,y0,color);
	drawline(pixel_ctrl_ptr,x0,y0,x0,y0+sizey,color);
	drawline(pixel_ctrl_ptr,x0,y0+sizey,x0+sizex+1,y0+sizey,color);
	drawline(pixel_ctrl_ptr,x0+sizex,y0,x0+sizex,y0+sizey,color);
}
void drawline(volatile short * pixel_ctrl_ptr, alt_u16 x0, alt_u16 y0,alt_u16 x1,alt_u16 y1,alt_u16 color)
{
	alt_u8 is_steep = abs(y1-y0) > abs(x1 -x0);
	alt_u16 temp;
	if (is_steep)
	{
		temp = x0;
		x0 = y0;
		y0 = temp;

		temp = x1;
		x1 = y1;
		y1 = temp;
	}
	if(x0 > x1)
	{
		temp = x0;
		x0 = x1;
		x1 = temp;

		temp = y0;
		y0 = y1;
		y1 = temp;
	}
	alt_u16 deltax = x1 - x0;
	alt_u16 deltay = abs(y1-y0);
	alt_16 error = -(deltax /2);
	alt_u16 y = y0;
	alt_u16 x = x0;
	alt_8 y_step;
	if(y0 < y1)
	{
		y_step =1;
	}
	else
	{
		y_step =-1;
	}
	for(x=x0;x<x1;x++)
	{
		if(is_steep)
		{
			*(pixel_ctrl_ptr + (x << 9) + y) = color;
		}
		else
		{
			*(pixel_ctrl_ptr + (y << 9) + x) = color;
		}
		error = error + deltay;
		if(error >= 0)
		{
			y = y + y_step;
			error = error - deltax;
		}
	}
}
void drawcircle(volatile short * pixel_ctrl_ptr, alt_u16 xs, alt_u8 ys, alt_u8 circle_radius, alt_u16 color)
{
	int x = circle_radius;
	int y = 0;
	int err = -circle_radius;
	while(x >= y)
	{
		*(pixel_ctrl_ptr + ((y+ys) << 9) + (xs+x)) = color;
		*(pixel_ctrl_ptr + ((ys+y) << 9) + (xs-x)) = color;
		*(pixel_ctrl_ptr + ((ys-y) << 9) + (xs+x)) = color;
		*(pixel_ctrl_ptr + ((ys-y) << 9) + (xs-x)) = color;
		*(pixel_ctrl_ptr + ((ys+x) << 9) + (xs+y)) = color;
		*(pixel_ctrl_ptr + ((x+ys) << 9) + (xs-y)) = color;
		*(pixel_ctrl_ptr + ((ys-x) << 9) + (xs+y)) = color;
		*(pixel_ctrl_ptr + ((ys-x) << 9) + (xs-y)) = color;

		err += y++;
		err += y;
		if(err >= 0)
		{
			x--;
			err = err -x -x;
		}


	}
}
void wait_for_vsync(volatile short * buffer_register,volatile short *  dma_control)
{
	*(buffer_register) = 1; //swap backbuffer and front buffer (which are the same at the moment)

	while(*(dma_control) & 1) //if this bit becomes 0, the screen has updated
	{
	}
}
void drawtext(volatile char * char_ctrl_ptr, char * text_ptr,alt_u16 x, alt_u8 y)
{
	int offset = (y<<7)+x;
	while(*(text_ptr))
	{
		*(char_ctrl_ptr + offset) = *(text_ptr); //write to character buffer
		++text_ptr;
		++offset;
	}
}
