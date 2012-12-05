#ifndef SOM_H
#define SOM_H

#include <stdio.h>
#include <stdlib.h>
#include <math.h>

#include "symbol.h"
#include "input.h"

enum 
{
	SOM_LEARNING,
	SOM_CLASSIFYING
};

enum
{
	SOM_BMU_METHOD_FIXED,
	SOM_BMU_METHOD_CENTROID
};

enum {
	SOM_INT_EMPTY = 0,
	SOM_INT_FULL = 1,
	SOM_INT_PARTIAL = 2
};

enum {
	SOM_STYLE_ACTUAL,
	SOM_STYLE_QUALITY
};

enum {
	SOM_REQUEST_LEARN,
	SOM_REQUEST_CLASSIFY
};


/* How I get a 2D address out of the linear array. */
#define SOM_ADR(row, col, som) (((row) * ((som)->sd.cols)) + (col))

/* forward declaration because SOM_s is used a bit recursively */
struct SOM_s;

typedef float (*SOM_RADIUS_FUNC)(struct SOM_s *s);

/* This structure describes the initial characteristics of a SOM, this is
	used in other places as a convienent way to specify construction of a
	SOM */
typedef struct SOMDesc_s
{
	unsigned short dim;
	unsigned int train_iter;
	unsigned int rows;
	unsigned int cols;
	SOM_RADIUS_FUNC radius_func;
} SOMDesc;

typedef struct SOM_s
{
	/* am I in a training mode, or a classification mode? */
	/* XXX I need to make this thing figure out how to adapt to what its
		input is. If the input is stable over long periods of time, then
		I should stop learning, but if the input starts deviating from
		what the classification was, I should start relearning */
	int mode;

	/* the initial physical characteristics of this som */
	SOMDesc sd;

	/* what iteration of my training am I currently on? */
	int current_iter;

	/* how large is the initial neighborhood */
	float initial_radius;

	/* the array of the neurons, they are symbols of all the same dimension. */
	Symbol **neuron;

	/* an array containing a quality map of the som */
	float final_computation;
	float max_dist;
	float *qmap;

} SOM;


/* 
	dim is the dimension of the som's symbol vectors in each neuron 
	train_iter is how many iteration to train on input data
	rows is how many rows there are in the som
	cols is how many cols there are in the som
	x,y is the lower left hand corner of the SOM
	radius_func is a supplied(or NULL) function to compute the initial
		neighborhood radius for learning
*/
SOM* som_init(unsigned short dim, unsigned int train_iter, unsigned int rows, 
	unsigned int cols, SOM_RADIUS_FUNC radius_func);

/* Do the same as above, just accept a SOMDesc for some of the information */
SOM* som_init_with_sd(SOMDesc *sd);

/* determine the best matching unit, modified slightly so that when I'm in 
	clasification mode I always get back the idenitical neuron for the same
	lookup instead of a 1/n probability choice out of all suitable bmu
	candidates */
void som_bmu(SOM *s, Symbol *p, int *row, int *col, int method, int dx, int dy);

/* Make the SOM learn about the symbol, if it is still learning at all.
	prow and pcol are filled in regardless if the som is in learning or
	classification mode. Also, I can ask this function to merely classify the
	input, or actually perform learning on it for each symbol if I choose. */
unsigned int som_learn(SOM *s, Symbol *p, int *prow, int *pcol, int dx, int dy,
	int request);

int som_get_rows(SOM *s);
int som_get_cols(SOM *s);

/* Am I in classification or learning mode? */
unsigned int som_get_mode(SOM *s);

/* what dimension of information does this SOM accept? */
unsigned int som_get_dimension(SOM *s);

/* give me the symbol pointer of the neuron in question */
Symbol* som_symbol_ref(SOM *s, int row, int col);

/* a function that produces a well-known initial radius of neighbors to affect
	when learning first starts */
float som_radius_func_default(SOM *s);

/* when computin gthe quality map of the SOM, how well should it be done? */
void som_set_quality_factor(SOM *s, float quality);

/* get rid of a SOM */
void som_free(SOM *s);

/* draw the SOM, either drawing the neurons themselves, or the quality map */
void som_draw(SOM *s, unsigned int style, int x, int y);

#endif





