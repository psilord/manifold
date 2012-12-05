#ifndef INPUT_H
#define INPUT_H

#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include "symbol.h"

/* This structure creates an input mapping from printable characters to 
	arbitrary(but constant dimension) symbols. When asked for a translation
	or something where a symbol is returned, the symbol is a pointer into 
	static memory, and so must be copied for use. */

typedef struct XlateTable_s
{
	char c;
	Symbol *sym;
} XlateTable;

/* a grouping of a certain number of symbols */
/* WARNING, all symbols must be of the same dimension */
typedef struct Input_s
{
	/* dimension of the symbols returned */
	int dim;

	/* number of printable characters */
	int printable;

	/* ascii character translations for printable characters */
	XlateTable xlt[256];

} Input;

/* default to pefect mode */
Input* input_init(unsigned short dimension);

/* return the symbol associated with the printable character */
Symbol* input_xlate_char(Input *inp, char c);

/* just return a random choice of available printable characters */
Symbol* input_random_choice(Input *inp);

/* return a random choice of 'a' through 'a' + num */
Symbol* input_random_choice_special(Input *inp, int num);

/* free the input */
void input_free(Input *inp);

/* given an unknown symbol, decode what it is closest to of the known 
	input symbols */
Symbol* input_decode_to_sym(Input *inp, Symbol *unk, double *error);
char input_decode_to_char(Input *inp, Symbol *unk, double *error);

#endif



