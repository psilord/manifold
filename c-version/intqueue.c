#include <stdio.h>
#include <stdlib.h>
#include "common.h"

IntQueue* intqueue_init(int num_syms, int num_slices)
{
	int i, j;
	IntQueue *iq = NULL;
	Symbol **sq = NULL;

	iq = (IntQueue*)xmalloc(sizeof(IntQueue) * 1);

	iq->num_syms = num_syms;
	iq->num_slices = num_slices;

	/* allocate the slice array */
	iq->slice = (Symbol***)xmalloc(sizeof(Symbol**) * num_slices);

	/* allocate the requested queue(of length num_syms) for each slice */
	for (i = 0; i < num_slices; i++)
	{
		iq->slice[i] = (Symbol**)xmalloc(sizeof(Symbol*) * num_syms);
		
		/* initialize it to NULL */
		sq = iq->slice[i];
		
		for (j = 0; j < num_syms; j++)
		{
			sq[j] = NULL;
		}
	}

	/* allocate the array that tells me how full each slice queue is */
	iq->slice_top = (int*)xmalloc(sizeof(int) * num_slices);
	for (i = 0; i < num_slices; i++)
	{
		/* when the queue is completely empty, the top is equal to num_syms
			and this mustr be treated carefully during insertion of symbols
			int othe queue */
		iq->slice_top[i] = num_syms;
	}

	/* start at the beginning slice */
	iq->curr_slice = 0;

	/* This is whether or not the integration queue is ready to give me
		something */
	iq->ready = FALSE;
	iq->ready_slice = -1;

	return iq;
}

void intqueue_free(IntQueue *iq)
{
	Symbol **sq = NULL;
	int i, j;

	for (i = 0; i < iq->num_slices; i++)
	{
		sq = iq->slice[i];
		/* delete the queue, taking care if it is partially full */
		for (j = iq->slice_top[i]; j < iq->num_syms; j++)
		{
			/* The integration queue owns the symbol memory, so it must free
				them */
			if (sq[j] != NULL)
			{
				symbol_free(sq[j]);
			}
		}

		free(iq->slice[i]);
	}

	free(iq->slice);

	free(iq->slice_top);
}

/* put the symbol into the current slice's queue, if the slice would be full 
	after I put it in, then mark the intqueue as ready, and set up the ready
	slice variable. Then increment the slice index. */
void intqueue_enqueue(IntQueue *iq, Symbol *sym)
{
	int i;
	Symbol **sq = NULL;

	sq = iq->slice[iq->curr_slice];

	switch(iq->slice_top[iq->curr_slice])
	{
		/* if I enqueue this symbol into an already full current slice, then
			free the top, move everything up, and add it to the bottom. Then
			mark this slicing queue as the ready queue, and increment the 
			slice index */
		case 0:
			/* get rid of the top of the slice queue */
			symbol_free(sq[0]);

			/* move everything up */
			for (i = 1; i < iq->num_syms; i++)
			{
				sq[i-1] = sq[i];
			}

			/* enqueue the symbol onto the bottom */
			(iq->slice[iq->curr_slice])[iq->num_syms - 1] = sym;

			/* do not modify the top of this slice, cause it is still zero */

			/* this queue is now ready */
			iq->ready = TRUE;
			iq->ready_slice = iq->curr_slice;
		
			break;

			/* if I enqueue this symbol into an slice queue which then becomes 
				full as a result of the enqueue, then mark it as ready */
		case 1:

			/* move everything up */
			for (i = iq->slice_top[iq->curr_slice]; i < iq->num_syms; i++)
			{
				sq[i-1] = sq[i];
			}

			/* enqueue the symbol onto the bottom */
			(iq->slice[iq->curr_slice])[iq->num_syms - 1] = sym;

			/* fix the top of this slice to be correct */
			iq->slice_top[iq->curr_slice]--;

			/* this queue is now ready */
			iq->ready = TRUE;
			iq->ready_slice = iq->curr_slice;
			
			break;

		/* just enqueue the symbol into this particular slice */
		default:
			/* move everything up */
			for (i = iq->slice_top[iq->curr_slice]; i < iq->num_syms; i++)
			{
				sq[i-1] = sq[i];
			}

			/* enqueue the symbol onto the bottom */
			(iq->slice[iq->curr_slice])[iq->num_syms - 1] = sym;

			/* Now that I added something to the queue, move the head */
			iq->slice_top[iq->curr_slice]--;

			/* once an slicing queue becomes ready, every next slicing queue
				MUST be ready when the next enqueue happens, so this is a 
				sanity check for that. */
			if (iq->ready == TRUE)
			{
				printf("intqueue_enqueue(): cannot go from ready to "
					"nonready!\n");
				exit(EXIT_FAILURE);
			}
			iq->ready = FALSE;
			
			break;
	}

	/* the next enqueue goes to the next slice in a round robin fashion */
	iq->curr_slice = (iq->curr_slice + 1) % iq->num_slices;
}

int intqueue_ready(IntQueue *iq)
{
	return iq->ready;
}

/* Take the particular ready queue specified and produce an abstraction of it */
Symbol* intqueue_dequeue(IntQueue *iq)
{
	Symbol *abstraction;

	if (iq->ready == FALSE)
	{
		return NULL;
	}

	abstraction = symbol_abstract(iq->slice[iq->ready_slice], iq->num_syms);

	return abstraction;
}

/* return how many integrations this int queue is performing. */
int intqueue_integrations(IntQueue *iq)
{
	return iq->num_syms;
}

void intqueue_stdout(IntQueue *iq)
{
	int i, j;
	Symbol **sq = NULL;

	printf("\t\tIntegration Queue:\n");
	printf("\t\t\tReady: %s\n", iq->ready==TRUE?"TRUE":"FALSE");
	printf("\t\t\tReady slice: %d\n", iq->ready_slice);
	printf("\t\t\tNum Syms: %d\n", iq->num_syms);
	printf("\t\t\tNum Slices: %d\n", iq->num_slices);
	for (i = 0; i < iq->num_slices; i++)
	{
		sq = iq->slice[i];
		printf("\t\t\t\tSlice: %d\n", i);
		for (j = 0; j < iq->num_syms; j++)
		{
			if (j < iq->slice_top[i]) {
				printf("\t\t\t\t\tN/A\n");
			} else {
				printf("\t\t\t\t\t");
				symbol_stdout(sq[j]);
			}
		}
	}
}



















