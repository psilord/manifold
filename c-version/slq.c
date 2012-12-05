#include <stdio.h>
#include <stdlib.h>
#include "common.h"

/* BEGIN SLQline implementation */
SLQline* slqline_init(int slid, int width, int dim)
{
	SLQline *slql;
	int i;

	slql = (SLQline*)xmalloc(sizeof(SLQline) * 1);

	slql->slid = slid;
	slql->width = width;
	slql->line = (Symbol**)xmalloc(sizeof(Symbol*) * slql->width);
	for (i = 0; i < width; i++)
	{
		slql->line[i] = symbol_init(dim);
		symbol_zero(slql->line[i]);
	}

	slql->next = NULL;

	return slql;
}

int slqline_get_slid(SLQline *slql)
{
	return slql->slid;
}

/* don't free the pointer associated with this function call */
Symbol** slqline_line(SLQline *slql)
{
	return slql->line;
}

int slqline_get_width(SLQline *slql)
{
	return slql->width;
}

/* check to make sure I don't delete something I'm not supposed to */
void slqline_free(SLQline *slql)
{
	int i;

	for (i = 0; i < slql->width; i++)
	{
		if (slql->line[i] != NULL)
		{
			symbol_free(slql->line[i]);
			slql->line[i] = NULL;
		}
	}

	free(slql->line);
	slql->line = NULL;

	slql->next = NULL;

	free(slql);
}

/* END SLQline implementation */


/* BEGIN SLQueue implementation */

/* initialize a scanline queuing buffer to put stuff into */
SLQueue* slq_init(int span)
{
	SLQueue *slq = NULL;

	slq = (SLQueue*)xmalloc(sizeof(SLQueue) * 1);

	slq->finished = FALSE;
	slq->count = 0;
	slq->span = span;

	/* the dequeue side of the queue, this is a linked list of structures
		that go from the tail to the head. */
	slq->tail = NULL;

	/* pointer to the head of the queue */
	slq->head = NULL;

	return slq;
}

void slq_free(SLQueue *slq)
{
	SLQline *curr;
	SLQline *deleted;

	/* walked down the linked list and delete it */
	curr = slq->tail;
	while(curr != NULL)
	{
		deleted = curr;
		curr = curr->next;
		slqline_free(deleted);
	}

	slq->tail = NULL;
	slq->head = NULL;

	free(slq);
}

void slq_enqueue(SLQueue *slq, SLQline *slql)
{
	/* insert it into the queue */
	if (slq->tail == NULL)
	{
		/* first thing in the queue */
		slq->tail = slql;
		slq->head = slql;
	}
	else
	{
		/* multiple things in the queue */
		slq->head->next = slql;
		slq->head = slql;
	}
	
	slq->count++;
}

SLQline* slq_dequeue(SLQueue *slq)
{
	SLQline *slql;

	if (slq_ready(slq) == FALSE)
	{
		return NULL;
	}

	slql = slq->tail;
	slq->tail = slq->tail->next;

	if (slq->tail == NULL)
	{
		slq->head = NULL;
	}

	slql->next = NULL;

	slq->count--;

	return slql;
}

void slq_set_finished(SLQueue *slq, int value)
{
	slq->finished = value;
}

int slq_get_finished(SLQueue *slq)
{
	return slq->finished;
}

int slq_empty(SLQueue *slq)
{
	if (slq->count == 0)
	{
		return TRUE;
	}

	return FALSE;
}

int slq_ready(SLQueue *slq)
{
	if (slq_empty(slq) == TRUE)
	{
		return FALSE;
	}

	if (slq_get_finished(slq) == TRUE)
	{
		return TRUE;
	}

	if (slq->count > slq->span)
	{
		return TRUE;
	}

	return FALSE;
}

/* END SLQueue implementation */












