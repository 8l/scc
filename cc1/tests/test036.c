
/*
name: TEST036
description: Duff's device
error:
test036.c:60: warning: type defaults to 'int' in declaration
output:
G5	F	"send
{
R1	P	"to
R2	P	"from
R3	I	"count
\
R7	I	"n
	R7	R3	#I7	+I	#I8	/I	:I
	s	L9	R3	#I8	%I
L10
	e
L11
	R1	@I	R2	#N2	:iP	@I	:I
L13
	R1	@I	R2	#N2	:iP	@I	:I
L14
	R1	@I	R2	#N2	:iP	@I	:I
L15
	R1	@I	R2	#N2	:iP	@I	:I
L16
	R1	@I	R2	#N2	:iP	@I	:I
L17
	R1	@I	R2	#N2	:iP	@I	:I
L18
	R1	@I	R2	#N2	:iP	@I	:I
L19
	R1	@I	R2	#N2	:iP	@I	:I
	y	L11	R7	#I1	:-I	#I0	>I
	b
L12
	j	L8
L9
	t	#N8
	v	L19	#I1
	v	L18	#I2
	v	L17	#I3
	v	L16	#I4
	v	L15	#I5
	v	L14	#I6
	v	L13	#I7
	v	L10	#I0
L8
}
*/

/* Disgusting, no?  But it compiles and runs just fine.  I feel a combination of
   pride and revulsion at this discovery.  If no one's thought of it before,
   I think I'll name it after myself.
   It amazes me that after 10 years of writing C there are still
   little corners that I haven't explored fully.
   - Tom Duff */

send(to, from, count)
	register short *to, *from;
	register count;
{
	register n=(count+7)/8;
	switch(count%8){
	case 0:      do{*to = *from++;
	case 7:           *to = *from++;
	case 6:           *to = *from++;
	case 5:           *to = *from++;
	case 4:           *to = *from++;
	case 3:           *to = *from++;
	case 2:           *to = *from++;
	case 1:           *to = *from++;
	                    }while(--n>0);
	}
}
