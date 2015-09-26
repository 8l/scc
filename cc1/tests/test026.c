
/*
name: TEST026
descritpion: Test of predefined cpp macros
output:
F1
G2	F1	main
{
\
A3	I	y
A5	P	p
	A5	"746573743032362E63	'P	:P
	A3	#I1E	:I
	A3	#I1	:I
	A3	#I1	:I
	A3	#I1	:I
	A3	A3	:I
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
