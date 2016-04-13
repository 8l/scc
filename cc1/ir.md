# Scc intermediate representation #

Scc IR tries to be be a simple and easily parseable intermediate
representation, and it makes it a bit terse and criptic. The main
characteristic of the IR is that all the types and operations are
represented with only one letter, so parsing tables can be used
to parse it.

The language is composed by lines, which represent statements,
and fields in statements are separated by tabulators. Declaration
statements begin in column 0, meanwhile expressions and control
flow begin with a tabulator. When the front end detects an error
it closes the output stream.

## Types ##

Types are represented using upper case letters:

* C -- char
* I -- int
* W -- long
* O -- long long
* M -- unsigned char
* N -- unsigned int
* Z -- unsigned long
* Q -- unsigned long long
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

This list is built for the original Z80 backend, where 'int'
had the same size than 'short'. Several types need an identifier
after the type letter, mainly S, F, V and U, to be able to
differentiate between different structs, functions, vectors and
unions (S1, V12 ...).

## Storage class ##

Storage class is represented using upper case letters:

* A -- automatic
* R -- register
* G -- public (global variable declared in the module)
* X -- extern (global variable declared in another module)
* Y -- private (file scoped variable)
* T -- local (function scopped static variable)
* M -- member (struct/union member)
* L -- label

## Declarations/definitions ##

Variables names are composed by a storage class and an identifier,
A1, R2 or T3. Declarations/definitions are composed by a variable
name, a type and the name of the variable:

	A1	I	i
	R2	C	c
	A3	S4	str

### Type declarations ###

Some declarations need a previous declaration of the types involved
in the variable declaration. In the case of members, they form part
of the last struct or union declared.

For example the next code:

	struct foo {
		int i;
		ong c;
	} var1;

will generate the next output:

	S2	foo
	M3	I	i
	M4	W	c
	G5	S2	var1


## Functions ##

A function prototype like

	int printf(char *cmd);

will generate a type declaration and a variable declaration:

	F3	P
	X6	F3	printf

After the type specification of the function (F and an identifier),
the types of the function parameters are described.
A '{' in the first column begins the body for the previously
declared function: For example:

	int printf(char *cmd) {}

will generate

	F3	P
	G6	F3	printf
	{
	A7	P	cmd
	\
	}

Again, the front end must ensure that '{' appears only after the
declaration of a function. The character '\' marks the separation
between parameters and local variables:

	int printf(register char *cmd) {int i;};

will generate

	F3	P
	G6	F3	printf
	{
	R7	P	cmd
	\
	A8	I	i
	}


### Expressions ###

Expressions are emitted as postorder expressions, making very easy
to parse them and convert them to a tree representation.

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

Constants are introduced by the character '#'. For example 10 is
translated to #IA (all the constants are emitted in hexadecimal),
where I indicates that is an integer constant. Strings represent
a special case because they are represented with the " character.
The constant "hello" is emitted as "68656C6C6F. Example:

	int
	main(void)
	{
		int i, j;
		i = j+2*3;
	}

generates:

	F1
	G1	F1	main
	{
	\
	A2      I	i
	A3      I	j
		A2	A3	#I6	+I	:I
	}

Casting are expressed with the letter 'g' followed of the type
involved in the cast.

### Statements ###
#### Jumps #####

Jumps have the next form:

* j	L?	[expression]

the optional expression field indicates some condition which
must be satisfied to jump. Example:

	int
	main(void)
	{
		int i;
		goto    label;
	label:  i -= i;
	}

generates:

	F1
	G1      F1      main
	{
	\
	A2	I	i
		j	L3
	L3
		A2	A2	:-
	}

Another form of jump is the return statement, which uses the
letter 'r' with an optional expression.
For example:

	int
	main(void)
	{
		return 16;
	}

produces:

	F1
	G1	F1	main
	{
	\
		r	#I10
	}


#### Loops ####

There is a two special characters that are used to indicate
to the backend that the next statements are part of the body
of a loop:

* b -- begin of loop
* e -- end of loop

#### Switch statement ####

Switches are represented using a table, in which the labels
where to jump for each case are indicated. Common cases are
represented by 'v', meanwhile default is represented by 'f'.
The switch statement itself is represented by 's' followed by
the label where the jump table is located, and the expression
of the switch. For example:

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

generates:

	F2	I
	G1	F2	func
	{
	A1	I	n
	\
		s	L4	A1	#I1	+
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


The beginning of the jump table is indicated by the the letter t,
followed by the number of cases (including default case) of the
switch.

## Resumen ##

* C -- char
* I -- int
* W -- long
* O -- long long
* M -- unsigned char
* N -- unsigned int
* Z -- unsigned long
* Q -- unsigned long long
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
* Y -- private (file scoped variable)
* T -- local (function scopped static variable)
* M -- member (struct/union member)
* L -- label
* { -- end of function body
* } -- end of fucntion body
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
* g -- casting
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
