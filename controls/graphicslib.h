#ifndef GRAPHICSLIB
#define GRAPHICSLIB
#include <alt_types.h>
#include <stdlib.h>
#include "gamelib.h"

void waitForVSync(volatile short* buffer_register, volatile short* dma_control);
void clearScreen(volatile short* pixel_ctrl_ptr);
void fillScreen(volatile short* pixel_ctrl_ptr, alt_u16 color);
void drawLine(volatile short* pixel_ctrl_ptr, alt_u16 color, alt_u16 x0, alt_u16 y0, alt_u16 x1, alt_u16 y1);
void drawRect(volatile short* pixel_ctrl_ptr, alt_u16 color, alt_u16 x0, alt_u16 y0, alt_u16 w, alt_u16 h);
void drawBox(volatile short* pixel_ctrl_ptr, alt_u16 color, alt_u16 x0, alt_u16 y0, alt_u8 w, alt_u8 h);
void drawCircle(volatile short* pixel_ctrl_ptr, alt_u16 color, alt_u16 xs, alt_u8 ys, alt_u8 r);
void drawText(volatile char* char_ctrl_ptr, char* text_ptr, alt_u16 x, alt_u8 y);
void drawCollisionLine(Level* level_ptr, alt_u8 type, alt_u16 x0, alt_u16 y0, alt_u16 x1, alt_u16 y1);
void drawCollisionRect(volatile short* pixel_ctrl_ptr, Level* level_ptr, alt_u8 type, alt_u16 color, alt_u8 visible, alt_u16 x0, alt_u16 y0, alt_u16 w, alt_u16 h);
void drawBtnAndDoor(volatile short* pixel_ctrl_ptr, Level* level_ptr, alt_u8 n);

#endif
