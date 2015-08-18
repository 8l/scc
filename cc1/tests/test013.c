/*
name: TEST013
description: Basic test of integer types and integer conversions
comments: This test depends of the configuration in the type system.
          With the current configuration char is equal to unsigned char,
          short is equal to int, and unsigned short is equal to unsigned.
output:
G1	I	a
G2	N	b
G3	M	c
G4	C	d
G5	M	e
G6	W	f
G7	Z	g
G8	Q	h
G9	O	i
G10	I	j
G11	N	k
F1
G12	F1	main
{
-
	G1	G2	NI	:I
	G1	G3	MI	:I
	G1	G4	CI	:I
	G1	G5	MI	:I
	G1	G6	WI	:I
	G1	G7	ZI	:I
	G1	G8	QI	:I
	G1	G9	OI	:I
	G1	G10	:I
	G1	G11	NI	:I
	G2	G1	IN	:N
	G2	G3	MN	:N
	G2	G4	CN	:N
	G2	G5	MN	:N
	G2	G6	WN	:N
	G2	G7	ZN	:N
	G2	G8	QN	:N
	G2	G9	ON	:N
	G2	G10	IN	:N
	G2	G11	:N
	G3	G1	IM	:M
	G3	G2	NM	:M
	G3	G4	CM	:M
	G3	G5	:M
	G3	G6	WM	:M
	G3	G7	ZM	:M
	G3	G8	QM	:M
	G3	G9	OM	:M
	G3	G10	IM	:M
	G3	G11	NM	:M
	G4	G1	IC	:C
	G4	G2	NC	:C
	G4	G3	MC	:C
	G4	G5	MC	:C
	G4	G6	WC	:C
	G4	G7	ZC	:C
	G4	G8	QC	:C
	G4	G9	OC	:C
	G4	G10	IC	:C
	G4	G11	NC	:C
	G5	G1	IM	:M
	G5	G2	NM	:M
	G5	G3	:M
	G5	G4	CM	:M
	G5	G6	WM	:M
	G5	G7	ZM	:M
	G5	G8	QM	:M
	G5	G9	OM	:M
	G5	G10	IM	:M
	G5	G11	NM	:M
	G6	G1	IW	:W
	G6	G2	NW	:W
	G6	G3	MW	:W
	G6	G4	CW	:W
	G6	G5	MW	:W
	G6	G7	ZW	:W
	G6	G8	QW	:W
	G6	G9	OW	:W
	G6	G10	IW	:W
	G6	G11	NW	:W
	G7	G1	IZ	:Z
	G7	G2	NZ	:Z
	G7	G3	MZ	:Z
	G7	G4	CZ	:Z
	G7	G5	MZ	:Z
	G7	G6	WZ	:Z
	G7	G8	QZ	:Z
	G7	G9	OZ	:Z
	G7	G10	IZ	:Z
	G7	G11	NZ	:Z
	G8	G1	IQ	:Q
	G8	G2	NQ	:Q
	G8	G3	MQ	:Q
	G8	G4	CQ	:Q
	G8	G5	MQ	:Q
	G8	G6	WQ	:Q
	G8	G7	ZQ	:Q
	G8	G9	OQ	:Q
	G8	G10	IQ	:Q
	G8	G11	NQ	:Q
	G9	G1	IO	:O
	G9	G2	NO	:O
	G9	G3	MO	:O
	G9	G4	CO	:O
	G9	G5	MO	:O
	G9	G6	WO	:O
	G9	G7	ZO	:O
	G9	G8	QO	:O
	G9	G10	IO	:O
	G9	G11	NO	:O
	G10	G1	:I
	G10	G2	NI	:I
	G10	G3	MI	:I
	G10	G4	CI	:I
	G10	G5	MI	:I
	G10	G6	WI	:I
	G10	G7	ZI	:I
	G10	G8	QI	:I
	G10	G9	OI	:I
	G10	G11	NI	:I
	G11	G1	IN	:N
	G11	G2	:N
	G11	G3	MN	:N
	G11	G4	CN	:N
	G11	G5	MN	:N
	G11	G6	WN	:N
	G11	G7	ZN	:N
	G11	G8	QN	:N
	G11	G10	IN	:N
	G11	G9	ON	:N
}
*/

int a;
unsigned b;
char c;
signed char d;
unsigned char e;
long f;
unsigned long g;
long long h;
unsigned long long i;
short j;
unsigned short k;

int
main(void)
{
	a = b;
	a = c;
	a = d;
	a = e;
	a = f;
	a = g;
	a = h;
	a = i;
	a = j;
	a = k;

	b = a;
	b = c;
	b = d;
	b = e;
	b = f;
	b = g;
	b = h;
	b = i;
	b = j;
	b = k;

	c = a;
	c = b;
	c = d;
	c = e;
	c = f;
	c = g;
	c = h;
	c = i;
	c = j;
	c = k;

	d = a;
	d = b;
	d = c;
	d = e;
	d = f;
	d = g;
	d = h;
	d = i;
	d = j;
	d = k;

	e = a;
	e = b;
	e = c;
	e = d;
	e = f;
	e = g;
	e = h;
	e = i;
	e = j;
	e = k;

	f = a;
	f = b;
	f = c;
	f = d;
	f = e;
	f = g;
	f = h;
	f = i;
	f = j;
	f = k;

	g = a;
	g = b;
	g = c;
	g = d;
	g = e;
	g = f;
	g = h;
	g = i;
	g = j;
	g = k;

	h = a;
	h = b;
	h = c;
	h = d;
	h = e;
	h = f;
	h = g;
	h = i;
	h = j;
	h = k;

	i = a;
	i = b;
	i = c;
	i = d;
	i = e;
	i = f;
	i = g;
	i = h;
	i = j;
	i = k;

	j = a;
	j = b;
	j = c;
	j = d;
	j = e;
	j = f;
	j = g;
	j = h;
	j = i;
	j = k;

	k = a;
	k = b;
	k = c;
	k = d;
	k = e;
	k = f;
	k = g;
	k = h;
	k = j;
	k = i;
}
