#include "address_map_nios2.h"
#include <stdio.h>

/* Declare volatile pointers to I/O registers (volatile means that IO load
	   and store instructions will be used to access these pointer locations,
	   instead of regular memory loads and stores) */
volatile int * JTAG_UART_ptr 	= (int *) JTAG_UART_BASE;	// JTAG UART address
volatile int * RED_LED_ptr 		= (int *) RED_LED_BASE;		// RED LED address

/********************************************************************************
 * Controller codes are formatted in the following way:
 * (Controller number) (Button) (Pressed/Released)
 *
 * For example:
 * 	1ap			controller 1 - button A - pressed
 * 	1ar			controller 1 - button A - released
 * 	2bp			controller 2 - button B - pressed
 * 	etc.
 *
********************************************************************************/


int main(void)
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
				cmd[0] = data;
				data = *(JTAG_UART_ptr);		 		// read the JTAG_UART data register
				if (data & 0x00008000)					// check RVALID to see if there is new data
				{
					data = data & 0x000000FF;			// the data is in the least significant byte
					while (1)
					{
						cmd[1] = data;
						data = *(JTAG_UART_ptr);		 		// read the JTAG_UART data register
						if (data & 0x00008000)					// check RVALID to see if there is new data
						{
							data = data & 0x000000FF;			// the data is in the least significant byte
							while ((char) data == 'r' || (char) data == 'p')
							{
								cmd[2] = data;
								break;
							}
						}
						break;
					}
				}
				break;
			}
		}
		if (cmd[0] != 0 && cmd[1] != 0 && cmd[2] != 0)
		{
//			printf("\n");
//			for (i = 0; i < sizeof(cmd)/sizeof(cmd[0]); i++)
//			{
//				printf("%c", cmd[i]);
//			}
//			printf("\n");
			switch(cmd[2])
			{
				case 'p':
					*(RED_LED_ptr) = (1<<3);
					break;
				case 'r':
					*(RED_LED_ptr) = (0<<3);
					break;
			}
			for (i = 0; i < sizeof(cmd)/sizeof(cmd[0]); i++)
			{
				cmd[i] = 0;
			}
			i = 0;
		}
	}
}
