#ifndef INT_QUEUE
#define INT_QUEUE


/* An integration queue is a special type of queue which preserves aliasing
	of incomming data while integrating it. When a symbol in enqueued, it is
	enqueued into a special internal queue called a "slice", and then the
	slice index in incremented. When a 'slicing queue' is full, the IntQueue
	is marked ready, and can then produce a fully abstracted symbol representing
	the queue which had been marked as active */

typedef struct IntQueue_s
{
	/* how many symbols this queue is integrating */
	int num_syms;

	/* how many internal queues do I need to preserve the alias ordering */
	int num_slices;

	/* each slice index is another queue of num_syms length, the queues
		fill from the end to the beginning and when something gets pushed off
		the top, it is deallocated. */
	Symbol ***slice;

	/* each slice is a queue, and this array holds how full each slicing
		queue is, this lets me know how to append new symbols into each 
		individual queue. The TOP of the queue is 0 and the END of the queue
		is num_syms. This number is a valid index to the head of the queue. */
	int *slice_top;

	/* which slice queue to enqueue the data into this wraps at the mod of
		num_slices */
	int curr_slice;

	/* this is false if the queue can't provide an integration, and true if
		it can */
	int ready;

	/* which slice is ready to be integrated, this is one behind the
		curr_slice */
	int ready_slice;

} IntQueue;

/* set up an integration queue */
IntQueue* intqueue_init(int num_syms, int num_slices);

/* free an integration queue */
void intqueue_free(IntQueue *iq);

/* insert a symbol into the integration queue, making sure to stick it into
	the right slice, as determined by how many previous integration requests
	there had been. This function takes control of the incomming memory
	associated with the symbol. */
void intqueue_enqueue(IntQueue *iq, Symbol *sym);

/* return TRUE or FALSE if this queue is ready to be abstracted */
int intqueue_ready(IntQueue *iq);

/* performs an abstraction of the integration queue and returns a newly
	allocated abstracted symbol (of the active slice). If no slice is 
	active, then return NULL */
Symbol* intqueue_dequeue(IntQueue *iq);

/* tell me how many integrations this intqueue is performing */
int intqueue_integrations(IntQueue *iq);

void intqueue_stdout(IntQueue *iq);

#endif


