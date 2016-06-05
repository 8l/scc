/* See LICENSE file for copyright and license details. */

/*
name: TEST023
description: Basic test for long constants
comments: This test is done for z80 data types
error:
output:
G2	I	F	"main
{
\
A3	W	"i
A4	Z	"u
	A3	#W1	:W
	A3	#WFFFFFFFF	:W
	A3	#WFFFFFFFF	:W
	A3	#WFFFF	:W
	A3	#WFFFFFFFF	:W
	A3	#W3	:W
	A3	#W1	:W
	A3	#W0	:W
	A4	#Z1	:Z
	A4	#ZFFFFFFFF	:Z
	A4	#ZFFFFFFFF	:Z
	A4	#ZFFFF	:Z
	A4	#ZFFFFFFFF	:Z
	A4	#Z3	:Z
	A4	#Z1	:Z
	A4	#Z0	:Z
	h	#I0
}
*/

int
main(void)
{
	long i;
	unsigned long u;

	i = 1;
	i = -1;
	i = -1l;
	i = -1u;
	i = -1ll;
	i = (1ll << 32) - 1 & 3;
	i = (long) ((1ll << 32) - 1) < 0;
	i = -1u < 0;

	u = 1;
	u = -1;
	u = -1l;
	u = -1u;
	u = -1ll;
	u = (1ll << 32) - 1 & 3;
	u = (long) ((1ll << 32) - 1) < 0;
	u = -1u < 0;
	return 0;
}
