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
unsigned int playerx = 0;
unsigned int playery = 0;
unsigned char py;
unsigned char px;


/* Definition of Task Stacks */
#define   TASK_STACKSIZE       2048
OS_STK    task1_stk[TASK_STACKSIZE];
OS_STK    task2_stk[TASK_STACKSIZE];
OS_STK    getcontrols_stk[TASK_STACKSIZE];
ALT_SEM(display)
ALT_SEM(audio)
ALT_SEM(player)


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
	ALT_SEM_PEND(player,0);

	OSTimeDly(1);
	volatile short * pixel_ctrl_ptr = (volatile short *) pdata;

	int i=0;

	while(1)
	{
		wait_for_vsync(buffer_register,dma_control);
		OSTimeDly(10);
		if(py == 1)
		{
			playery++;
		}
		else if(py == 2)
		{
			if(playery > 0)
				playery--;
		}
		if(px == 1)
		{
			playerx++;
		}
		else if(px ==2 )
		{
			if(playerx > 0)
				playerx--;
		}
		drawrectangle( pixel_buffer_start,playerx-1,playery-1,7,7,0);
		drawrectangle( pixel_buffer_start,playerx,playery,5,5,0xFFFF);
		ALT_SEM_POST(player);
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
				ALT_SEM_PEND(player,0);


				switch(cmd[1])
				{
				case 'd':

					break;
				case 'c':

					break;
				case 'e':

					break;
				case  'z':

					break;
				case 'g':
					if(cmd[2] != 'r')
					{
						py = 1;
					}
					else
					{
						py = 0;
					}
					break;
				case 'h':
					if(cmd[2] != 'r')
					{
						py = 2;
					}
					else
					{
						py = 0;
					}
					break;
				case 'l':
					if(cmd[2] != 'r')
					{
						px = 2;
					}
					else
					{
						px =0;
					}
					break;
				case 'r':
					if(cmd[2] != 'r')
					{
						px = 1;
					}
					else
					{
						px = 0;
					}
					break;
				case 'f':

					break;
				case 's':

					break;
				case 'y':

					break;
				case 'x':

					break;
				case 'b':

					break;
				case 'a':

					break;
				default:

					break;


				}

				for (i = 0; i < sizeof(cmd)/sizeof(cmd[0]); i++)
				{
					cmd[i] = 0;
				}
				i = 0;

			}
			ALT_SEM_POST(player);

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
	err = ALT_SEM_CREATE(&player,1);
	if(err!= 0)
	{
		printf("Semaphore not created\n");
	}

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
