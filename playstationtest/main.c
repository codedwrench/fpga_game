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
#include <alt_types.h>
#include "graphicslib.h"

volatile short  * pixel_buffer_start = (short *) 0x08000000;
volatile short * buffer_register = (short *) 0x10003060;
volatile short * dma_control = (short *) 0x1000306C;
volatile char * character_buffer = (char *) 0x09000000;

/* Definition of Task Stacks */
#define   TASK_STACKSIZE       2048
OS_STK    task1_stk[TASK_STACKSIZE];
OS_STK    task2_stk[TASK_STACKSIZE];

/* Definition of Task Priorities */
#define TASK1_PRIORITY      2
#define TASK2_PRIORITY      1

void task1(void* pdata)
{

	OSTimeDly(1);
	volatile short * pixel_ctrl_ptr = (volatile short *) pdata;

	int i=0;
	//d-pad
	drawrectangle( pixel_ctrl_ptr , 80, 100, 10, 15, 0xFFFF);
	drawrectangle( pixel_ctrl_ptr , 80, 130, 10, 15, 0xFFFF);
	drawrectangle( pixel_ctrl_ptr , 62, 117, 15, 10, 0xFFFF);
	drawrectangle( pixel_ctrl_ptr , 93, 117, 15, 10, 0xFFFF);

	//select start
	drawrectangle( pixel_ctrl_ptr , 130, 117,15, 10, 0xFFFF);
	drawrectangle( pixel_ctrl_ptr , 160, 117, 15, 10, 0xFFFF);

	//buttons
	drawcircle( pixel_ctrl_ptr ,205,121,7,0xFFFF);
	drawcircle( pixel_ctrl_ptr ,235,121,7,0xFFFF);
	drawcircle( pixel_ctrl_ptr ,220,106,7,0xFFFF);
	drawcircle( pixel_ctrl_ptr ,220,136,7,0xFFFF);

	//upper keys
	drawrectangle( pixel_ctrl_ptr , 75, 80, 20, 10, 0xFFFF);
	drawrectangle( pixel_ctrl_ptr , 75, 65, 20, 10, 0xFFFF);
	drawrectangle( pixel_ctrl_ptr , 210, 80, 20,10, 0xFFFF);
	drawrectangle( pixel_ctrl_ptr , 210, 65, 20, 10, 0xFFFF);

	//analog sticks
	drawcircle( pixel_ctrl_ptr ,110,155,15,0xFFFF);
	drawcircle( pixel_ctrl_ptr ,195,155,15,0xFFFF);
	drawcircle( pixel_ctrl_ptr ,110,155,9,0xFFFF);
	drawcircle( pixel_ctrl_ptr ,195,155,9,0xFFFF);
	while(1)
	{
		if(i>=10)
		{
			drawline(pixel_ctrl_ptr,0+i-10,50,0+i-10,60,0x0000);
			drawline(pixel_ctrl_ptr,0+i-11,50,0+i-11,60,0x0000);
		}
		if(i<=320)
		{
			drawline(pixel_ctrl_ptr,0+i,50,0+i,60,0x0FFF);
			drawline(pixel_ctrl_ptr,0+i+1,50,0+i+1,60,0x0FFF);
		}
		if(i==330)
		{
			i = 0;
		}
		i++;
		wait_for_vsync(buffer_register,dma_control);
		OSTimeDly(1);
	}
}
void task2(void* pdata)
{

		drawtext((volatile char* )pdata, "FPGA_GAME\0" ,36, 10);
		OSTaskDel(OS_PRIO_SELF);

}
int main(void)
{
	OSInit();
	volatile short * pixel_ctrl_ptr = pixel_buffer_start;
	volatile char * char_ctrl_ptr = character_buffer;

	*(dma_control) &= (1<<2); //Enable DMA controller

	clearscreen(pixel_ctrl_ptr);
	drawbox(pixel_ctrl_ptr, 34*4, 9*4, 48, 10,0xFA01);

	wait_for_vsync(buffer_register,dma_control);




	OSTaskCreateExt(task1,
			pixel_ctrl_ptr,
			(void *)&task1_stk[TASK_STACKSIZE-1],
			TASK1_PRIORITY,
			TASK1_PRIORITY,
			task1_stk,
			TASK_STACKSIZE,
			NULL,
			0);


	OSTaskCreateExt(task2,
			char_ctrl_ptr,
			(void *)&task2_stk[TASK_STACKSIZE-1],
			TASK2_PRIORITY,
			TASK2_PRIORITY,
			task2_stk,
			TASK_STACKSIZE,
			NULL,
			0);
	OSStart();
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
