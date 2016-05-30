#include "graphicslib.h"
#include "gamelib.h"

void waitForVSync(volatile short* buffer_register, volatile short* dma_control)
{
	*(buffer_register) = 1; //swap backbuffer and front buffer (which are the same at the moment)

	while(*(dma_control) & 1) //if this bit becomes 0, the screen has updated
	{
	}
}
void clearScreen(volatile short * pixel_ctrl_ptr)
{
	alt_u8 row, col;
	for(row = 0; row <= 240; row++){
		for(col = 0; col <= 160; col++)
		{
			*(pixel_ctrl_ptr + (row << 9) + col) = 0;
			*(pixel_ctrl_ptr + (row << 9) + col+160) = 0;
		}
	}
}
void fillScreen(volatile short * pixel_ctrl_ptr, alt_u16 color)
{
	alt_u8 row, col;
	for(row = 0; row <= 240; row++){
		for(col = 0; col <= 160; col++)
		{
			*(pixel_ctrl_ptr + (row << 9) + col) = color;
			*(pixel_ctrl_ptr + (row << 9) + col+160) = color;
		}
	}
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
void drawRect(volatile short* pixel_ctrl_ptr, alt_u16 color, alt_u16 x0, alt_u16 y0, alt_u16 w, alt_u16 h)
{
	drawLine(pixel_ctrl_ptr, color, x0, y0, x0+w, y0);					// top line
	drawLine(pixel_ctrl_ptr, color, x0, y0, x0, y0+h);					// left line
	drawLine(pixel_ctrl_ptr, color, x0, y0+h, x0+w+1, y0+h);	// right line
	drawLine(pixel_ctrl_ptr, color, x0+w, y0, x0+w, y0+h);		// bottom line
}
void drawBox(volatile short* pixel_ctrl_ptr, alt_u16 color, alt_u16 x0, alt_u16 y0, alt_u8 w, alt_u8 h)
{
	alt_u8 row;
	alt_u16 col;
	for(row = y0; row <= y0+h; row++){
		for(col=x0; col <= x0+w; col++)
		{
			*(pixel_ctrl_ptr + (row << 9) + col) = color;
		}
	}
}
void drawCircle(volatile short* pixel_ctrl_ptr, alt_u16 color, alt_u16 xs, alt_u8 ys, alt_u8 r)
{
	int x = r;
	int y = 0;
	int err = -r;
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
void drawCollisionLine(Level* level_ptr, alt_u8 type, alt_u16 x0, alt_u16 y0, alt_u16 x1, alt_u16 y1)
{
	int dx = abs(x1-x0), sx = x0<x1 ? 1 : -1;
	int dy = abs(y1-y0), sy = y0<y1 ? 1 : -1;
	int err = (dx>dy ? dx : -dy)/2, e2;

	while(1)
	{
//		setPixel(x0,y0);
		level_ptr->map[x0][y0] = type;
		if (x0==x1 && y0==y1) break;
		e2 = err;
		if (e2 >-dx) { err -= dy; x0 += sx; }
		if (e2 < dy) { err += dx; y0 += sy; }
	}

}
void drawCollisionRect(volatile short* pixel_ctrl_ptr, Level* level_ptr, alt_u8 type, alt_u16 color, alt_u8 visible, alt_u16 x0, alt_u16 y0, alt_u16 w, alt_u16 h)
{
	if (visible)
		drawRect(pixel_ctrl_ptr, color, x0, y0, w, h);
	drawCollisionLine(level_ptr, type, x0, y0, x0+w, y0);				// top
	drawCollisionLine(level_ptr, type, x0, y0, x0, y0+h);				// left
	drawCollisionLine(level_ptr, type, x0, y0+h, x0+w, y0+h);			// right
	drawCollisionLine(level_ptr, type, x0+w, y0, x0+w, y0+h);			// bottom
}
void drawButton(volatile short* pixel_ctrl_ptr, Level* level_ptr, alt_u8 num, alt_u8 type, alt_u16 color, alt_u16 x0, alt_u16 y0)
{
	drawCollisionRect(pixel_ctrl_ptr, level_ptr, type, color, 1, x0, y0, BUTTON_SIZE, BUTTON_SIZE);
	Button btn = { x0, y0, num };
	level_ptr->buttons[num] = btn;
}
