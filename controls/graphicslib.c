#include "graphicslib.h"
#include "gamelib.h"

void waitForVSync(volatile short* buffer_register, volatile short* dma_control)
{
	*(buffer_register) = 1; //swap backbuffer and front buffer (which are the same at the moment)

	while(*(dma_control) & 1) //if this bit becomes 0, the screen has updated
	{
	}
}
void fillScreen(volatile short * pixel_ctrl_ptr, alt_u16 color)
{
	alt_u8 row, col;
	for(row = 0; row <= 240; row++){
		for(col = 0; col <= 160; col++)
		{
			drawPixel(pixel_ctrl_ptr,col,row,color);
			drawPixel(pixel_ctrl_ptr,col+160,row,color);
			 //write a color to these spots of the pixel buffer, we use 2 functions to be able to use smaller variables
		}
	}
}
void drawPixel(volatile short * pixel_ctrl_ptr,alt_u16 x, alt_u8 y, alt_u16 color)
{
			*(pixel_ctrl_ptr + (y << 9) + x) = color;
}
alt_u16 getPixel(volatile short * pixel_ctrl_ptr, alt_u16 x, alt_u8 y)
{
			return (alt_u16)(*(pixel_ctrl_ptr + (y << 9) + x));
}
void drawLine(volatile short* pixel_ctrl_ptr, alt_u16 color, alt_u16 x0, alt_u16 y0, alt_u16 x1, alt_u16 y1)
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
	for(x = x0; x < x1; x++)
	{
		if(is_steep)
		{
			if(getPixel(pixel_ctrl_ptr,y,x) != color )
			{
				drawPixel(pixel_ctrl_ptr,y,x,color);
			}
		}
		else
		{
			if(getPixel(pixel_ctrl_ptr,x,y) != color)
			{
				drawPixel(pixel_ctrl_ptr,x,y,color);
			}
		}
		error = error + deltay;
		if(error >= 0)
		{
			y = y + y_step;
			error = error - deltax;
		}
	}
}
void drawRect(volatile short* pixel_ctrl_ptr, alt_u16 color, alt_u16 x0, alt_u16 y0, alt_u16 w, alt_u16 h)
{
	drawLine(pixel_ctrl_ptr, color, x0, y0, x0+w, y0);					// top line
	drawLine(pixel_ctrl_ptr, color, x0, y0, x0, y0+h);					// left line
	drawLine(pixel_ctrl_ptr, color, x0, y0+h, x0+w+1, y0+h);	// right line
	drawLine(pixel_ctrl_ptr, color, x0+w, y0, x0+w, y0+h);		// bottom line
}
void fillRect(volatile short* pixel_ctrl_ptr, alt_u16 color, alt_u16 x0, alt_u16 y0, alt_u8 w, alt_u8 h)
{
	alt_u8 row;
	alt_u16 col;
	for(row = y0; row <= y0+h; row++){
		for(col=x0; col <= x0+w; col++)
		{
			drawPixel(pixel_ctrl_ptr,col,row,color);
		}
	}
}
void drawText(volatile char* char_ctrl_ptr, char* text_ptr, alt_u16 x, alt_u8 y)
{
	int offset = (y<<7)+x;
	while(*(text_ptr))
	{
		*(char_ctrl_ptr + offset) = *(text_ptr); //write to character buffer
		++text_ptr;
		++offset;
	}
}

