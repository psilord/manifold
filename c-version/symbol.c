#include <stdio.h>
#include <stdlib.h>
#include "common.h"

Symbol* symbol_init(unsigned short dimension)
{
	Symbol *sym;


	/* get the header and then the additional floats... I use minus one
		on the dimension since one float is already included in the Symbol
		size */
	sym = (Symbol*)xmalloc(sizeof(Symbol) + (sizeof(float) * (dimension - 1)));

	sym->dim = dimension;

	symbol_zero(sym);

	return sym;
}

void symbol_zero(Symbol *sym)
{
	int i;

	for (i = 0; i < sym->dim; i++)
	{
		sym->vec[i] = 0.0;
	}
}

void symbol_one(Symbol *sym)
{
	int i;

	for (i = 0; i < sym->dim; i++)
	{
		sym->vec[i] = 1.0;
	}
}

void symbol_set_all(Symbol *sym, float val)
{
	int i;

	for (i = 0; i < sym->dim; i++)
	{
		sym->vec[i] = val;
	}
}

Symbol* symbol_copy(Symbol *sym)
{
	int i;
	Symbol *newsym;

	newsym = symbol_init(sym->dim);

	for (i = 0; i < sym->dim; i++)
	{
		newsym->vec[i] = sym->vec[i];
	}
	
	return newsym;
}


/* return the faster distance from a to b */
float symbol_fdist(Symbol *a, Symbol *b)
{
	float sum = 0;
	float tmp;
	int i;

	/* removed to get rid of a branch in a function that's always used in
		a tight loop */
	/*
	if (a->dim != b->dim)
	{
		printf("symbol_fdist(): invalid dimensions %d != %d\n",
			a->dim, b->dim);
		exit(EXIT_FAILURE);
	}
	*/

	/* optimize calculating the dist for cases of 8 dimensions or less.
		If I want to undo this, just use the default case instead.
	*/
	switch(a->dim)
	{
		// done this way to make cache hits better
		case 8:
			tmp = b->vec[0] - a->vec[0];
			sum += tmp*tmp;
			tmp = b->vec[1] - a->vec[1];
			sum += tmp*tmp;
			tmp = b->vec[2] - a->vec[2];
			sum += tmp*tmp;
			tmp = b->vec[3] - a->vec[3];
			sum += tmp*tmp;
			tmp = b->vec[4] - a->vec[4];
			sum += tmp*tmp;
			tmp = b->vec[5] - a->vec[5];
			sum += tmp*tmp;
			tmp = b->vec[6] - a->vec[6];
			sum += tmp*tmp;
			tmp = b->vec[7] - a->vec[7];
			sum += tmp*tmp;
			break;

		case 7:
			tmp = b->vec[0] - a->vec[0];
			sum += tmp*tmp;
			tmp = b->vec[1] - a->vec[1];
			sum += tmp*tmp;
			tmp = b->vec[2] - a->vec[2];
			sum += tmp*tmp;
			tmp = b->vec[3] - a->vec[3];
			sum += tmp*tmp;
			tmp = b->vec[4] - a->vec[4];
			sum += tmp*tmp;
			tmp = b->vec[5] - a->vec[5];
			sum += tmp*tmp;
			tmp = b->vec[6] - a->vec[6];
			sum += tmp*tmp;
			break;

		case 6:
			tmp = b->vec[0] - a->vec[0];
			sum += tmp*tmp;
			tmp = b->vec[1] - a->vec[1];
			sum += tmp*tmp;
			tmp = b->vec[2] - a->vec[2];
			sum += tmp*tmp;
			tmp = b->vec[3] - a->vec[3];
			sum += tmp*tmp;
			tmp = b->vec[4] - a->vec[4];
			sum += tmp*tmp;
			tmp = b->vec[5] - a->vec[5];
			sum += tmp*tmp;
			break;

		case 5:
			tmp = b->vec[0] - a->vec[0];
			sum += tmp*tmp;
			tmp = b->vec[1] - a->vec[1];
			sum += tmp*tmp;
			tmp = b->vec[2] - a->vec[2];
			sum += tmp*tmp;
			tmp = b->vec[3] - a->vec[3];
			sum += tmp*tmp;
			tmp = b->vec[4] - a->vec[4];
			sum += tmp*tmp;
			break;

		case 4:
			tmp = b->vec[0] - a->vec[0];
			sum += tmp*tmp;
			tmp = b->vec[1] - a->vec[1];
			sum += tmp*tmp;
			tmp = b->vec[2] - a->vec[2];
			sum += tmp*tmp;
			tmp = b->vec[3] - a->vec[3];
			sum += tmp*tmp;
			break;

		case 3:
			tmp = b->vec[0] - a->vec[0];
			sum += tmp*tmp;
			tmp = b->vec[1] - a->vec[1];
			sum += tmp*tmp;
			tmp = b->vec[2] - a->vec[2];
			sum += tmp*tmp;
			break;

		case 2:
			tmp = b->vec[0] - a->vec[0];
			sum += tmp*tmp;
			tmp = b->vec[1] - a->vec[1];
			sum += tmp*tmp;
			break;

		case 1:
			tmp = b->vec[0] - a->vec[0];
			sum += tmp*tmp;
			break;

		default:
			for(i = 0; i < a->dim; i++)
			{
				tmp = b->vec[i] - a->vec[i];
				sum += tmp*tmp;
			}
			break;
	}
	return sum;
}

/* return the correct distance from a to b */
float symbol_dist(Symbol *a, Symbol *b)
{
	float sum = 0;
	float tmp;
	int i;

	if (a->dim != b->dim)
	{
		printf("symbol_dist(): invalid dimensions: a[%d] b[%d]\n",
			a->dim, b->dim);
		exit(EXIT_FAILURE);
	}

	for(i = 0; i < a->dim; i++)
	{
		tmp = b->vec[i] - a->vec[i];
		sum += tmp*tmp;
	}
	return sqrt(sum);
}

/* make a more like b by the amount of t (from 0 to 1) */
void symbol_interpolate(Symbol *a, Symbol *b, float t)
{
	int i;

	/* removed because this is a highly called function */
	/*
	if (a->dim != b->dim)
	{
		printf("symbol_interpolate(): invalid dimensions\n");
		exit(EXIT_FAILURE);
	}
	*/

	/* do some optimization for up to 8 dimensional interpolations.
	 	If I wanted to undo this optimization, just do the default
		case.
	*/
	switch(a->dim)
	{
		/* done this way to increase cache hits */
		case 8:
			a->vec[0] = (a->vec[0] * (1.0 - t)) + (b->vec[0] * t);
			a->vec[1] = (a->vec[1] * (1.0 - t)) + (b->vec[1] * t);
			a->vec[2] = (a->vec[2] * (1.0 - t)) + (b->vec[2] * t);
			a->vec[3] = (a->vec[3] * (1.0 - t)) + (b->vec[3] * t);
			a->vec[4] = (a->vec[4] * (1.0 - t)) + (b->vec[4] * t);
			a->vec[5] = (a->vec[5] * (1.0 - t)) + (b->vec[5] * t);
			a->vec[6] = (a->vec[6] * (1.0 - t)) + (b->vec[6] * t);
			a->vec[7] = (a->vec[7] * (1.0 - t)) + (b->vec[7] * t);
			break;

		case 7:
			a->vec[0] = (a->vec[0] * (1.0 - t)) + (b->vec[0] * t);
			a->vec[1] = (a->vec[1] * (1.0 - t)) + (b->vec[1] * t);
			a->vec[2] = (a->vec[2] * (1.0 - t)) + (b->vec[2] * t);
			a->vec[3] = (a->vec[3] * (1.0 - t)) + (b->vec[3] * t);
			a->vec[4] = (a->vec[4] * (1.0 - t)) + (b->vec[4] * t);
			a->vec[5] = (a->vec[5] * (1.0 - t)) + (b->vec[5] * t);
			a->vec[6] = (a->vec[6] * (1.0 - t)) + (b->vec[6] * t);
			break;

		case 6:
			a->vec[0] = (a->vec[0] * (1.0 - t)) + (b->vec[0] * t);
			a->vec[1] = (a->vec[1] * (1.0 - t)) + (b->vec[1] * t);
			a->vec[2] = (a->vec[2] * (1.0 - t)) + (b->vec[2] * t);
			a->vec[3] = (a->vec[3] * (1.0 - t)) + (b->vec[3] * t);
			a->vec[4] = (a->vec[4] * (1.0 - t)) + (b->vec[4] * t);
			a->vec[5] = (a->vec[5] * (1.0 - t)) + (b->vec[5] * t);
			break;

		case 5:
			a->vec[0] = (a->vec[0] * (1.0 - t)) + (b->vec[0] * t);
			a->vec[1] = (a->vec[1] * (1.0 - t)) + (b->vec[1] * t);
			a->vec[2] = (a->vec[2] * (1.0 - t)) + (b->vec[2] * t);
			a->vec[3] = (a->vec[3] * (1.0 - t)) + (b->vec[3] * t);
			a->vec[4] = (a->vec[4] * (1.0 - t)) + (b->vec[4] * t);
			break;

		case 4:
			a->vec[0] = (a->vec[0] * (1.0 - t)) + (b->vec[0] * t);
			a->vec[1] = (a->vec[1] * (1.0 - t)) + (b->vec[1] * t);
			a->vec[2] = (a->vec[2] * (1.0 - t)) + (b->vec[2] * t);
			a->vec[3] = (a->vec[3] * (1.0 - t)) + (b->vec[3] * t);
			break;

		case 3:
			a->vec[0] = (a->vec[0] * (1.0 - t)) + (b->vec[0] * t);
			a->vec[1] = (a->vec[1] * (1.0 - t)) + (b->vec[1] * t);
			a->vec[2] = (a->vec[2] * (1.0 - t)) + (b->vec[2] * t);
			break;

		case 2:
			a->vec[0] = (a->vec[0] * (1.0 - t)) + (b->vec[0] * t);
			a->vec[1] = (a->vec[1] * (1.0 - t)) + (b->vec[1] * t);
			break;

		case 1:
			a->vec[0] = (a->vec[0] * (1.0 - t)) + (b->vec[0] * t);
			break;

		default:
			for(i = 0; i < a->dim; i++) {
				a->vec[i] = (a->vec[i] * (1.0 - t)) + (b->vec[i] * t);
			}
			break;
	}
}

void symbol_randomize(Symbol *sym)
{
	int i;

	for (i = 0; i < sym->dim; i++)
	{
		sym->vec[i] = drand48();
	}
}

unsigned short symbol_get_dim(Symbol *sym)
{
	return sym->dim;
}

/* take all of the symbols in the list, and concatenate them together producing
	a higher dimensional symbol. */
Symbol* symbol_abstract(Symbol **list, int num)
{
	int i, j;
	int tindex;
	Symbol *abstraction;
	int abstraction_dimension = 0;

	/* figure out what the newly created symbol's abstraction dimension is
		going to be. */
	for (i = 0; i < num; i++)
	{
		abstraction_dimension += list[i]->dim;
	}
	
	/* make it */
	abstraction = symbol_init(abstraction_dimension);

	/* perform the merge */
	tindex = 0;
	/* copy each piece out of the list into the abstraction symbol preserving
		the order of the list in the abstraction symbol */
	for (i = 0; i < num; i++)
	{
		for (j = 0; j < list[i]->dim; j++)
		{
			abstraction->vec[tindex] = list[i]->vec[j];
			tindex++;
		}
	}

	return abstraction;
}

/* given the unabstraction dimension array, take the larger symbol and produce
	an array of smaller symbols stripping the dimensions off in order from the
	larger symbol. If I specify one smaller symbol of the same dimension
	as the passed in symbol, then effectively this degenerates into a 
	symbol_copy() call because I'm unabstracting a symbol into itself. */
Symbol** symbol_unabstract(Symbol *sym, int *dim, int num_dims)
{
	int i, j, count;
	unsigned int aggregate;
	Symbol **list;

	/* see if the unabstraction dimensions add up to the higher dimensional
		symbol */
	aggregate = 0;
	for (i = 0; i < num_dims; i++)
	{
		aggregate += dim[i];
		if (dim[i] <= 0)
		{
			printf("symbol_unabstract(): Cannot create lower dimensional "
				"symbol %d with %d dimensions!\n", i, dim[i]);
			exit(EXIT_FAILURE);
		}
	}

	/* sanity check and decent output */
	if (aggregate != sym->dim)
	{
		printf("symbol_unabstract(): Tried to unabstract a %d dimensional\n",
			sym->dim);
		printf("\tsymbol into %d lower dimensional symbols like this:\n",
			num_dims);
		for (i = 0; i < num_dims; i++)
		{
			printf("\t\tLower symbol number[%d]: dimension %d\n", i, dim[i]);
		}
		printf("\t%d total lower dimensions don't add up the original "
			"%d dimensions!\n", aggregate, sym->dim);
		exit(EXIT_FAILURE);
	}

	/* ok, if I got here, then unabstract the symbol */
	list = (Symbol**)xmalloc(sizeof(Symbol*) * num_dims);

	count = 0;
	for (i = 0; i < num_dims; i++)
	{
		list[i] = symbol_init(dim[i]);

		/* copy out of the big one, into the little one, preserving order */
		for (j = 0; j < dim[i]; j++)
		{
			list[i]->vec[j] = sym->vec[count];
			count++;
		}
	}

	return list;
}

void symbol_free(Symbol *sym)
{
	free(sym);
}

void symbol_add(Symbol *out, Symbol *lhs, Symbol *rhs)
{
	int i;

	if (lhs->dim != rhs->dim)
	{
		printf("symbol_add(): invalid dimensions\n");
		exit(EXIT_FAILURE);
	}

	for (i = 0; i < lhs->dim; i++)
	{
		out->vec[i] = lhs->vec[i] + rhs->vec[i];
	}
}

void symbol_mul(Symbol *sym, float num)
{
	int i;

	for (i = 0; i < sym->dim; i++)
	{
		sym->vec[i] *= num;
	}
}

void symbol_div(Symbol *sym, float num)
{
	int i;

	for (i = 0; i < sym->dim; i++)
	{
		sym->vec[i] /= num;
	}
}

void symbol_stdout(Symbol *sym)
{
	int i;

	printf("Symbol[%d]: ", sym->dim);
	for (i = 0; i < sym->dim; i++)
	{
		printf("%.16f ", sym->vec[i]);
	}
	printf("\n");
}

/* set values into a symbol */
void symbol_set(Symbol *sym, float *vals, int num_vals)
{
	int i;
	if (num_vals != sym->dim)
	{
		printf("symbol_set(): Can't set skew dimensional symbol!\n");
		exit(EXIT_FAILURE);
	}

	for (i = 0; i < sym->dim; i++)
	{
		sym->vec[i] = vals[i];
	}
}

void symbol_set_1(Symbol *sym, float v0)
{
	float val[1] = {v0};
	symbol_set(sym, val, 1);
}

void symbol_set_2(Symbol *sym, float v0, float v1)
{
	float val[2] = {v0, v1};
	symbol_set(sym, val, 2);
}

void symbol_set_3(Symbol *sym, float v0, float v1, float v2)
{
	float val[3] = {v0, v1, v2};
	symbol_set(sym, val, 3);
}

void symbol_set_4(Symbol *sym, float v0, float v1, float v2, float v3)
{
	float val[4] = {v0, v1, v2, v3};
	symbol_set(sym, val, 4);
}

void symbol_set_5(Symbol *sym, float v0, float v1, float v2, float v3, float v4)
{
	float val[5] = {v0, v1, v2, v3, v4};
	symbol_set(sym, val, 5);
}

void symbol_set_6(Symbol *sym, float v0, float v1, float v2, float v3, float v4,
					float v5)
{
	float val[6] = {v0, v1, v2, v3, v4, v5};
	symbol_set(sym, val, 6);
}

void symbol_set_index(Symbol *sym, float val, int index)
{
	if (index < 0 || index >= symbol_get_dim(sym))
	{
		printf("symbol_set_index(): Bounds error!\n");
		exit(EXIT_FAILURE);
	}

	sym->vec[index] = val;
}

/* get values out of a symbol and into an user allocated array */
void symbol_get(Symbol *sym, float *vals, int num_vals)
{
	int i;
	if (num_vals != sym->dim)
	{
		printf("symbol_set(): Can't get skew dimensional symbol!\n");
		exit(EXIT_FAILURE);
	}

	for (i = 0; i < sym->dim; i++)
	{
		vals[i] = sym->vec[i];
	}
}

void symbol_get_1(Symbol *sym, float *v0)
{
	float val[1];
	symbol_get(sym, val, 1);
	*v0 = val[0];
}

void symbol_get_2(Symbol *sym, float *v0, float *v1)
{
	float val[2];
	symbol_get(sym, val, 2);
	*v0 = val[0];
	*v1 = val[1];
}

void symbol_get_3(Symbol *sym, float *v0, float *v1, float *v2)
{
	float val[3];
	symbol_get(sym, val, 3);
	*v0 = val[0];
	*v1 = val[1];
	*v2 = val[2];
}

void symbol_get_4(Symbol *sym, float *v0, float *v1, float *v2, float *v3)
{
	float val[4];
	symbol_get(sym, val, 4);
	*v0 = val[0];
	*v1 = val[1];
	*v2 = val[2];
	*v3 = val[3];
}

void symbol_get_5(Symbol *sym, float *v0, float *v1, float *v2, float *v3, 
					float *v4)
{
	float val[5];
	symbol_get(sym, val, 5);
	*v0 = val[0];
	*v1 = val[1];
	*v2 = val[2];
	*v3 = val[3];
	*v4 = val[4];
}

void symbol_get_6(Symbol *sym, float *v0, float *v1, float *v2, float *v3, 
					float *v4, float *v5)
{
	float val[6];
	symbol_get(sym, val, 6);
	*v0 = val[0];
	*v1 = val[1];
	*v2 = val[2];
	*v3 = val[3];
	*v4 = val[4];
	*v5 = val[5];
}

void symbol_get_index(Symbol *sym, float *val, int index)
{
	if (index < 0 || index >= symbol_get_dim(sym))
	{
		printf("symbol_get_index(): Bounds error!\n");
		exit(EXIT_FAILURE);
	}

	*val = sym->vec[index];
}

void symbol_move(Symbol *dest, Symbol *source)
{
	int i;

	if (dest->dim != source->dim)
	{
		printf("symbol_move(): invalid dimensions\n");
		exit(EXIT_FAILURE);
	}

	for (i = 0; i < dest->dim; i++)
	{
		dest->vec[i] = source->vec[i];
	}
}













