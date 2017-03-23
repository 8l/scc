/* See LICENSE file for copyright and license details. */

/*
name: TEST026
descritpion: Test of predefined cpp macros
error:
output:
G3	I	F	"main
{
\
A4	I	"y
A6	P	"p
V7	K	#NA
Y8	V7	"	(
	#"test026.c
	#K00
)
	A6	Y8	'P	:P
	A4	#I24	:I
	A4	#I1	:I
	A4	#I1	:I
	A4	#I1	:I
	A4	#I1	:I
}
*/

#define x(y)  (y)

int
main(void)
{
	int y;
	char *p;

	p = __FILE__;
	y = __LINE__;
/*	p = __DATE__;   __DATE__ generates  different value each time */
	y = __STDC__;
/*	p = __TIME__;   __TIME__ generates  different value each time */
	y = __STDC_HOSTED__;
	y = __SCC__;
	y = x(1);
}
