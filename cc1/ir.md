# scc intermediate representation #

The scc IR tries to be be a simple and easily parseable intermediate
representation, and it makes it a bit terse and cryptic. The main
characteristic of the IR is that all the types and operations are
represented with only one letter, so parsing tables can be used
to parse it.

The language is composed of lines, representing statements.
Each statement is composed of tab-separated fields.
Declaration statements begin in column 0, expressions and
control flow begin with a tabulator.
When the frontend detects an error, it closes the output stream.

## Types ##

Types are represented with uppercase letters:

* C -- signed    8-Bit integer
* I -- signed   16-Bit integer
* W -- signed   32-Bit integer
* Q -- signed   64-Bit integer
* K -- unsigned  8-Bit integer
* N -- unsigned 16-Bit integer
* Z -- unsigned 32-Bit integer
* O -- unsigned 64-Bit integer
* 0 -- void
* P -- pointer
* F -- function
* V -- vector
* U -- union
* S -- struct
* B -- bool
* J -- float
* D -- double
* H -- long double

This list has been built for the original Z80 backend, where 'int'
has the same size as 'short'. Several types (S, F, V, U and others) need
an identifier after the type letter for better differentiation
between multiple structs, functions, vectors and unions (S1, V12 ...)
naturally occuring in a C-program.

## Storage classes ##

The storage classes are represented using uppercase letters:

* A -- automatic
* R -- register
* G -- public (global variable declared in the module)
* X -- extern (global variable declared in another module)
* Y -- private (variable in file-scope)
* T -- local (static variable in function-scope)
* M -- member (struct/union member)
* L -- label

## Declarations/definitions ##

Variable names are composed of a storage class and an identifier
(e.g. A1, R2, T3).
Declarations and definitions are composed of a variable
name, a type and the name of the variable:

	A1	I	maxweight
	R2	C	flag
	A3	S4	statstruct

### Type declarations ###

Some declarations (e.g. structs) involve the declaration of member
variables.
Struct members are declared normally after the type declaration in
parentheses.

For example the struct declaration

	struct foo {
		int i;
		long c;
	} var1;

generates

	S2      foo     (
	M3      I       i
	M4      W       c
	)
	G5      S2      var1

## Functions ##

A function prototype

	int printf(char *cmd, int flag, void *data);

will generate a type declaration and a variable declaration

	F5	P	I	P
	X1	F5	printf

The first line gives the function-type specification 'F' with
an identifier '5' and subsequently lists the types of the
function parameters.
The second line declares the 'printf' function as a publicly
scoped variable.

Analogously, a statically declared function in file scope

	static int printf(char *cmd, int flag, void *data);

generates

	F5      P       I       P
	T1      F5      printf

Thus, the 'printf' variable  went into local scope ('T').

A '{' in the first column starts the body of the previously
declared function:

	int printf(char *cmd, int flag, void *data) {}

generates

	F5      P       I       P
	G1      F5      printf
	{
	A2      P       cmd
	A3      I       flag
	A4      P       data
	-
	}

Again, the frontend must ensure that '{' appears only after the
declaration of a function. The character '-' marks the separation
between parameters and local variables:

	int printf(register char *cmd, int flag, void *data) {int i;};

generates

	F5      P       I       P
	G1      F5      printf
	{
	R2      P       cmd
	A3      I       flag
	A4      P       data
	-
	A6      I       i
	}

### Expressions ###

Expressions are emitted in reverse polish notation, simplifying
parsing and converting into a tree representation.

#### Operators ####

Operators allowed in expressions are:

* \+ -- addition
* \- -- substraction
* \* -- multiplication
* % -- modulo
* / -- division
* l -- left shift
* r -- right shift
* < -- less than
* > -- greather than
* ] -- greather or equal than
* [ -- less or equal than
* = -- equal than
* ! -- different than
* & -- bitwise and
* | -- bitwise or
* ^ -- bitwise xor
* ~ -- bitwise complement
* : -- asignation
* _ -- unary negation
* c -- function call
* p -- parameter
* . -- field
* , -- comma operator
* ? -- ternary operator
* ' -- take address
* a -- logical shortcut and
* o -- logical shortcut or
* @ -- content of pointer

Assignation has some suboperators:

* :/ -- divide and assign
* :% -- modulo and assign
* :+ -- addition and assign
* :- -- substraction and assign
* :l -- left shift and assign
* :r -- right shift and assign
* :& -- bitwise and and assign
* :^ -- bitwise xor and assign
* :| -- bitwise or and assign
* :i -- post increment
* :d -- post decrement

Every operator in an expression has a type descriptor.

#### Constants ####

Constants are introduced with the character '#'. For instance, 10 is
translated to #IA (all constants are emitted in hexadecimal),
where I indicates that it is an integer constant.
Strings are a special case because they are represented with
the " character.
The constant "hello" is emitted as "68656C6C6F. For example

	int
	main(void)
	{
		int i, j;

		i = j+2*3;
	}

generates

	F1
	G1	F1	main
	{
	-
	A2      I	i
	A3      I	j
		A2	A3	#I6	+I	:I
	}

Type casts are expressed with a tuple denoting the
type conversion

        int
	main(void)
	{
		int i;
		long j;

		j = (long)i;
	}

generates

	F1
	G1      F1      main
	{
	-
	A2      I       i
	A3      W       j
	        A2      A3      WI      :I
	}

### Statements ###
#### Jumps #####

Jumps have the following form:

	j	L#	[expression]

the optional expression field indicates some condition which
must be satisfied to jump. Example:

	int
	main(void)
	{
		int i;

		goto    label;
	label:
		i -= i;
	}

generates

	F1
	G1      F1      main
	{
	-
	A2	I	i
		j	L3
	L3
		A2	A2	:-I
	}

Another form of jump is the return statement, which uses the
letter 'y' followed by a type identifier.
Depending on the type, an optional expression follows.

	int
	main(void)
	{
		return 16;
	}

generates

	F1
	G1	F1	main
	{
	-
		yI	#I10
	}


#### Loops ####

There are two special characters that are used to indicate
to the backend that the following statements are part of
a loop body.

* b -- beginning of loop
* e -- end of loop

#### Switch statement ####

Switches are represented using a table, in which the labels
where to jump for each case are indicated. Common cases are
represented with 'v' and default with 'f'.
The switch statement itself is represented with 's' followed
by the label where the jump table is located, and the
expression of the switch:

	int
	func(int n)
	{
		switch (n+1) {
		case 1:
		case 2:
		case 3:
		default:
			++n;
		}
	}

generates

	F2	I
	G1	F2	func
	{
	A1	I	n
	-
		s	L4	A1	#I1	+I
	L5
	L6
	L7
	L8
		A1	#I1	:+I
		j	L3
	L4
		t	#4
		v	L7	#I3
		v	L6	#I2
		v	L5	#I1
		f	L8
	L3
	}

The beginning of the jump table is indicated by the the letter 't',
followed by the number of cases (including default case) of the
switch.

## Resumen ##

* C -- signed    8-Bit integer
* I -- signed   16-Bit integer
* W -- signed   32-Bit integer
* O -- signed   64-Bit integer
* M -- unsigned  8-Bit integer
* N -- unsigned 16-Bit integer
* Z -- unsigned 32-Bit integer
* Q -- unsigned 64-Bit integer
* 0 -- void
* P -- pointer
* F -- function
* V -- vector
* U -- union
* S -- struct
* B -- bool
* J -- float
* D -- double
* H -- long double
* A -- automatic
* R -- register
* G -- public (global variable declared in the module)
* X -- extern (global variable declared in another module)
* Y -- private (variable in file-scope)
* T -- local (static variable in function-scope)
* M -- member (struct/union member)
* L -- label
* { -- beginning of function body
* } -- end of function body
* \\ -- end of function parameters
* \+ -- addition
* \- -- substraction
* \* -- multiplication
* % -- modulo
* / -- division
* l -- left shift
* r -- right shift
* < -- less than
* > -- greather than
* ] -- greather or equal than
* [ -- less or equal than
* = -- equal than
* ! -- different than
* & -- bitwise and
* | -- bitwise or
* ^ -- bitwise xor
* ~ -- bitwise complement
* : -- asignation
* _ -- unary negation
* c -- function call
* p -- parameter
* . -- field
* , -- comma operator
* ? -- ternary operator
* ' -- take address
* a -- logical shortcut and
* o -- logical shortcut or
* @ -- content of pointer
* :/ -- divide and assign
* :% -- modulo and assign
* :+ -- addition and assign
* :- -- substraction and assign
* :l -- left shift and assign
* :r -- right shift and assign
* :& -- bitwise and and assign
* :^ -- bitwise xor and assign
* :| -- bitwise or and assign
* ;+ -- post increment
* ;- -- post decrement
* j -- jump
* y -- return
* b -- begin of loop
* d -- end of loop
* s -- switch statement
* t -- switch table
* v -- case entry in switch table
* f -- default entry in switch table
