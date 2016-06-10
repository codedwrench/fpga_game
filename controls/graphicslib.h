#ifndef GRAPHICSLIB
#define GRAPHICSLIB
#include "main.h"

void waitForVSync(volatile short* buffer_register, volatile short* dma_control);
void fillScreen(volatile short * pixel_ctrl_ptr, alt_u16 color);
void drawLine(volatile short* pixel_ctrl_ptr, alt_u16 color, alt_u16 x0, alt_u16 y0, alt_u16 x1, alt_u16 y1);
void drawRect(volatile short* pixel_ctrl_ptr, alt_u16 color, alt_u16 x0, alt_u16 y0, alt_u16 w, alt_u16 h);
void fillRect(volatile short* pixel_ctrl_ptr, alt_u16 color, alt_u16 x0, alt_u16 y0, alt_u8 w, alt_u8 h);
void drawText(volatile char* char_ctrl_ptr, char* text_ptr, alt_u16 x, alt_u8 y);
void drawPixel(volatile short * pixel_ctrl_ptr,alt_u16 x, alt_u8 y, alt_u16 color);
alt_u16 getPixel(volatile short * pixel_ctrl_ptr, alt_u16 x, alt_u8 y);



#endif
