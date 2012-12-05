#ifndef CONV_H
#define CONV_H

/* what kind of convolution would I like? */
enum
{
	/* param is how much the rest of the neurons join in the blur */
	CONV_BLUR_SMEAR, 

	/* param val is how severe the blur effect is */
	CONV_BLUR_LIGHT,

	/* param val controls how much detail */
	CONV_ENHANCED_DETAIL,

	/* param val controls how soft */
	CONV_SOFTEN,

	/* param val controls how severe the edge detection is */
	CONV_LAPLACIAN,

	/* param val is ignored */
	CONV_USER_DEF
};

/* A tolerance for stabalization purposes when the divisor is close to zero */
#define CONV_TOL (1e-13)

/* A simple convolution description */
typedef struct Conv_s
{
	/* how do I identify what kind of convolution this is */
	char *desc; /* do not free this pointer */
	int type;

	/* the convolution itself */
	int rows, cols;
	double *coef;

} Conv;

#define CONV_COEF(row, col, conv) (((row) * ((conv)->cols)) + (col))

/* initialize a convolution for me */
Conv* conv_init(int style, double param);

/* get rid of a convolution */
void conv_free(Conv *conv);

/* apply a convolution to a som */
void conv_apply(SOM *som, Conv *c, double thld);

/* this is so I can make them myself, if I wanted */
double conv_get_coef(int row, int col, Conv *c);
void conv_set_coef(int row, int col, Conv *c, double param);

/* get the size of the convolution */
double conv_get_rows(Conv *c);
double conv_get_cols(Conv *c);

/* An all signing, all dancing application of a convolution style to a SOM. 
	This is generally what you want to use if you're just using an available
	convolution style. */
void conv_apply_easy(SOM *som, int style, double param, double thld);

#endif


