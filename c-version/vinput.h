#ifndef VISION_H

enum 
{
	VINPUT_RANGE_DOWN,
	VINPUT_RANGE_UP,
	VINPUT_RANGE_RANDOM
};


typedef struct VInput_s
{
	/* the size of the entire bitmap image */
	int full_bit_rows;
	int full_bit_cols;

	/* the size of a single glyph in the image */
	int subimage_bit_rows;
	int subimage_bit_cols;

	/* the number of single glyphs in a row */
	int rows;
	/* the number of single glyphs in a col */
	int cols;

	/* a pointer to the bits in question. */
	unsigned char *bits;
} VInput;


/* screws around with a global array and produces a structure representing it */
VInput* vinput_init(int sibr, int sibc, int rows, int cols);
void vinput_destroy(VInput *vinp);

/* This returns an array of symbols pointers subimage_bit_rows long that are
	pointing to subimage_dim_cols dimension symbols. The index goes from zero
	to rows * cols. The images are accessed from the upper left corner in a 
	left to right fashion from the large image! This is allocated memory and
	it is up to the user to deallocate the container whereas the cortex will
	assume control of the memory associated with the actual symbols */
Symbol** vinput_glyph(VInput *vinp, int index);

/* for a specified glyph location, get the specific bit */
int vinput_glyph_bit(VInput *vinp, int row, int col, int r, int c);

/* draw the input resolution of a reverse lookup */
void vinput_draw_irt(VInput *vinp, InputResTable *irt, int x, int y);

/* draw a glyph */
void vinput_draw_glyph(VInput *vinp, Symbol **glyph, int x, int y);

/* corrupt the glyph per_chance amount of time, and then if it is corrupted,
	then change a percentage of the pixels in per_pixels by a percentage
	range according to the range_style(either all up, all down, or random). */
void vinput_corrupt(VInput *vinp, Symbol **glyph, float per_pixels, 
	float per_range, float per_chance, int range_style);

#endif
