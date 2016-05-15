/*
 * 127 nesting levels of blocks
 */
#define NR_BLOCK        127
/*
 * 63 nesting levels of conditional inclusion
 */
#define NR_COND         63
/*
 * 12 pointer, array, and function declarators (in any combinations)
 * modifying an arithmetic, a structure, a union, or an incomplete type
 * in a declaration
 */
#define NR_DECLARATORS  12
/*
 * 63 nesting levels of parenthesized declarators within a full
 * declarator
 */
#define NR_SUBTYPE      63
/*
 * 63 nesting levels of parenthesized expressions within a full
 * expression
 */
#define NR_SUBEXPR      63
/*
 * 63 significant initial characters in an internal identifier or a
 * macro name (each universal character name or extended source
 * character is considered a single character)
 */
#define INTIDENTSIZ     63
/*
 * 31 significant initial characters in an external identifier (each
 * universal character name specifying a short identifier of 0000FFFF
 * or less is considered 6 characters, each universal character name
 * specifying a short identifier of 00010000 or more is considered 10
 * characters, and each extended source character is considered the
 * same number of characters as the corresponding universal character
 * name, if any)
 */
#define EXTIDENTSIZ     31
/*
 * 4095 external identifiers in one translation unit
 */
#define NR_EXT_IDENT    4095
/*
 * 511 identifiers with block scope declared in one block
 */
#define NR_INT_IDENT    511
/*
 * 4096 macro identifiers simultaneously defined in one preprocessing
 * translation unit
 */
#define NR_MACROIDENT   4096
/*
 * 127 parameters in one function definition
 */
#define NR_FUNPARAM     127
/*
 * 127 arguments in one function call
 */
#define NR_FUNARG       127
/*
 * 127 parameters in one macro definition
 */
#define NR_MACROPARAM   127
/*
 * 127 arguments in one macro invocation
 */
#define NR_MACROARG     127
/*
 * 4095 characters in a logical source line
 */
#define LINESIZ         4095
/*
 * 4095 characters in a character string literal or wide string literal
 * (after concatenation)
 */
#define STRINGSIZ       4095
/*
 * 65535 bytes in an object (in a hosted environment only)
 */
#define OBJECTSIZ       65535
/*
 * 15 nesting levels for #include'd files
 */
#define NR_INCLUDE      15
/*
 * 1023 case labels for a switch statement (excluding those for any
 * nested switch statements)
 */
#define NR_SWITCH       1023
/*
 * 1023 members in a single structure or union
 */
#define NR_FIELDS       1023
/*
 * 1023 enumeration constants in a single enumeration
 */
#define NR_ENUM_CTES    1023
/*
 * 63 levels of nested structure or union definitions in a single
 * struct-declaration-list
 */
#define NR_STRUCT_LEVEL 63
/*
 * number of defined structs/unions in one translation unit
 */
#define NR_MAXSTRUCTS   127
