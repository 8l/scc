/* See LICENSE file for copyright and license details. */

/*
name: TEST013
description: Basic test of integer types and integer conversions
comments: This test depends of the configuration in the type system.
          With the current configuration char is equal to unsigned char,
          short is equal to int, and unsigned short is equal to unsigned.
error:
output:
G1	I	"a
G2	N	"b
G3	K	"c
G4	C	"d
G5	K	"e
G6	W	"f
G7	Z	"g
G8	Q	"h
G9	O	"i
G10	I	"j
G11	N	"k
G13	I	F	"main
{
\
	G1	G2	gI	:I
	G1	G3	gI	:I
	G1	G4	gI	:I
	G1	G5	gI	:I
	G1	G6	gI	:I
	G1	G7	gI	:I
	G1	G8	gI	:I
	G1	G9	gI	:I
	G1	G10	:I
	G1	G11	gI	:I
	G2	G1	gN	:N
	G2	G3	gN	:N
	G2	G4	gN	:N
	G2	G5	gN	:N
	G2	G6	gN	:N
	G2	G7	gN	:N
	G2	G8	gN	:N
	G2	G9	gN	:N
	G2	G10	gN	:N
	G2	G11	:N
	G3	G1	gK	:K
	G3	G2	gK	:K
	G3	G4	gK	:K
	G3	G5	:K
	G3	G6	gK	:K
	G3	G7	gK	:K
	G3	G8	gK	:K
	G3	G9	gK	:K
	G3	G10	gK	:K
	G3	G11	gK	:K
	G4	G1	gC	:C
	G4	G2	gC	:C
	G4	G3	gC	:C
	G4	G5	gC	:C
	G4	G6	gC	:C
	G4	G7	gC	:C
	G4	G8	gC	:C
	G4	G9	gC	:C
	G4	G10	gC	:C
	G4	G11	gC	:C
	G5	G1	gK	:K
	G5	G2	gK	:K
	G5	G3	:K
	G5	G4	gK	:K
	G5	G6	gK	:K
	G5	G7	gK	:K
	G5	G8	gK	:K
	G5	G9	gK	:K
	G5	G10	gK	:K
	G5	G11	gK	:K
	G6	G1	gW	:W
	G6	G2	gW	:W
	G6	G3	gW	:W
	G6	G4	gW	:W
	G6	G5	gW	:W
	G6	G7	gW	:W
	G6	G8	gW	:W
	G6	G9	gW	:W
	G6	G10	gW	:W
	G6	G11	gW	:W
	G7	G1	gZ	:Z
	G7	G2	gZ	:Z
	G7	G3	gZ	:Z
	G7	G4	gZ	:Z
	G7	G5	gZ	:Z
	G7	G6	gZ	:Z
	G7	G8	gZ	:Z
	G7	G9	gZ	:Z
	G7	G10	gZ	:Z
	G7	G11	gZ	:Z
	G8	G1	gQ	:Q
	G8	G2	gQ	:Q
	G8	G3	gQ	:Q
	G8	G4	gQ	:Q
	G8	G5	gQ	:Q
	G8	G6	gQ	:Q
	G8	G7	gQ	:Q
	G8	G9	gQ	:Q
	G8	G10	gQ	:Q
	G8	G11	gQ	:Q
	G9	G1	gO	:O
	G9	G2	gO	:O
	G9	G3	gO	:O
	G9	G4	gO	:O
	G9	G5	gO	:O
	G9	G6	gO	:O
	G9	G7	gO	:O
	G9	G8	gO	:O
	G9	G10	gO	:O
	G9	G11	gO	:O
	G10	G1	:I
	G10	G2	gI	:I
	G10	G3	gI	:I
	G10	G4	gI	:I
	G10	G5	gI	:I
	G10	G6	gI	:I
	G10	G7	gI	:I
	G10	G8	gI	:I
	G10	G9	gI	:I
	G10	G11	gI	:I
	G11	G1	gN	:N
	G11	G2	:N
	G11	G3	gN	:N
	G11	G4	gN	:N
	G11	G5	gN	:N
	G11	G6	gN	:N
	G11	G7	gN	:N
	G11	G8	gN	:N
	G11	G10	gN	:N
	G11	G9	gN	:N
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
