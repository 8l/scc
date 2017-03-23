/* See LICENSE file for copyright and license details. */

#define STDC_VERSION    "199409L"

/* Translation limits */
/*
 * 15 nesting levels of compound statements, iteration control
 * structures, and selection control structures
 */
#define NR_BLOCK        15
/*
 * 8 nesting levels of conditional inclusion
 */
#define NR_COND         8
/*
 * 12 pointer, array, and function declarators (in any combinations)
 * modifying an arithmetic, a structure, a union, or an incomplete type
 * in a declaration
 */
#define NR_DECLARATORS  12
/*
 * 31 declarators nested by parentheses within a full declarator
 */
#define NR_SUBTYPE      31
/*
 * 32 expressions nested by parentheses within a full expression
 */
#define NR_SUBEXPR      32
/*
 * 31 significant initial characters in an internal identifier or a
 * macro name
 */
#define INTIDENTSIZ     31
/*
 * 6 significant initial characters in an external identifier
 */
#define EXTIDENTSIZ     6
/*
 * 511 external identifiers in one translation unit
 */
#define NR_EXT_IDENT    511
/*
 * 127 identifiers with block scope declared in one block
 */
#define NR_INT_IDENT    127
/*
 * 1024 macro identifiers simultaneously defined in one translation
 * unit
 */
#define NR_MACROIDENT   1024
/*
 * 31 parameters in one function definition
 */
#define NR_FUNPARAM     31
/*
 * 31 arguments in one function call
 */
#define NR_FUNARG       31
/*
 * 31 parameters in one macro definition
 */
#define NR_MACROPARAM   31
/*
 * 31 arguments in one macro invocation
 */
#define NR_MACROARG     31
/*
 * 509 characters in a logical source line
 */
#define LINESIZ         509
/*
 * 509 characters in a character string literal or wide string literal
 * (after concatenation)
 */
#define STRINGSIZ       509
/*
 * 32767 bytes in an object (in a hosted environment only)
 */
#define OBJECTSIZ       32767
/*
 * 8 nesting levels for #include'd files
 */
#define NR_INCLUDE      8
/*
 * 257 case labels for a switch statement (excluding those for any
 * nested switch statements)
 */
#define NR_SWITCH       257
/*
 * 127 members in a single structure or union
 */
#define NR_FIELDS       127
/*
 * 127 enumeration constants in a single enumeration
 */
#define NR_ENUM_CTES    127
/*
 * 15 levels of nested structure or union definitions in a single
 * struct-declaration-list
 */
#define NR_STRUCT_LEVEL 15
/*
 * number of defined structs/unions in one translation unit
 */
#define NR_MAXSTRUCTS   127
