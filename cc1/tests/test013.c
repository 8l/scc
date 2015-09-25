/*
name: TEST013
description: Basic test of integer types and integer conversions
comments: This test depends of the configuration in the type system.
          With the current configuration char is equal to unsigned char,
          short is equal to int, and unsigned short is equal to unsigned.
output:
G1	I	a
G2	N	b
G3	K	c
G4	C	d
G5	K	e
G6	W	f
G7	Z	g
G8	Q	h
G9	O	i
G10	I	j
G11	N	k
F12	I
G13	F12	main
{
\
	G1	G2	gN	:I
	G1	G3	gK	:I
	G1	G4	gC	:I
	G1	G5	gK	:I
	G1	G6	gW	:I
	G1	G7	gZ	:I
	G1	G8	gQ	:I
	G1	G9	gO	:I
	G1	G10	:I
	G1	G11	gN	:I
	G2	G1	gI	:N
	G2	G3	gK	:N
	G2	G4	gC	:N
	G2	G5	gK	:N
	G2	G6	gW	:N
	G2	G7	gZ	:N
	G2	G8	gQ	:N
	G2	G9	gO	:N
	G2	G10	gI	:N
	G2	G11	:N
	G3	G1	gI	:K
	G3	G2	gN	:K
	G3	G4	gC	:K
	G3	G5	:K
	G3	G6	gW	:K
	G3	G7	gZ	:K
	G3	G8	gQ	:K
	G3	G9	gO	:K
	G3	G10	gI	:K
	G3	G11	gN	:K
	G4	G1	gI	:C
	G4	G2	gN	:C
	G4	G3	gK	:C
	G4	G5	gK	:C
	G4	G6	gW	:C
	G4	G7	gZ	:C
	G4	G8	gQ	:C
	G4	G9	gO	:C
	G4	G10	gI	:C
	G4	G11	gN	:C
	G5	G1	gI	:K
	G5	G2	gN	:K
	G5	G3	:K
	G5	G4	gC	:K
	G5	G6	gW	:K
	G5	G7	gZ	:K
	G5	G8	gQ	:K
	G5	G9	gO	:K
	G5	G10	gI	:K
	G5	G11	gN	:K
	G6	G1	gI	:W
	G6	G2	gN	:W
	G6	G3	gK	:W
	G6	G4	gC	:W
	G6	G5	gK	:W
	G6	G7	gZ	:W
	G6	G8	gQ	:W
	G6	G9	gO	:W
	G6	G10	gI	:W
	G6	G11	gN	:W
	G7	G1	gI	:Z
	G7	G2	gN	:Z
	G7	G3	gK	:Z
	G7	G4	gC	:Z
	G7	G5	gK	:Z
	G7	G6	gW	:Z
	G7	G8	gQ	:Z
	G7	G9	gO	:Z
	G7	G10	gI	:Z
	G7	G11	gN	:Z
	G8	G1	gI	:Q
	G8	G2	gN	:Q
	G8	G3	gK	:Q
	G8	G4	gC	:Q
	G8	G5	gK	:Q
	G8	G6	gW	:Q
	G8	G7	gZ	:Q
	G8	G9	gO	:Q
	G8	G10	gI	:Q
	G8	G11	gN	:Q
	G9	G1	gI	:O
	G9	G2	gN	:O
	G9	G3	gK	:O
	G9	G4	gC	:O
	G9	G5	gK	:O
	G9	G6	gW	:O
	G9	G7	gZ	:O
	G9	G8	gQ	:O
	G9	G10	gI	:O
	G9	G11	gN	:O
	G10	G1	:I
	G10	G2	gN	:I
	G10	G3	gK	:I
	G10	G4	gC	:I
	G10	G5	gK	:I
	G10	G6	gW	:I
	G10	G7	gZ	:I
	G10	G8	gQ	:I
	G10	G9	gO	:I
	G10	G11	gN	:I
	G11	G1	gI	:N
	G11	G2	:N
	G11	G3	gK	:N
	G11	G4	gC	:N
	G11	G5	gK	:N
	G11	G6	gW	:N
	G11	G7	gZ	:N
	G11	G8	gQ	:N
	G11	G10	gI	:N
	G11	G9	gO	:N
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
