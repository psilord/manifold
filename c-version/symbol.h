#ifndef SYMBOL_H
#define SYMBOL_H

#include <stdio.h>
#include <stdlib.h>
#include <math.h>

/* the symbol that is both representing the input and the neuron, for now... */
typedef struct Symbol_s
{
	/* maximum representable dimension of 65535 */
	unsigned short dim;
	/* this is a stretchy array style structure */
	float vec[1];
} Symbol;

/* malloc a symbol for me with associated vector */
Symbol* symbol_init(unsigned short dimension);

/* XXX these functions assume that the dimension of the symbols being operated 
	on are the same */

/* find the _fast_ distance between two Symbols */
float symbol_fdist(Symbol *a, Symbol *b);

/* find the correct distance between the two Symbols */
float symbol_dist(Symbol *a, Symbol *b);

/* make symbol a more like symbol b in a continuous and linear fashion
	based upon t which goes from 0 to 1 */
void symbol_interpolate(Symbol *a, Symbol *b, float t);

/* make the symbol the "zero" symbol */
void symbol_zero(Symbol *sym);

/* make the symbol the "one" symbol */
void symbol_one(Symbol *sym);

/* fill the symbol with random data */
void symbol_randomize(Symbol *sym);

/* set every member of the symbol to the passed in floating point value */
void symbol_set_all(Symbol *sym, float val);

/* copy the symbol into new memory */
Symbol* symbol_copy(Symbol *sym);

/* copy a symbol into a preexisting symbol */
void symbol_move(Symbol *dest, Symbol *source);

/* get the dimension of the symbol */
unsigned short symbol_get_dim(Symbol *sym);

/* add one symbol to another. Also, if you use a single symbol and place its 
	pointer into all three paces in this function, it is valid, and merely 
	adds the symbol to itself. */
void symbol_add(Symbol *out, Symbol *lhs, Symbol *rhs);

/* multiply each element in the symbol by a value */
void symbol_mul(Symbol *sym, float num);

/* divide each element in the symbol by a value */
void symbol_div(Symbol *sym, float num);

/* print out a symbol */
void symbol_stdout(Symbol *sym);

/* set a symbol to be what in is the vals array, num_vals must be equal to
	the dimension of the symbol */
void symbol_set(Symbol *sym, float *vals, int num_vals);

/* hard coded sets for various needs */
void symbol_set_1(Symbol *sym, float v0);
void symbol_set_2(Symbol *sym, float v0, float v1);
void symbol_set_3(Symbol *sym, float v0, float v1, float v2);
void symbol_set_4(Symbol *sym, float v0, float v1, float v2, float v3);
void symbol_set_5(Symbol *sym, float v0, float v1, float v2, float v3, 
	float v4);
void symbol_set_6(Symbol *sym, float v0, float v1, float v2, float v3,
	float v4, float v5);

/* set a slot in the symbol specifically */
void symbol_set_index(Symbol *sym, float val, int index);

/* fill in the vals array with what is contained in the symbol */
void symbol_get(Symbol *sym, float *vals, int num_vals);

/* hard coded sets for various needs */
void symbol_get_1(Symbol *sym, float *v0);
void symbol_get_2(Symbol *sym, float *v0, float *v1);
void symbol_get_3(Symbol *sym, float *v0, float *v1, float *v2);
void symbol_get_4(Symbol *sym, float *v0, float *v1, float *v2, float *v3);
void symbol_get_5(Symbol *sym, float *v0, float *v1, float *v2, float *v3,
	float *v4);
void symbol_get_6(Symbol *sym, float *v0, float *v1, float *v2, float *v3,
	float *v4, float *v5);

/* retrieve the value from a specific slot in a symbol */
void symbol_get_index(Symbol *sym, float *val, int index);

/* join some symbols together to form a new symbol, the permution of the
	list matters, meaning if the same members of the list are given but in
	a different order, the target would be considered a different symbol
	unless the merging symbols are all the same. The dimensionality of the
	target will be the addition of all dimensionalities of the supplied
	symbols in the list */
Symbol* symbol_abstract(Symbol **list, int num);

/* return an array of symbols pointers pointing to symbols matching the 
	unabstraction specification. for example, if I have a 5
	dimensional symbol, then the dim array could be [2,2,1] in
	which case I'll get back a 2 dimensional symbol, another 2
	dimensional symbol, and a 1 dimensional symbol in the list in that
	order. The unabstraction happens in order, meaning 2 dimensions
	are stripped of starting from sym->vec[0] to sym->vec[1], then
	2 more dimensions from sym->vec[2] to sym->vec[3], then the last
	dimension of sym->vec[4]. The caller already knows the number of
	symbols returned in the list because they know num_dims. Also,
	the array and the symbols contained in the array are malloced
	memory and must be freed by the caller. */
Symbol** symbol_unabstract(Symbol *sym, int *dim, int num_dims);

void symbol_free(Symbol *sym);

#endif





