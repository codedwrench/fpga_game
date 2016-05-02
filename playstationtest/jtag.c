#include "address_map_nios2.h"
#include <stdio.h>

/* function prototypes */
char put_jtag(volatile int *, char);

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
	/* Declare volatile pointers to I/O registers (volatile means that IO load
	   and store instructions will be used to access these pointer locations,
	   instead of regular memory loads and stores) */
	volatile int * JTAG_UART_ptr 	= (int *) JTAG_UART_BASE;	// JTAG UART address

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
			cmd[i] = put_jtag (JTAG_UART_ptr, (char) data);
//			printf("\ncmd[%i]: %c\n", i, cmd[i]);
			if (i < 2)
				i++;
			else
				i = 0;
		}
		if (cmd[0] != 0 && cmd[1] != 0 && cmd[2] != 0)
		{
			printf("\n");
			for (i = 0; i < sizeof(cmd)/sizeof(cmd[0]); i++)
			{
				printf("%c", cmd[i]);
			}
			printf("\n");
			for (i = 0; i < sizeof(cmd)/sizeof(cmd[0]); i++)
			{
				cmd[i] = 0;
			}
			i = 0;
		}
	}
}

/********************************************************************************
 * Subroutine to send a character to the JTAG UART
********************************************************************************/
char put_jtag( volatile int * JTAG_UART_ptr, char c )
{
	int control;
	control = *(JTAG_UART_ptr + 1);			// read the JTAG_UART control register
	if (control & 0xFFFF0000)				// if space, then echo character, else ignore
	{
		*(JTAG_UART_ptr) = c;
	}
	return c;
}
