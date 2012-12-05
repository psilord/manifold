#include <stdio.h>
#include <math.h>
#include "common.h"

/* Create for me a representation of a specific convolution adjusting it, if
	applicable, by the parameter the user supplies */
Conv* conv_init(int style, double param)
{
	int i, j;
	Conv *c = NULL;

	c = (Conv*)xmalloc(sizeof(Conv) * 1);

	switch(style)
	{
		case CONV_BLUR_SMEAR:

			c->desc = "All Edge Sensitive";
			c->type = style;
			c->rows = 3;
			c->cols = 3;
			c->coef = (double*)xmalloc(sizeof(double) * 
							(c->rows*c->cols));

			for (i = 0; i < c->rows; i++)
			{
				for (j = 0; j < c->cols; j++)
				{
					c->coef[CONV_COEF(i,j,c)] = param;
				}
			}
			c->coef[CONV_COEF(c->rows / 2 , c->cols / 2,c)] = 1;

			break;

		case CONV_BLUR_LIGHT:

			c->desc = "Simple Blur";
			c->type = style;
			c->rows = 3;
			c->cols = 3;
			c->coef = (double*)xmalloc(sizeof(double) * 
							(c->rows*c->cols));

			c->coef[CONV_COEF(0,0,c)] = param / sqrt(2.0); 
			c->coef[CONV_COEF(0,1,c)] = param; 
			c->coef[CONV_COEF(0,2,c)] = param / sqrt(2.0);

			c->coef[CONV_COEF(1,0,c)] = param; 
			c->coef[CONV_COEF(1,1,c)] = param;
			c->coef[CONV_COEF(1,2,c)] = param;

			c->coef[CONV_COEF(2,0,c)] = param / sqrt(2.0); 
			c->coef[CONV_COEF(2,1,c)] = param; 
			c->coef[CONV_COEF(2,2,c)] = param / sqrt(2.0);
			break;

		case CONV_ENHANCED_DETAIL:

			c->desc = "Enhanced Detail";
			c->type = style;
			c->rows = 3;
			c->cols = 3;
			c->coef = (double*)xmalloc(sizeof(double) * 
							(c->rows*c->cols));

			c->coef[CONV_COEF(0,0,c)] = 0; 
			c->coef[CONV_COEF(0,1,c)] = -1; 
			c->coef[CONV_COEF(0,2,c)] = 0;

			c->coef[CONV_COEF(1,0,c)] = -1; 
			c->coef[CONV_COEF(1,1,c)] = param; 
			c->coef[CONV_COEF(1,2,c)] = -1;

			c->coef[CONV_COEF(2,0,c)] = 0; 
			c->coef[CONV_COEF(2,1,c)] = -1; 
			c->coef[CONV_COEF(2,2,c)] = 0;
			break;

		case CONV_SOFTEN:

			c->desc = "Soften";
			c->type = style;
			c->rows = 3;
			c->cols = 3;
			c->coef = (double*)xmalloc(sizeof(double) * 
							(c->rows*c->cols));

			c->coef[CONV_COEF(0,0,c)] = param; 
			c->coef[CONV_COEF(0,1,c)] = param; 
			c->coef[CONV_COEF(0,2,c)] = param;

			c->coef[CONV_COEF(1,0,c)] = param; 
			c->coef[CONV_COEF(1,1,c)] = param; 
			c->coef[CONV_COEF(1,2,c)] = param;

			c->coef[CONV_COEF(2,0,c)] = param; 
			c->coef[CONV_COEF(2,1,c)] = param; 
			c->coef[CONV_COEF(2,2,c)] = param;
			break;

		case CONV_LAPLACIAN:

			c->desc = "Laplacian";
			c->type = style;
			c->rows = 3;
			c->cols = 3;
			c->coef = (double*)xmalloc(sizeof(double) * 
							(c->rows*c->cols));

			c->coef[CONV_COEF(0,0,c)] = -1; 
			c->coef[CONV_COEF(0,1,c)] = -1; 
			c->coef[CONV_COEF(0,2,c)] = -1;

			c->coef[CONV_COEF(1,0,c)] = -1; 
			c->coef[CONV_COEF(1,1,c)] = param; 
			c->coef[CONV_COEF(1,2,c)] = -1;

			c->coef[CONV_COEF(2,0,c)] = -1; 
			c->coef[CONV_COEF(2,1,c)] = -1; 
			c->coef[CONV_COEF(2,2,c)] = -1;
			break;

		case CONV_USER_DEF:

			c->desc = "User Defined";
			c->type = style;
			c->rows = 3;
			c->cols = 3;
			c->coef = (double*)xmalloc(sizeof(double) * 
							(c->rows*c->cols));

			/* Since the user is going to screw with this, give them an
				identity convolution */
			c->coef[CONV_COEF(0,0,c)] = 0; 
			c->coef[CONV_COEF(0,1,c)] = 0; 
			c->coef[CONV_COEF(0,2,c)] = 0;

			c->coef[CONV_COEF(1,0,c)] = 0; 
			c->coef[CONV_COEF(1,1,c)] = 1; 
			c->coef[CONV_COEF(1,2,c)] = 0;

			c->coef[CONV_COEF(2,0,c)] = 0; 
			c->coef[CONV_COEF(2,1,c)] = 0; 
			c->coef[CONV_COEF(2,2,c)] = 0;
			break;

		default:
			printf("convolution_init(): Unknown convolution style!\n");
			exit(EXIT_FAILURE);
			break;

	};

	return c;
}

void conv_free(Conv *c)
{
	free(c->coef);
	free(c);
}
double conv_get_rows(Conv *c)
{
	return c->rows;
}

double conv_get_cols(Conv *c)
{
	return c->cols;
}

double conv_get_coef(int row, int col, Conv *c)
{
	return c->coef[CONV_COEF(row,col,c)];
}

void conv_set_coef(int row, int col, Conv *c, double param)
{
	c->coef[CONV_COEF(row,col,c)] = param;
}

void conv_apply_easy(SOM *som, int style, double param, double thld)
{
	Conv *c;

	c = conv_init(style, param);

	conv_apply(som, c, thld);

	conv_free(c);
}

/* This function uses a scanline buffer to be as frugal as it can in the
	memory usage associated with an application of a convolution. In addition,
	when looking at a neuron from the source image with the template, only 
	use it if its percentage error is less than the threshold. If it is unused,
	then clip it out of the convolution for the candidate neuron. */
void conv_apply(SOM *s, Conv *c, double thld)
{
	int row, col;
	Symbol *sum;
	Symbol *mul;
	Symbol *candidate;
	double divisor;
	SLQueue *slq;
	SLQline *slql;
	Symbol **data;
	int i;
	int slid;
	int xrow, xcol;
	int cr, cc;
	double coef;
	double dist;

	/* make a scanline buffer big enough to hold half of the convolution */
	slq = slq_init(((int)(conv_get_rows(c) / 2.0)) + 1);

	sum = symbol_init(s->sd.dim);
	mul = symbol_init(s->sd.dim);
	
	/* perform the convolution for each neuron in the SOM */
	for(row = 0; row < s->sd.rows; row++)
	{
		/* set up a scan line for the line buffer */
		slql = slqline_init(row, s->sd.cols, s->sd.dim);
		/* data is an alias pointer into memory associated with the slqline */
		data = slqline_line(slql);

		for (col = 0; col < s->sd.cols; col++)
		{	
			/* set things up to compute the convolution */
			symbol_one(mul);
			divisor = 0;

			/* the end result of the convolution */
			symbol_zero(sum);

			/* Perform the convolution of the applicable elements around the 
				neuron at row, col */
			for (cr = 0; cr < conv_get_rows(c); cr++)
			{
				for (cc = 0; cc < conv_get_cols(c); cc++)
				{
					/* This is the coordinates of the convolution in SOM 
						space. */
					xrow = row - (int)(conv_get_rows(c) / 2) + cr;
					xcol = col - (int)(conv_get_cols(c) / 2) + cc;

					/* clip the convolution map against the edges of the SOM.
						The center of the convolution represents the neuron
						at row,col */
					if ((xrow >= 0 && xrow < s->sd.rows) &&
						(xcol >= 0 && xcol < s->sd.cols))
					{
						/* see if the neuron I've chosen should be clipped out 
							because it is too far away or not */
						dist = symbol_dist(
							som_symbol_ref(s, row, col),
							som_symbol_ref(s, xrow, xcol));

						if (dist <= thld)
						{	
							coef = conv_get_coef(cr, cc, c);
							symbol_move(mul, som_symbol_ref(s, xrow, xcol));
							symbol_mul(mul, coef);
							symbol_add(sum, sum, mul);
	
							/* if an element of the convolution has been utilized,
								then make sure it is counted in the divisor. */
							divisor += coef;
						}
					}
				}
			}

			/* if the divisor ends not being zero, then divide the convolution
				result by it. If it is effectively zero, then ignore it since
				technically it becomes 1, and x / 1 = x */
			if ( !(fabs(divisor) < CONV_TOL) )
			{
				/* apply the divisor */
				symbol_div(sum, divisor);
			}

			/* store the result in the line buffer at the right location */
			symbol_move(data[col], sum);
		}

		/* the slql memory is now owned by the slq structure */
		slq_enqueue(slq, slql);

		/* defensive coding */
		slql = NULL;
		data = NULL; 

		/* if anything happens to be ready, copy it back into the image */
		while(slq_ready(slq) == TRUE)
		{
			slql = slq_dequeue(slq);
			if (slql == NULL)
			{
				printf("slql shouldn't be null here!\n");
				exit(EXIT_FAILURE);
			}
			data = slqline_line(slql);
			slid = slqline_get_slid(slql);

			for (i = 0; i < slqline_get_width(slql); i++)
			{
				candidate = som_symbol_ref(s, slid, i);
				symbol_move(candidate, data[i]);
			}

			slqline_free(slql);
			slql = NULL;
		}
	}
	
	/* no more to put in, so mark the queue as fully finished */
	slq_set_finished(slq, TRUE);

	/* consume the rest of the stuff in the queue */
	while(slq_ready(slq) == TRUE)
	{
		slql = slq_dequeue(slq);
		if (slql == NULL)
		{
			printf("slql shouldn't be null here either!\n");
			exit(EXIT_FAILURE);
		}
		data = slqline_line(slql);
		slid = slqline_get_slid(slql);

		for (i = 0; i < slqline_get_width(slql); i++)
		{
			candidate = som_symbol_ref(s, slid, i);
			symbol_move(candidate, data[i]);
		}

		slqline_free(slql);
		slql = NULL;
	}

	slq_free(slq);

	symbol_free(sum);
	symbol_free(mul);

}

