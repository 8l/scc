
/*
name: TEST025
descritpion: Test of ifdef and ifndef
error:
output:
G1	I	"a
G2	I	"b
G3	I	"c
G4	I	"d
G5	I	"_1
G6	I	"_2
G7	I	"e_
G8	I	"f_
G9	I	"h
G10	I	"i
*/

#define FOO

#ifdef FOO
	int a;
	int b;
	#undef FOO
	#ifndef FOO
		int c;
		int d;
	#else
		int e;
		int f;
	#endif
	int _1;
	int _2;
	#ifdef FOO
		int c_;
		int d_;
	#else
		int e_;
		int f_;
	#endif
	int h;
	int i;
#else
	int j;
	int k;
	#ifdef FOO
		int l;
		int m;
	#else
		int n;
		int o;
	#endif
	int p;
	int q;
	#ifndef FOO
		int r;
		int s;
	#else
		int t;
		int u;
	#endif
	int v;
	int w;
#endif

