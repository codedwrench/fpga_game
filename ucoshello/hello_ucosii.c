/*************************************************************************
* Copyright (c) 2004 Altera Corporation, San Jose, California, USA.      *
* All rights reserved. All use of this software and documentation is     *
* subject to the License Agreement located at the end of this file below.*
**************************************************************************
* Description:                                                           *
* The following is a simple hello world program running MicroC/OS-II.The * 
* purpose of the design is to be a very simple application that just     *
* demonstrates MicroC/OS-II running on NIOS II.The design doesn't account*
* for issues such as checking system call return codes. etc.             *
*                                                                        *
* Requirements:                                                          *
*   -Supported Example Hardware Platforms                                *
*     Standard                                                           *
*     Full Featured                                                      *
*     Low Cost                                                           *
*   -Supported Development Boards                                        *
*     Nios II Development Board, Stratix II Edition                      *
*     Nios Development Board, Stratix Professional Edition               *
*     Nios Development Board, Stratix Edition                            *
*     Nios Development Board, Cyclone Edition                            *
*   -System Library Settings                                             *
*     RTOS Type - MicroC/OS-II                                           *
*     Periodic System Timer                                              *
*   -Know Issues                                                         *
*     If this design is run on the ISS, terminal output will take several*
*     minutes per iteration.                                             *
**************************************************************************/


#include <stdio.h>
#include "includes.h"
#include "altera_up_avalon_video_pixel_buffer_dma.h"
#include "altera_up_avalon_video_character_buffer_with_dma.h"
#include "sys/alt_stdio.h"
#include "io.h"

/* Definition of Task Stacks */
#define   TASK_STACKSIZE       2048
OS_STK    task1_stk[TASK_STACKSIZE];



/* Display stuff */
alt_up_pixel_buffer_dma_dev *pixel_buffer_dev;
short buffer[512][256];
alt_up_char_buffer_dev *char_buffer_dev;


/* Definition of Task Priorities */

#define TASK1_PRIORITY      1
int clear_text_buffer(alt_up_char_buffer_dev *char_buffer);

/* Prints "Hello World" and sleeps for three seconds */
void task1(void* pdata)
{
	OSTimeDlyHMSM(0,0,5,0);
    alt_up_pixel_buffer_dma_draw_box(pixel_buffer_dev, 34*4, 28*4, 50*4, 32*4, 0x001F, 0);
	alt_up_char_buffer_string (char_buffer_dev, "Test\0", 35, 29);
    OSTaskDel(OS_PRIO_SELF);
}
/* Prints "Hello World" and sleeps for three seconds */
/* The main function creates two task and starts multi-tasking */
int main(void)
{
	/* initialize the pixel buffer HAL */
	pixel_buffer_dev = alt_up_pixel_buffer_dma_open_dev ("/dev/VGA_Subsystem_VGA_Pixel_DMA");
	if ( pixel_buffer_dev == NULL)
		alt_printf ("Error: could not open VGA pixel buffer device\n");
	else
		alt_printf ("Opened character VGA pixel buffer device\n");

	/* set both the main buffer and back buffer to the allocated space */
	alt_up_pixel_buffer_dma_change_back_buffer_address(pixel_buffer_dev, (unsigned int) buffer);
	alt_up_pixel_buffer_dma_swap_buffers(pixel_buffer_dev);
	while (alt_up_pixel_buffer_dma_check_swap_buffers_status(pixel_buffer_dev));
	alt_up_pixel_buffer_dma_change_back_buffer_address(pixel_buffer_dev, (unsigned int) buffer);

	/* clear the graphics screen */
	alt_up_pixel_buffer_dma_clear_screen(pixel_buffer_dev, 0);


  
  OSTaskCreateExt(task1,
                  NULL,
                  (void *)&task1_stk[TASK_STACKSIZE-1],
                  TASK1_PRIORITY,
                  TASK1_PRIORITY,
                  task1_stk,
                  TASK_STACKSIZE,
                  NULL,
                  0);
              

  OSStart();
  return 0;
}
int clear_text_buffer(alt_up_char_buffer_dev *char_buffer) {
	// boundary check
	//7632 is the size of the character buffer for whatever reason
	alt_u8 cnt = 0;
	alt_u8 i = 0;

	for ( cnt=0;cnt< char_buffer->y_resolution; cnt++)
	{
		for(i=0;i<char_buffer->x_resolution;i++)
		{
			IOWR_8DIRECT(char_buffer->buffer_base, (cnt<<7)+i, ' ');
		}
	}
	return 0;
}

/******************************************************************************
*                                                                             *
* License Agreement                                                           *
*                                                                             *
* Copyright (c) 2004 Altera Corporation, San Jose, California, USA.           *
* All rights reserved.                                                        *
*                                                                             *
* Permission is hereby granted, free of charge, to any person obtaining a     *
* copy of this software and associated documentation files (the "Software"),  *
* to deal in the Software without restriction, including without limitation   *
* the rights to use, copy, modify, merge, publish, distribute, sublicense,    *
* and/or sell copies of the Software, and to permit persons to whom the       *
* Software is furnished to do so, subject to the following conditions:        *
*                                                                             *
* The above copyright notice and this permission notice shall be included in  *
* all copies or substantial portions of the Software.                         *
*                                                                             *
* THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR  *
* IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,    *
* FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE *
* AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER      *
* LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING     *
* FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER         *
* DEALINGS IN THE SOFTWARE.                                                   *
*                                                                             *
* This agreement shall be governed in all respects by the laws of the State   *
* of California and by the laws of the United States of America.              *
* Altera does not recommend, suggest or require that this reference design    *
* file be used in conjunction or combination with any other product.          *
******************************************************************************/
