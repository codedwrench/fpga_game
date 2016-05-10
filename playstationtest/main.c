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
#include <os/alt_sem.h>
#include "graphicslib.h"
#include "address_map_nios2.h"
#define BUF_SIZE 5000000			// about 10 seconds of buffer (@ 48K samples/sec)
#define BUF_THRESHOLD 96		// 75% of 128 word buffer

volatile short  * pixel_buffer_start = (short *) 0x08000000;
volatile short * buffer_register = (short *) 0x10003060;
volatile short * dma_control = (short *) 0x1000306C;
volatile char * character_buffer = (char *) 0x09000000;
volatile int * audio_ptr = (int *) 0x10003040;			// audio port address
int left_buffer[BUF_SIZE];
int right_buffer[BUF_SIZE];
volatile int * RED_LED_ptr 		= (int *) RED_LED_BASE;		// RED LED address
volatile int * JTAG_UART_ptr 	= (int *) JTAG_UART_BASE;	// JTAG UART address
int fifospace, leftdata, rightdata;

/* Definition of Task Stacks */
#define   TASK_STACKSIZE       2048
OS_STK    task1_stk[TASK_STACKSIZE];
OS_STK    task2_stk[TASK_STACKSIZE];
OS_STK    getcontrols_stk[TASK_STACKSIZE];
ALT_SEM(display)
ALT_SEM(audio)


/* Definition of Task Priorities */
#define TASK1_PRIORITY      6
#define TASK2_PRIORITY      5
#define GETCONTROLS_PRIORITY      7

void playtone(int height,int time)
{


		signed long high = 2147483392;
		signed long low = -2147483648;
		int  buffer_index = 0;
		fifospace = 0;
		int i = 0;
			for( i = 0; i<time*10000;i++)
			{

				if(buffer_index < height)
				{
				*(audio_ptr + 2) = high;
				*(audio_ptr + 3) = high;
				}
				else if (buffer_index > height)
				{
					*(audio_ptr + 2) = low;
					*(audio_ptr + 3) = low;
				}
				if(buffer_index == 1+height*2)
				{
					buffer_index = 0;
				}


				++buffer_index;

			}

}
void task1(void* pdata)
{
	ALT_SEM_PEND(display,0);

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
		ALT_SEM_POST(display);
	}
}
void task2(void* pdata)
{

	drawtext((volatile char* )pdata, "FPGA_GAME\0" ,36, 10);
	OSTaskDel(OS_PRIO_SELF);

}
void getcontrols(void *pdata)
{

	int data, i;
	char cmd[3];

	for (i = 0; i < sizeof(cmd)/sizeof(cmd[0]); i++)
	{
		cmd[i] = 0;
	}
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
				ALT_SEM_PEND(display,0);

				//		printf("\n");
				//		for (i = 0; i < sizeof(cmd)/sizeof(cmd[0]); i++)
				//		{
				//			printf("%c", cmd[i]);
				//		}
				//		printf("\n");
				alt_u16 x;
				alt_u8 y;
				switch(cmd[1])
				{
				case 'd':
					x = 81;
					y= 66;
					break;
				case 'c':
					x = 81;
					y = 81;
					break;
				case 'e':
					x = 216;
					y = 66;
					break;
				case  'z':
					x = 216;
					y = 81;
					break;
				case 'h':
					x = 81;
					y = 104;
					break;
				case 'g':
					x = 81;
					y = 134;
					break;
				case 'l':
					x = 65;
					y = 118;
					break;
				case 'r':
					x = 96;
					y= 118;
					break;
				case 'f':
					x = 133;
					y= 118;
					break;
				case 's':
					x = 163;
					y = 118;
					break;
				case 'y':
					x = 216;
					y = 102;
					break;
				case 'x':
					x = 201;
					y = 117;
					break;
				case 'b':
					x = 231;
					y = 117;
					break;
				case 'a':
					x = 216;
					y = 132;
					break;
				default:
					x = 0;
					y = 0;
					break;


				}


				switch(cmd[2])
				{
				case 'p':
					drawrectangle(pixel_buffer_start,x,y,8,8,0xFFFF);
					int i;
					int err;
					playtone(5000,10);
					playtone(8000,10);
					playtone(5000,10);







					*(RED_LED_ptr) = (1<<3);
					break;
				case 'r':
					drawrectangle(pixel_buffer_start,x,y,8,8,0x0000);
					*(RED_LED_ptr) = (0<<3);
					break;
				}
				for (i = 0; i < sizeof(cmd)/sizeof(cmd[0]); i++)
				{
					cmd[i] = 0;
				}
				i = 0;
			}
			OSTimeDly(1);
			ALT_SEM_POST(display);

		}
	}
}

int main(void)
{
	OSInit();
	volatile short * pixel_ctrl_ptr = pixel_buffer_start;
	volatile char * char_ctrl_ptr = character_buffer;
	int err = ALT_SEM_CREATE(&display,1);
	if(err != 0)
		printf("Semaphore not created\n");
	 err = ALT_SEM_CREATE(&audio,1);
	if(err != 0)
		printf("Semaphore not created\n");

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

	OSTaskCreateExt(getcontrols,
			char_ctrl_ptr,
			(void *)&getcontrols_stk[TASK_STACKSIZE-1],
			GETCONTROLS_PRIORITY,
			GETCONTROLS_PRIORITY,
			getcontrols_stk,
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
