#include "glyphs.h"
#include "common.h"
#include <GL/gl.h>
#include <GL/glu.h>

/* The images are accessed from the upper left corner of the large image! */
VInput* vinput_init(int sibr, int sibc, int rows, int cols)
{
	VInput *vinp = NULL;

	vinp = (VInput*)xmalloc(sizeof(VInput) * 1);

	vinp->full_bit_rows = glyphs_height;
	vinp->full_bit_cols = glyphs_width;

	vinp->subimage_bit_rows = sibr;
	vinp->subimage_bit_cols = sibc;

	vinp->rows = rows;
	vinp->cols = cols;

	/* point it to the global array */
	vinp->bits = glyphs_bits;

	if ((vinp->subimage_bit_rows * vinp->rows) != vinp->full_bit_rows)
	{
		printf("vinput_init(): Row mismatch!\n");
		exit(EXIT_FAILURE);
	}

	if ((vinp->subimage_bit_cols * vinp->cols) != vinp->full_bit_cols)
	{
		printf("vinput_init(): Col mismatch!\n");
		exit(EXIT_FAILURE);
	}

	return vinp;
}

void vinput_destroy(VInput *vinp)
{
	free(vinp);
}

/* return an array of the image where the length of the array is the number
	of rows in the image, and the dimension of the symbol is the number of
	columns. I'm returning horizontal scanlines of the image. */
Symbol** vinput_glyph(VInput *vinp, int index)
{
	int irow;
	int icol;
	int i;
	int r, c;
	Symbol **glyph;
	float bitval;
	
	/* the location on the 2D glyph map where this index resides */
	irow = index / vinp->rows;
	icol = index % vinp->cols;

	/* ok, allocate the symbol structure I'm going to fill up with 
		the glyph data */
	glyph = (Symbol**)xmalloc(sizeof(Symbol*) * vinp->subimage_bit_rows);
	for (i = 0; i < vinp->subimage_bit_rows; i++)
	{
		glyph[i] = symbol_init(vinp->subimage_bit_cols);
	}

	/* now, pick out the bits for the glyph I want out of the large image
		and stuff it into the thing I just made */
	for (r = 0; r < vinp->subimage_bit_rows; r++)
	{
		for (c = 0; c < vinp->subimage_bit_cols; c++)
		{	
			/* retrieve the requested bit value from the requested glyph */
			bitval = vinput_glyph_bit(vinp, irow, icol, r, c);
			symbol_set_index(glyph[r], bitval, c);
		}
	}

	return glyph;
}

/* for a specified glyph location, get the specific bit */
int vinput_glyph_bit(VInput *vinp, int row, int col, int r, int c)
{
	int corner, bindex;
	int elem;
	unsigned char notch;
	int bitval;

	/* the location of the bit in question as if the gigantic bit array
		was one long sequence of bits (and it is) */

	/* find the upper left hand corner of the glyph in the mythical bit array */
	corner = (row * ((vinp->subimage_bit_rows * vinp->subimage_bit_cols) * vinp->cols)) + 
				(col * vinp->subimage_bit_cols);

	/* should be the index to the actual bit in the bit array */
	bindex = corner + (r * (vinp->subimage_bit_cols * vinp->cols)) + c;

	/* now convert that index into the stride of the unsigned char
		representation of the actual array */

	elem = bindex / (8 * sizeof(unsigned char));

	/* umm... this is funny. I think 'bitmap' is storing the bitmaps in a wierd
		LSB to MSB order... */
	notch = 0x01 << (c % 8);

	bitval = vinp->bits[elem] & notch;

	/* Since I have either a nonzero number in bitval, representing that
		some bit had been turned on, or all bits off, meaning zero, encode
		that fact here in a trinary. */
	return bitval?1:0;
}

/* draw a single glyph */
void vinput_draw_glyph(VInput *vinp, Symbol **glyph, int x, int y)
{
	int i, j;
	float val;

	glPointSize(5.0);
	glBegin(GL_POINTS);

	for(i = 0; i < vinp->subimage_bit_rows; i++)
	{
		for(j = 0; j < vinp->subimage_bit_cols; j++)
		{
			symbol_get_index(glyph[i], &val, j);
			glColor3f(val, val, val);
			glVertex3f(x + j*5, y - i*5, 0.0);
		}
	}

	glEnd();
	glPointSize(1.0);
}

/* draw the timeslices of an input resolution */
void vinput_draw_irt(VInput *vinp, InputResTable *irt, int x, int y)
{
	int input;
	int pixel;
	float color;

	/* for now, just draw time slice zero. The null inputs get a blue
		shade, error will be a red 'x' over the "pixel" whose intensity
		is modulated by the actual error value */

	glPointSize(5.0);
	glBegin(GL_POINTS);
	for (input = 0; input < irt->num_inres; input++) {
		if (irt->inres[input].active == TRUE) {
			for (pixel = 0; pixel < vinp->subimage_bit_cols; pixel++)
			{
				symbol_get_index(irt->inres[input].resolution[0], 
						&color, pixel);
				glColor3f(color, color, color);
				/* flip the y axis */
				glVertex3f(x + pixel*5, y - input*5, 0.0);
			}
		}
		else
		{
			for (pixel = 0; pixel < vinp->subimage_bit_cols; pixel++)
			{
				glColor3f(1.0, 0, 0);

				/* flip the y axis */
				glVertex3f(x + pixel*5, y - input*5, 0.0);
			}
		}
	}
	glEnd();
	glPointSize(1.0);
}

void vinput_corrupt(VInput *vinp, Symbol **glyph, float per_pixels, 
    float per_range, float per_chance, int range_style)
{
	int i, j;
	double rnd;
	float *vals;
	int num;

	/* allocate something to place the glyphs vals into for messing up */
	num = vinp->subimage_bit_cols;
	vals = (float*)xmalloc(sizeof(float) * num);

	/* see if I affect the glyph at all */
	rnd = drand48();
	if (rnd < per_chance)
	{
		/* mess up each pixel row by row*/
		for (i = 0; i < vinp->subimage_bit_rows; i++)
		{
			symbol_get(glyph[i], vals, num);
			/* look at each pixel */
			for (j = 0; j < num; j++)
			{
				/* see if the pixel needs messing up */
				rnd = drand48();
				if (rnd < per_pixels)
				{
					/* see how much I should affect the pixel */
					rnd = drand48() * per_range;
					switch(range_style)
					{
						case VINPUT_RANGE_DOWN:
							/* calculate the new value based upon 
								movement of this value down by the percentage
								specfied toward zero */
							vals[j] = vals[j] - vals[j]*rnd;
							break;

						case VINPUT_RANGE_UP:
							/* calculate the new value based upon 
								movement of this value up by the percentage
								specfied toward one */
							vals[j] = vals[j] + (1.0 - vals[j])*rnd;
							break;

						case VINPUT_RANGE_RANDOM:
							if (drand48() < .5) {
								vals[j] = vals[j] - vals[j]*rnd;
							} else {
								vals[j] = vals[j] + (1.0 - vals[j])*rnd;
							}
							break;

						default:
							printf("vinput_corrupt(): Unknown range!\n");
							exit(EXIT_FAILURE);
					}
				}
			}
			symbol_set(glyph[i], vals, num);
		}
	}

	free(vals);
}
























