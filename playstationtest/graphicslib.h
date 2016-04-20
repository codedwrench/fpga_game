#ifndef GRAPHICSLIB
#define GRAPHICSLIB
#include <alt_types.h>
#include <stdlib.h>

void wait_for_vsync(volatile short * buffer_register,volatile short *  dma_control);
void drawbox(volatile short * pixel_ctrl_ptr, alt_u16 x0,alt_u16 y0,alt_u8 sizex,alt_u8 sizey,alt_u16 color);
void clearscreen(volatile short * pixel_ctrl_ptr);
void drawcircle(volatile short * pixel_ctrl_ptr, alt_u16 xs, alt_u8 ys, alt_u8 circle_radius, alt_u16 color);
void drawline(volatile short * pixel_ctrl_ptr, alt_u16 x0, alt_u16 y0,alt_u16 x1,alt_u16 y1,alt_u16 color);
void drawtext(volatile char * character_buffer, char * text_ptr,alt_u16 x, alt_u8 y);
void drawrectangle(volatile short * pixel_ctrl_ptr, alt_u16 x0,alt_u16 y0,alt_u8 sizex,alt_u8 sizey,alt_u16 color);

#endif
