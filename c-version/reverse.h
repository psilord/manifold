#ifndef REVERSE_H
#define REVERSE_H

/* this holds the information needed for a single slot symbol to be expanded
	into multiple views of that slot's parents. This is mostly a bookeeping
	structure so I can keep track of everything on a per slot basis. */
typedef struct SlotExpansion_s
{
	/* the dimension of the slot(including all integrations) in question */
	int dim;

	/* The number of integrations for this slot */
	int ints;

	/* The dimension of a single input symbol for this slot, is 2 if the parent
		of this slot is another section, is 'input dimension' if the parent
		is an input channel */
	int inputdim;

	/* The parent id and parent kind for this slot */
	int parent_id;
	int kind;

	/* The actual symbols representing a particular (integrated)slot's
		info going backwards in time(from index 0). This is filled in AFTER 
		the view symbols have been converted into slot symbols. num_sym
		should be the same as the number of views in the viewpoint. */
	int num_sym;
	Symbol **sym;
	
	/* the expanded views associated with this slot which can be computed once
		everything above in this structure is completed. This is what gets
		written into the wavefront nodes after this is all set up */
	ViewPoint *expview;

} SlotExpansion;

SlotExpansion *sexp_init(int num_slots);
void sexp_destroy(SlotExpansion *sexp, int num_slots);

#endif
