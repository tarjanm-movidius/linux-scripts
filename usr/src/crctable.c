/* --------------------------------------------------------------------------
 * ----| crctable.c
 * ----|
 * ----| Creates the CRC feedback terms table for the 8-bit CRC polynomial
 * ----| X^8 + X^6 + X^1 + X^0 = 0001 0100 0011 binary = 0x0143 hex.
 * ----|
 * ----| The feedback terms table consists of 256 8-bit entries.  They
 * ----| simply represent the results of eight shift/xor operations for
 * ----| combinations of data and CRC values.
 * ----|
 * --------------------------------------------------------------------------
 * ----| NOTES:
 * ----|
 * ----| In C, U means unsigned, 0x in front of a number means it is hex.
 * ----|
 * ----| In printf(), %x means print the number in hex format.
 * ----|
 * ----| The ^ operator means bitwise XOR.  For example, (3 ^ 6) = 5, since
 * ----| 3 = 0011b and 6 = 0110b, which results in 0101b = 5.
 * ----|
 * ----| In C, the << operator means shift-left and >> means shift-right.
 * ----| For example, (3 << 6) = 192U, since 3 = 00000011b, which when
 * ----| shifted to the left by 6 positions is 11000000b = 192U.
 * ----|
 * ----| The % operator means modulus which returns the remainder of
 * ----| integer division.  For example 15 % 4 = 3.  The result can never
 * ----| be greater than or equal to the divisor.  It is used in this
 * ----| program to print a carriage-return every 4th column.
 * ----|
 * --------------------------------------------------------------------------
 * ----| Compile with:  cc -Wall -s crctable.c -o crctable
 * --------------------------------------------------------------------------
 * ----| Usage:  crctable [polynomial_hex_value]
 * ----|                   (defaults to 0x143)
 * --------------------------------------------------------------------------
 * ----| Brad Noble - Tue Sep 17 06:13:15 CDT 2002
 * ----| Marcell Tarjan - extended with c-style prints and paramter input
 * --------------------------------------------------------------------------
 *
 */

#include <stdio.h>

#define CRCCODE 0x0143
#define CRCPADLEN 8

/* ----| Since it is used in every function, we'll make the table global |---- */
unsigned crctab[256];
unsigned generator = CRCCODE;

void make_crc_table()
/* ----| Create the CRC lookup-table for 8 bit data. |---- */
{
	int i, j;
	unsigned crc;

	for (i=0 ; i<256 ; i++) { /* loop over all 8 bit data points */
		crc = i << CRCPADLEN; /* pad zeros to the data */
		for (j=7 ; j>=0 ; j--) { /* there are 8 bits to divide */
			if (crc >> (j+CRCPADLEN)) { /* if the bit is one... */
				crc = crc ^ (generator << j); /* XOR with generator... */
			} /* otherwise go to the next bit */
		}
		crctab[i]=crc; /* store the result in the table */
	}
}

void print_crc_table()
/* ----| Prints the CRC results in a "pretty" way. |---- */
{
	int i, j;

	printf( "/* CRC feedback terms table for the polynomial 0x%X */\nconst unsigned char CRC[] = { \\\n", generator );
	for( i = 0; i < 16; i++ )
	{
		for( j = 0; j < 16; j++ )
		{
			if( i < 15 || j < 15 ) printf( " 0x%02X,", crctab[ i * 16 + j ] );
			  else printf( " 0x%02X };\n", crctab[255] );
		}
		if( i < 15 ) printf( " \\\n" );
	}
}


int main(int argc, char *argv[])
{

	int i;

	if( argc != 2 )
	{
		fprintf(stderr, "Parameter missing, defaulting to generator polynomial 0x%X\nUsage: %s [polynomial_hex_value]\n\n", CRCCODE, argv[0]);
	} else {
		generator = 0;
		for( i = 0; argv[1][i]; i++ )
		{
			generator <<= 4;
			if (argv[1][i] == 'x' || argv[1][i] == 'X') generator = 0;
			else if (argv[1][i] >= '0' && argv[1][i] <= '9') generator += argv[1][i] - '0';
			else if (argv[1][i] >= 'a' && argv[1][i] <= 'f') generator += argv[1][i] - ('a' - 10);
			else if (argv[1][i] >= 'A' && argv[1][i] <= 'F') generator += argv[1][i] - ('A' - 10);
			else
			{
				fprintf(stderr, "Parameter error: '%s', defaulting to generator polynomial 0x%X\nUsage: %s [polynomial_hex_value]\n\n", argv[1], CRCCODE, argv[0]);
				generator = CRCCODE;
				break;
			}
		}
	}
	make_crc_table();
	print_crc_table();
	return(0);
}
