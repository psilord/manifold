#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <GL/gl.h>
#include <GL/glu.h>
#include "common.h"

static void som_draw_actual(SOM *s, int x, int y);
static void som_draw_quality(SOM *s, unsigned int quality, int x, int y);

/* various bmu selection methods you can choose */
void som_bmu_fixed(SOM *s, Symbol *p, int *row, int *col, int dx, int dy);
void som_bmu_centroid(SOM *s, Symbol *p, int *row, int *col, int dx, int dy);

/* return a reference to a symbol in the SOM */
Symbol* som_symbol_ref(SOM *s, int row, int col)
{
	/* Sanity check the request */
/*	if (row < 0 || col < 0 || row >= s->sd.rows || col >= s->sd.cols)*/
/*	{*/
/*		printf("som_symbol_ref(): Invalid coordinates!\n");*/
/*		printf("SOM size: rows = %d, cols = %d\n", s->sd.rows, s->sd.cols);*/
/*		printf("Asked for location: row %d, col %d\n", row, col);*/
/*		exit(EXIT_FAILURE);*/
/*	}*/
	return s->neuron[SOM_ADR(row, col, s)];
}

/* a simple wrapper around the real initializer function */
SOM* som_init_with_sd(SOMDesc *sd)
{
	return som_init(sd->dim, sd->train_iter, sd->rows, sd->cols,
			sd->radius_func);
}

/* initialize the som with random values */
SOM* som_init(unsigned short dim, unsigned int train_iter, unsigned int rows, 
	unsigned int cols, float (*radius_func)(SOM *s))
{
	SOM *s;
	int i;

	s = (SOM*)xmalloc(sizeof(SOM) * 1);

	/* set up basic stuff */
	s->sd.dim = dim;
	s->sd.rows = rows;
	s->sd.cols = cols;
	s->sd.radius_func = radius_func;
	s->sd.train_iter = train_iter;
	if (s->sd.train_iter < 2)
	{
		printf("Whoops, need train_iter to be 2 or more\n");
		exit(EXIT_FAILURE);
	}

	s->current_iter = 0;
	s->mode = SOM_LEARNING;

	/* if the radius function is null, then use the default one */
	if (radius_func == NULL) {
		/* rows and cols must be set up before this is called */
		s->initial_radius = som_radius_func_default(s);
	} else {
		s->initial_radius = radius_func(s);
	}

	/* get the neuron map */
	s->neuron = (Symbol**)xmalloc(sizeof(Symbol*) * (s->sd.rows*s->sd.cols));

	/* initialize the neuron map */
	for (i = 0; i < (s->sd.rows*s->sd.cols); i++)
	{
		s->neuron[i] = symbol_init(s->sd.dim);
		symbol_randomize(s->neuron[i]);
	}

	/* set up the scalar_field for the quality map of the SOM */
	s->max_dist = 0.0;
	s->final_computation = FALSE;
	s->qmap = (float*)xmalloc(sizeof(float) * (s->sd.rows * s->sd.cols));

	/* For the exponential decay model, given max iterations, what is our 
		half-life if we want to have a neighborhood of 
		(+.5. -.5)  around the learning point). 
	*/
	s->half_life = 
		-((log(2.0) * s->sd.train_iter) / log(.5 / s->initial_radius));

	return s;
}

unsigned int som_get_dimension(SOM *s)
{
	return s->sd.dim;
}

unsigned int som_get_mode(SOM *s)
{
	return s->mode;
}

/* return the a distance in neuron space based upon the rows, and cols */
float som_radius_func_default(SOM *s)
{
	return s->sd.rows > s->sd.cols ? s->sd.rows / 2.0 : s->sd.cols / 2.0;
}

/* Pick a particular method of finding the BMU */
void som_bmu(SOM *s, Symbol *p, int *row, int *col, int method, int dx, int dy)
{
	switch(method)
	{
		case SOM_BMU_METHOD_FIXED:
			som_bmu_fixed(s, p, row, col, dx, dy);
			break;
		case SOM_BMU_METHOD_CENTROID:
			som_bmu_centroid(s, p, row, col, dx, dy);
			break;
		default:
			printf("som_bmu(): Unknown learning method!\n");
			exit(EXIT_FAILURE);
	}
}

/* compute the centroid of the winner (or winners) and return that as the 
	output for the bmu. There could be cases where the centroid isn't
	in the actual correct data points and what not, I need to look at that */
void som_bmu_centroid(SOM *s, Symbol *p, int *row, int *col, int dx, int dy)
{
	int r, c;
	double dist;
	double best_dist_so_far = 999999;
	int num_matches = 0;
	Symbol *sym;
	double drow = 0.0, dcol = 0.0;

	for (r = 0; r < s->sd.rows; r++)
	{
		for (c = 0; c < s->sd.cols; c++)
		{
			sym = som_symbol_ref(s, r, c);

			/* TODO: Get a better distance function here. Euclidean distance
				starts to fail in higher dimensions. */
			dist = symbol_fdist(sym, p);
			
			/* if the neuron in question is so close to the symbol in question,
				them make sure I evenly pick one out of the entire set of 
				"too close" neurons */
			if (fabs(dist - best_dist_so_far) < 1e-15)
			{
				/* sum everything up for the averaging later... */
				drow += r;
				dcol += c;
				/* the divisor for the number of matches found */
				num_matches++;
				
#if 1
				/* mark as red the coincident symbols */
				glBegin(GL_POINTS);
					glColor3f(1.0, 0.0, 0.0);
					glVertex3f(c+dx, r+dy, .2);
				glEnd();
#endif
			}
			else if (dist < best_dist_so_far)
			{
				/* if it is obviously better, then just follow the gradient
					to the best it could possibly be */

				best_dist_so_far = dist;

				/* ASIDE: on unknown input, this give the only best match 
					for it */
				drow = r;
				dcol = c;
				
				/* force the calculation of the centroid of neurons close to
					THIS particular best_dist_so_far group of neurons. */
				num_matches = 1;
			
#if 1
				/* mark as green the symbols better than the last symbols */
				glBegin(GL_POINTS);
					glColor3f(0.0, 1.0, 0.0);
					glVertex3f(c+dx, r+dy, .2);
				glEnd();
#endif
			}
		}
	}

	/* find the centroid of the selections, if I only actually found a single
		best point, then there will be one match, and the centroid of one point
		is itself. */
	*row = (int) ((double)drow / (double)num_matches);
	*col = (int) ((double)dcol / (double)num_matches);

	/* XXX There is some bad news in here since the centroid method will fail 
		if the group of winning neurons are disjoint or concavely connected. */
}

/* find the 2d location of the symbol in the SOM which minimizes the distance
	between the input symbol and the neuron symbol */
void som_bmu_fixed(SOM *s, Symbol *p, int *row, int *col, int dx, int dy)
{
	int r, c;
	double dist;
	double best_dist_so_far = 9999999.0;
	int num_matches = 0;
	int rnd;
	int prob;
	Symbol *sym;

	for (r = 0; r < s->sd.rows; r++)
	{
		for (c = 0; c < s->sd.cols; c++)
		{
			sym = som_symbol_ref(s, r, c);
			dist = symbol_fdist(sym, p);
			
			/* if the neuron in question is so close to the symbol in question,
				them make sure I evenly pick one out of the entire set of 
				"too close" neurons */
			if (fabs(dist - best_dist_so_far) < 1e-15)
			{
				/* XXX BEGIN Experimental */
				/* Since I've changed the algorithm to integrate locations on
					the hyperplane as unique identifiers representing the 
					underlying symbols at that particular neuron, I need to
					stabalize the lookup to ONE best match out of any number
					of probable best match candidates. */
				if (s->mode == SOM_CLASSIFYING)
				{
					*row = r;
					*col = c;
					/* just pick the first best one found, don't do a random
						choice of similar winners */
					goto skip;
				}
				/* XXX END Experimental */

				/* if it is very close to the last point, randomly choose it */
				rnd = rand();
				prob = 1 + (int)(((float)num_matches*rnd)/(RAND_MAX+1.0));
				if (prob == 1)
				{
					*row = r;
					*col = c;
				}

				skip: /* XXX this tag is part of the experimental stuff */

				/* make sure to give 1/n probability for choosing any neuron 
					which is the same as any other neuron */
				num_matches++;
			}
			else if (dist < best_dist_so_far)
			{
				/* if it is obviously better, then just follow the gradient
					to the best it could possibly be */
				best_dist_so_far = dist;

				/* force a new probability group for the next set of neurons
					in this potential group of neurons near the possible
					best_dist_so_far. This will ensure that neurons out
					of the last group of neurons close to the LAST 
					best_dist_so_far are removed from consideration. The reason
					that num_matches is set to 2 and not 1 is that if the next
					neuron checked in within the tolerance of best_dist_so_far,
					then have a 50% probability for choosing the last
					point (this one in this case one iteration ago), or the 
					current point in the new best_dist_so_far group. */
				num_matches = 2;

				/* ASIDE: on unknown input, this give the only best match 
					for it */
				*row = r;
				*col = c;
				
			}
		}
	}
}

/* figure out how far along the SOM learning path we are, and construct
	all needed interpolants from scratch. return whether or not I trained
	or moved into classification mode, prow, and pcol contain the best matching
	unit regardless if in training or classification mode */
unsigned int som_learn(SOM *s, Symbol *p, int *prow, int *pcol, int dx, int dy,
	int request, int bmu_supplied)
{
	float rad;
	int srow, scol;
	int erow, ecol;
	int i, j, t0, t1;
	float dist;
	float g, l;
	float delta, t;
	Symbol *sym;

	/* figure out the best matching unit in context of the input p */
	if (bmu_supplied == FALSE) {
		// If we _don't_ supply the bmu, then calculate it given the SOM and
		// the input symbol and return it in the prow and pcol parameters.
		som_bmu(s, p, prow, pcol, SOM_BMU_METHOD_CENTROID, dx, dy);
	}

	/* Now that we determined where our BMU is, store it. This is so we can
		visualize it better after the fact and such */
	s->bmu_row = *prow;
	s->bmu_col = *pcol;

	/* if I've asked to train beyond my maximum iterations, then automatically
		move into classification mode */
	if (s->current_iter >= s->sd.train_iter)
	{
		/* XXX once I do this, this SOM classifies things forever and will never
			learn again. I really would like to fix this somehow. */
		s->mode = SOM_CLASSIFYING;
		return s->mode;
	}

	/* if this function has been asked to only classify something, then don't
		actually learn it and don't modify the current iteration since I want
		the learning field size to stay equal to what it was before. */
	if (request == SOM_REQUEST_CLASSIFY) {
		return s->mode;
	}

	/* figure out the learning neighborhood */

	/* recompute the delta from one training session to another */
	delta = 1.0 / ((float)s->sd.train_iter - 1);

	/* figure how far along the training path I am */
	t = (float)s->current_iter * delta;





	/* Decay model. TODO: Lift into SOMDesc. */

	/* how my neighborhood shrinks in unit space wrt how far I am in learning */
	/* LINEAR DECAY */
/*	rad = s->initial_radius * (1.0 - t); */

	/* SIGMOIDAL DECAY */
	/* the sigmoid goes from -1 to 1 in t steps... */
/*	rad = s->initial_radius * */
/*		(1.0 - (1.0 / (1.0 + exp(-5.0 * (-1.0 + (2.0 * t))))));*/

	/* EXPONENTIAL DECAY */
	/* Here we want it so that at the last iteration, the neighborhood size
		is .5 (since it cannot be zero due to transcendental mathematics).
		A neightborhood of .5 represents a neghborhood of
		one neuron (since we add/sub the value to find the convolution kernel),
		so that should be good.

		We'd like the final iteration to have a neighborhood of one. So:
		end = start * 2 ^ (-i / h);    (1) 

		start is the start neighborhood size (calcaulated earlier).
		end is the end neighborhood size when done (1).
		i is the train_iter for this SOM
		h is the half-life (which we need to compute).

		analytically solve for half-life h:
		h = -((log(2) * i) / log(end / start));  (2) 

		Then all the values are known for (2) and we put the value
		we compute for (2) into (1) as i marches towards the
		final iteration.

		h is computed in som_init() (aka half_life).

		Then as we compute the below, we mark our time passed in
		s->current_iter until we get to sd->train_iter at the end.

		This method focuses on quickly arranging the clusters in the
		high dimensional space, but then spending a good while refining
		the clusters to be the input features.
	*/
	rad = s->initial_radius * powf(2.0, (-s->current_iter / s->half_life));






	/* bound rad to a small value if it is less than a constant, this prevents
	 	the division in the loops from blowing up too badly. This makes a shell
		of 1e-15 size around the point in question. This is ok for now...
	*/
	if (fabs(rad) < 1e-15)
	{
		rad = 1e-15;
	}

	/* produce the clipped box set up with the radius from the
		x,y location to a midpoint on the box. I need to consider the
		neighbors.  this gets smaller and smaller each iteration
	*/

	srow = s->bmu_row - rad;
	if (srow < 0)
	{
		srow = 0;
	}

	scol = s->bmu_col - rad;
	if (scol < 0)
	{
		scol = 0;
	}

	erow = s->bmu_row + rad;
	if (erow >= s->sd.rows)
	{
		erow = s->sd.rows - 1;
	}

	ecol = s->bmu_col + rad;
	if (ecol >= s->sd.cols)
	{
		ecol = s->sd.cols - 1;
	}

	/* using a gaussian function, teach the neurons closer to the x,y
		point much more than the ones farther away. Also, depending on t,
		how much you learn is also scaled by how far along you are in the
		learning process */
	for (i = srow; i <= erow; i++)
	{
		for (j = scol; j <= ecol; j++)
		{
			/* ok, find the distance from the row, col, location to the 
				i, j location */
			t0 = i - s->bmu_row;
			t1 = j - s->bmu_col;
			dist = sqrt(t0*t0 + t1*t1);
			/* rad is bound to a non-zero value outside of the function.
				This is so I do not have to do a division by zero check here
				and can get better instruction data dependancy flow 
				through this inner loop.
			*/
			dist /= rad;

			/* ok, this is what the gaussian is going to give me for the
				neighbor points, l should be 1.0 if i and j equal x and y */
			g = exp((-1.0 * (dist*dist)) / .15);

			/* ok, now scale the gaussian by how far along the process we are */
			/* add the 1 to avoid divide by zero error */
			l = g / (t*4.0 + 1.0); /* XXX magical mult constant? */

			/* now move the i/j point closer to p */
			sym = som_symbol_ref(s, i, j);
			symbol_interpolate(sym, p, l);
		}
	}

	/* now update the parts of the som that know about the learning */
	s->current_iter++;

	return s->mode;
}

int som_get_rows(SOM *s)
{
	return s->sd.rows;
}

int som_get_cols(SOM *s)
{
	return s->sd.cols;
}

int som_get_bmu_row(SOM *s)
{
	return s->bmu_row;
}

int som_get_bmu_col(SOM *s)
{
	return s->bmu_col;
}

void som_free(SOM *s)
{
	int i;

	for(i = 0; i < (s->sd.rows * s->sd.cols); i++)
	{
		symbol_free(s->neuron[i]);
	}

	free(s->neuron);
	free(s->qmap);
	free(s);
}


/* draw the som at some offset */
void som_draw(SOM *s, unsigned int style, int x, int y)
{
	int default_quality = 4;

	glPushMatrix();
	switch(style)
	{
		case SOM_STYLE_ACTUAL:
			som_draw_actual(s, x, y);
			break;
		case SOM_STYLE_QUALITY:
			som_draw_quality(s, default_quality, x, y);
			break;
		default:
			printf("som_draw(): Unknown style: %d\n", style);
			exit(EXIT_FAILURE);
	}
	glPopMatrix();
}

static void som_draw_quality(SOM *s, unsigned int quality, int x, int y)
{
	int row, col, arow, acol;
	int srow, scol, erow, ecol;
	Symbol *candidate, *center;
	float dist, sum;
	int count;
	float v0, v1, v2, v3;

	/* for now, to increase speed, don't recompute it if I don't have to. */
	if (s->mode == SOM_LEARNING || 
		(s->mode == SOM_CLASSIFYING && s->final_computation == FALSE))
	{
		/* Hmm... what a shitty algorithm */

		s->max_dist = 0.0;

		/* compute the quality for each neuron */
		for(row = 0; row < s->sd.rows; row++)
		{
			for (col = 0; col < s->sd.cols; col++)
			{
				srow = row - quality;
				if (srow < 0)
				{
					srow = 0;
				}

				scol = col - quality;
				if (scol < 0)
				{
					scol = 0;
				}

				erow = row + quality;
				if (erow >= s->sd.rows)
				{
					erow = s->sd.rows - 1;
				}

				ecol = col + quality;
				if (ecol >= s->sd.cols)
				{
					ecol = s->sd.cols - 1;
				}

				center = som_symbol_ref(s, row, col);
				sum = 0;
				count = 0;
			
				/* look at the neighbors of the neuron based upon quality */
				for (arow = srow; arow < erow; arow++)
				{
					for (acol = scol; acol < ecol; acol++)
					{
						candidate = som_symbol_ref(s, arow, acol);
						dist = symbol_dist(candidate, center);
						sum += dist;
						count++;
					}
				}

				/* this is the averaged weight for this neuron */
				sum /= (float)count;
				if (sum > s->max_dist)
				{
					s->max_dist = sum;
				}

				s->qmap[SOM_ADR(row, col, s)] = sum;
			}
		}

		if (s->mode == SOM_CLASSIFYING) {
			s->final_computation = TRUE;
		}

	}

	/* now draw the quality field */

	/* a little inefficient, I really need to use textures... */
	glBegin(GL_QUADS);
	for(row = 0; row < s->sd.rows - 1; row++)
	{
		for (col = 0; col < s->sd.cols - 1; col++)
		{
			/* don't forget to normalize it */
			/* in this convolution, the neurons which are closest together will
				be white, and those farthest apart will be black */
			v0 = 1.0 - (s->qmap[SOM_ADR(row, col, s)] / s->max_dist);
			v1 = 1.0 - (s->qmap[SOM_ADR(row, col+1, s)] / s->max_dist);
			v2 = 1.0 - (s->qmap[SOM_ADR(row+1, col+1, s)] / s->max_dist);
			v3 = 1.0 - (s->qmap[SOM_ADR(row+1, col, s)] / s->max_dist);

			glColor3f(v0, v0, v0);
			glVertex3f(x + col, y + row, 0.0);

			glColor3f(v1, v1, v1);
			glVertex3f(x + col+1, y + row, 0.0);

			glColor3f(v2, v2, v2);
			glVertex3f(x + col + 1, y + row + 1, 0.0);

			glColor3f(v3, v3, v3);
			glVertex3f(x + col, y + row+1, 0.0);
		}
	}
	glEnd();
}

void som_draw_reticule(SOM *s, int x, int y, int row, int col)
{
	/* draw a little box around the specified location in x, y. It is
		colored yellow if the SOM is still learning and green otherwise. */

	glBegin(GL_LINE_LOOP);
	switch(s->mode)
	{
		case SOM_LEARNING:
			glColor3f(1, 1, 0);
			break;
		case SOM_CLASSIFYING:
			glColor3f(0, 1, 0);
			break;
		default:
			glColor3f(1, 0, 0);
		break;
	}
	glVertex3f(col-10+x, row-10+y, 0.5);
	glVertex3f(col-10+x, row+10+y, 0.5);
	glVertex3f(col+10+x, row+10+y, 0.5);
	glVertex3f(col+10+x, row-10+y, 0.5);
	glEnd();
}

static void som_draw_actual(SOM *s, int x, int y)
{
	int row, col;
	Symbol *sym;
/*	glBegin(GL_LINES);*/
/*		glColor3f(1, 1, 1);*/
/*		glVertex3f(x, y, 0.1);*/
/*		glVertex3f(x + s->sd.cols, y + s->sd.rows, 1.0);*/
/*	glEnd();*/

	glNormal3f(0, 0, 1);
	switch(s->sd.dim)
	{
		case 0:
			printf("oops, can't draw a zero dimensional som\n");
			exit(EXIT_FAILURE);
			break;

		case 1:
			glBegin(GL_QUADS);
			for(row = 0; row < s->sd.rows - 1; row++)
			{
				for (col = 0; col < s->sd.cols - 1; col++)
				{
					/* According to page 45 of the opengl book */
					/* V0 */
					sym = som_symbol_ref(s, row, col);
					glColor3f(sym->vec[0], sym->vec[0], sym->vec[0]);
					glVertex3f(x + col, y + row, 0.0);
					
					/* V1 */
					sym = som_symbol_ref(s, row, col+1);
					glColor3f(sym->vec[0], sym->vec[0], sym->vec[0]);
					glVertex3f(x + col + 1, y + row, 0.0);
		
					/* V2 */
					sym = som_symbol_ref(s, row + 1, col + 1);
					glColor3f(sym->vec[0], sym->vec[0], sym->vec[0]);
					glVertex3f(x + col + 1, y + row + 1, 0.0);
		
					/* V3 */
					sym = som_symbol_ref(s, row+1, col);
					glColor3f(sym->vec[0], sym->vec[0], sym->vec[0]);
					glVertex3f(x + col, y + row + 1, 0.0);
				}
			}
			glEnd();
			break;

		case 2:
			glBegin(GL_QUADS);
			for(row = 0; row < s->sd.rows - 1; row++)
			{
				for (col = 0; col < s->sd.cols - 1; col++)
				{
					/* According to page 45 of the opengl book */
					/* V0 */
					sym = som_symbol_ref(s, row, col);
					glColor3f(sym->vec[0], sym->vec[1], 0);
					glVertex3f(x + col, y + row, 0.0);
					
					/* V1 */
					sym = som_symbol_ref(s, row, col+1);
					glColor3f(sym->vec[0], sym->vec[1], 0);
					glVertex3f(x + col+1, y + row, 0.0);
		
					/* V2 */
					sym = som_symbol_ref(s, row + 1, col + 1);
					glColor3f(sym->vec[0], sym->vec[1], 0);
					glVertex3f(x + col + 1, y + row + 1, 0.0);
		
					/* V3 */
					sym = som_symbol_ref(s, row+1, col);
					glColor3f(sym->vec[0], sym->vec[1], 0);
					glVertex3f(x + col + 1, y + row+1, 0.0);
				}
			}
			glEnd();
			break;

		default:
			glBegin(GL_QUADS);
			for(row = 0; row < s->sd.rows - 1; row++)
			{
				for (col = 0; col < s->sd.cols - 1; col++)
				{
					/* According to page 45 of the opengl book */
					/* V0 */
					sym = som_symbol_ref(s, row, col);
					glColor3f(sym->vec[0], sym->vec[1], sym->vec[2]);
					glVertex3f(x + col, y + row, 0.0);
					
					/* V1 */
					sym = som_symbol_ref(s, row, col+1);
					glColor3f(sym->vec[0], sym->vec[1], sym->vec[2]);
					glVertex3f(x + col+1, y + row, 0.0);
		
					/* V2 */
					sym = som_symbol_ref(s, row + 1, col + 1);
					glColor3f(sym->vec[0], sym->vec[1], sym->vec[2]);
					glVertex3f(x + col + 1, y + row + 1, 0.0);
		
					/* V3 */
					sym = som_symbol_ref(s, row+1, col);
					glColor3f(sym->vec[0], sym->vec[1], sym->vec[2]);
					glVertex3f(x + col, y + row+1, 0.0);
				}
			}
			glEnd();
			break;

	}
}
