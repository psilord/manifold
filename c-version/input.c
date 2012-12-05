#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include "common.h"

/* initialize the table to read convert printable ascii characters into 
	symbols of arbitrary(but constant) dimension */
Input* input_init(unsigned short dimension)
{
	int i;
	Input *inp;
	int sum;
	int count;

	inp = (Input*)xmalloc(sizeof(Input) * 1);
	inp->dim = dimension;
	inp->printable = 0;

	/* count how many printable characters there are */
	sum = 0;
	for(i = 0; i < 256; i++)
	{
		if (isprint(i))
		{
			sum++;
		}
	}
	
	/* set up the translation table */
	count = 0;
	for(i = 0; i < 256; i++)
	{
		inp->xlt[i].c = i;

		/* only make symbols for printable characters */
		if (isprint(i))
		{
			inp->xlt[i].sym = symbol_init(inp->dim);
			/* each symbol is going to interpolate from (nearly) zero 
				to one based upon how many printable
				characters there were. This prevents
				two characters from having the same
				symbol vector if I were to use random
				symbols. Also, because of the fact that
				the characters are actually grouped
				near to each other in ascii means
				that the interpolant will mimic the
				distance function between printable
				characters. This means S is near to T
				in the interpolant space. zero is left
				unused as I'm going to use it as a
				"unification" number in the math*/

			symbol_set_all(inp->xlt[i].sym, 
				((float)count+1.0) / (float)sum);

			inp->printable++;
			count++;
		}
		else
		{
			inp->xlt[i].sym = NULL;
		}
	}

	printf("Initialized %d printable characters.\n", inp->printable);
	return inp;
}

Symbol* input_random_choice(Input *inp)
{
	int i, count;
	int choice;

	/* using the higher order bits, get me a value from 0 to
		inp->printable - 1 */
	choice = (int)(((float)inp->printable*rand())/(RAND_MAX+1.0));

	if (choice < 0 || choice >= inp->printable)
	{
		printf("input_random_choice(): What!?! Why isn't %d in the "
			"range of 0 to %d\n", choice, inp->printable);
		exit(EXIT_FAILURE);
	}

	/* ok, now get the one we chose out of the bigger array */
	count = 0;
	for (i = 0; i < 256; i++)
	{
		/* of the defined ones, give me the random choice */
		if (inp->xlt[i].sym != NULL && count == choice)
		{
			return inp->xlt[i].sym;
		}

		/* only count the defined ones */
		if (inp->xlt[i].sym != NULL)
		{
			count++;
		}
	}

	printf("input_random_choice(): never supposed to return NULL!\n");
	fflush(NULL);
	return NULL;
}

/* get me a random choice from 'a' to the num desired, num must be less than
	26 */
Symbol* input_random_choice_special(Input *inp, int num)
{
	int choice;

	if (num >= 26)
	{
		printf("input_random_choice_special(): num must be less than 26!\n");
		exit(EXIT_FAILURE);
	}

	/* using the higher order bits, get me a value from 0 to
		inp->printable - 1 */
	choice = (int)(((float)num*rand())/(RAND_MAX+1.0));

	if (choice < 0 || choice >= num)
	{
		printf("input_random_choice(): What!?! Why isn't %d in the "
			"range of 0 to %d\n", choice, num);
		exit(EXIT_FAILURE);
	}

	/* ok, now get the one we chose out of the bigger array */
	return inp->xlt[((int)'a') + choice].sym;
}

Symbol* input_xlate_char(Input *inp, char c)
{
	if (inp->xlt[(int)c].sym == NULL)
	{
		printf("input_xlate_char(): passed a nonprintable character: %c!\n", c);
		exit(EXIT_FAILURE);
	}

	return inp->xlt[(int)c].sym;
}

/* compute the closes input vector sym is to, plus a normalized error that
	explain how good out of all errors was this match */
Symbol* input_decode_to_sym(Input *inp, Symbol *unk, double *error)
{
	double min_dist = 99999, max_dist = 0;
	double dist;
	int i;
	int loc = 0;

	for (i = 0; i < 256; i++)
	{
		if (inp->xlt[i].sym != NULL)
		{
			dist = symbol_dist(unk, inp->xlt[i].sym);
			if (dist < min_dist)
			{
				min_dist = dist;
				loc = i;
			}
			if (dist > max_dist)
			{
				max_dist = dist;
			}
		}
	}

	*error = 1.0 - ((max_dist - min_dist) / max_dist);

	return inp->xlt[loc].sym;
}

char input_decode_to_char(Input *inp, Symbol *unk, double *error)
{
	double min_dist = 99999, max_dist = 0;
	double dist;
	int i;
	int loc = 0;

	for (i = 0; i < 256; i++)
	{
		if (inp->xlt[i].sym != NULL)
		{
			dist = symbol_dist(unk, inp->xlt[i].sym);
			if (dist < min_dist)
			{
				min_dist = dist;
				loc = i;
			}
			if (dist > max_dist)
			{
				max_dist = dist;
			}
		}
	}

	*error = 1.0 - ((max_dist - min_dist) / max_dist);

	return inp->xlt[loc].c;
}


void input_free(Input *inp)
{
	int i;

	for(i = 0; i < 256; i++)
	{
		if (inp->xlt[i].sym != NULL)
		{
			symbol_free(inp->xlt[i].sym);
		}
	}

	free(inp);
}












