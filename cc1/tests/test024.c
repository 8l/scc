/* See LICENSE file for copyright and license details. */

/*
name: TEST024
description: Basic test for long long constants
comments: This test is done for z80 data types
error:
output:
G2	I	F	"main
{
\
A3	Q	"i
A4	O	"u
	A3	#Q1	:Q
	A3	#QFFFFFFFFFFFFFFFF	:Q
	A3	#QFFFFFFFFFFFFFFFF	:Q
	A3	#QFFFF	:Q
	A3	#QFFFFFFFFFFFFFFFF	:Q
	A3	#Q3	:Q
	A3	#Q1	:Q
	A4	#O1	:O
	A4	#OFFFFFFFFFFFFFFFF	:O
	A4	#OFFFFFFFFFFFFFFFF	:O
	A4	#OFFFF	:O
	A4	#OFFFFFFFFFFFFFFFF	:O
	A4	#O3	:O
	A4	#O0	:O
	h	#I0
}
*/

int
main(void)
{
	long long i;
	unsigned long long u;

	i = 1;
	i = -1;
	i = -1l;
	i = -1u;
	i = -1ll;
	i = -1ll & 3;
	i = -1ll < 0;

	u = 1;
	u = -1;
	u = -1l;
	u = -1u;
	u = -1ll;
	u = -1llu & 3;
	u = -1llu < 0;
	return 0;
}
