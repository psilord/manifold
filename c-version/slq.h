#ifndef SLQUEUE_H
#define SLQUEUE_H

/* This header file describes a scan line queue buffer which I use in 
	various "image processing" passes I do across the SOM. See, each
	"pixel" in the SOM can be a huge vector, so instead of making a
	complete copy of the SOM to write the image transform into(which
	would be extremely memory intensive), I just make a scanline
	buffer which I can configure to tell me when enough scanlines
	have been processed that I can start writing the results back
	into the SOM without fear of contaminating the part of the
	image which hasn't been processed yet. 
*/

/* the container for a single scanline of Symbol pointers */
typedef struct SLQline_s
{
	/* which scanline am I? */
	int slid;

	/* the actual symbols dedicated to this scanline */
	int dim;
	int width;
	Symbol **line;

	/* pointer to the next one */
	struct SLQline_s *next;

} SLQline;

/* This structure holds a queue of scanlines of symbols. It is used to make the
	memory used by various image processing algorithms I'm going to use on 
	the SOM much more memory efficient. */
typedef struct SLQueue_s
{
	/* if I'm done entering stuff, then this trumps how many lines are in the
		queue concerning readiness for dequeueing. */
	int finished;

	/* number of scan lines current in the queue */
	int count;

	/* how many scan lines need to be in the queue before I can be considered
		ready for dequeueing? */
	int span;

	/* the scanline queue itself */
	SLQline *tail;
	SLQline *head;

} SLQueue;

/* deal with the SLQline structure */

/* create for me a single line buffer */
SLQline* slqline_init(int slid, int width, int dim);

/* get the scanline id for this scanline so I know where it goes */
int slqline_get_slid(SLQline *slql);

/* DON'T free this pointer you get back. It points into a Symbol* array the 
	width you specified of allocated symbols for you to read/write. */
Symbol** slqline_line(SLQline *slql);

/* how wide is this scanline? */
int slqline_get_width(SLQline *slql);

/* when done with a dequeued SLQline structure, get rid of it */
void slqline_free(SLQline *slql);

/* deal with the SLQueue structure */

/* Create a scanline buffer to hold a number of allocated scanlines of a SOM.
	Used to keep the memory down while performing image-processing-like 
	algorithms in the SOM. If I want to dequeue the queue fully, then I should
	mark it as finished so it isn't waiting on having enough spanned 
	lines before being ready */
SLQueue* slq_init(int span);

/* When I enqueue a line, give back a line buffer that I can fill in with the
	processed scanline information. Make sure to supply a scan line id so I
	know where this scanline goes when I dequeue it. */
void slq_enqueue(SLQueue *slq, SLQline *slql);

/* give me back the tail of the queue, or NULL if not ready */
SLQline* slq_dequeue(SLQueue *slq);

/* If the queue is ready for dequeueing, let me know */
int slq_ready(SLQueue *slq);

/* if the queue is empty, return TRUE */
int slq_empty(SLQueue *slq);

/* when I am going to put no more into the queue, mark it finished so that
	it is ALWAYS ready */
void slq_set_finished(SLQueue *slq, int value);
int slq_get_finished(SLQueue *slq);

/* Free any memory that might exist in the SLQueue */
void slq_free(SLQueue *slq);


#endif



