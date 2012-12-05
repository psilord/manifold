#include "common.h"

/* This file embodies the implementation of the wave table and the 
	reverse lookup algorithm for the cortex */

/* when I click on the cortex space, which section, if any, do I hit? */
static Section* locate_section_by_coords(Cortex *core, int row, int col,
											int *sec_row, int *sec_col);

/* given a serial_id, find the lcoation of it in the wavetable */
static int wavefront_locate_serial_id(Cortex *core, int serial_id);


/* dealing with the wavefront as it expands in the wave table. */
static int wavetable_expanding(Cortex *core);
static void wavetable_merge(Cortex *core);
static void wavetable_expand(Cortex *core);
static void wavetable_delete_views(ViewPoint *vlist);
static ViewPoint* wavetable_centroid_join(ViewPoint *vlist);
static int wavetable_views_consistant(ViewPoint *vlist);
static int wavefront_expandable(WaveFront *wf, Cortex *core);
static void wavefront_expand_a_node(int wfloc, Cortex *core);
static InputResTable* wavetable_generate_irt(Cortex *core);

/* set up a suitable wave propogation table for reverse lookups based upon
	a supplied cortex */
void wavetable_init(Cortex *core)
{
	int i;
	int index;

	/* for purposes of the wave propogation table, the input sections and
		normal sections shuld both be counted */
	core->wt.num_sec = core->num_input + core->num_sec;

	/* allocate memory for each place a wavefront may exist */
	core->wt.wfs = (WaveFront*)xmalloc(sizeof(WaveFront) * core->wt.num_sec);

	/* initialize the wavefronts to be specific to a section or input
		channel */

	index = 0;

	/* set up the input channels first */
	for (i = 0; i < core->num_input; i++)
	{
		core->wt.wfs[index].serial_id = core->input[i].serial_id;
		core->wt.wfs[index].active = FALSE;
		core->wt.wfs[index].needed_observations = 0;
		core->wt.wfs[index].num_observations = 0;
		core->wt.wfs[index].views = NULL;
		index++;
	}

	/* continue with the sections */
	for (i = 0; i < core->num_sec; i++)
	{
		core->wt.wfs[index].serial_id = core->sec[i].serial_id;
		core->wt.wfs[index].active = FALSE;
		core->wt.wfs[index].needed_observations = 0;
		core->wt.wfs[index].num_observations = 0;
		core->wt.wfs[index].views = NULL;
		index++;
	}

	if (index != core->wt.num_sec)
	{
		printf("wavetable_init(): Algorithm Failure! "
			"index(%d) != core->wt.num_sec(%d)\n", index, core->wt.num_sec);
		exit(EXIT_FAILURE);
	}
}

/* remove all views and reset everything to not active in preparation for
	a reverse lookup */
void wavetable_reset(Cortex *core)
{
	int i;

	/* for each of the WaveFronts, make sure there are no views, and if there
		are delete them as well, but leave the serial_id fields alone */
	for (i = 0; i < core->wt.num_sec; i++)
	{
		core->wt.wfs[i].active = FALSE;
		core->wt.wfs[i].needed_observations = 0;
		core->wt.wfs[i].num_observations = 0;

		/* if there are any views, get rid of them, the wave table
			owns all symbol memory given to it.  */
		wavetable_delete_views(core->wt.wfs[i].views);
		core->wt.wfs[i].views = NULL;
	}
}

/* delete the list of viewpoints given to me */
void wavetable_delete_views(ViewPoint *vlist)
{
	int j;
	ViewPoint *current, *deleted;

	current = vlist;
	while(current != NULL)
	{
		deleted = current;
		current = current->next;

		/* delete the symbols in the view and then the view */
		for (j = 0; j < deleted->num_sym; j++)
		{
			/* get rid of the observations */
			if (deleted->sym[j] != NULL)
			{
				symbol_free(deleted->sym[j]);
				deleted->sym[j] = NULL;
			}
		}
		/* get rid of the observation array */
		if (deleted->sym != NULL)
		{
			free(deleted->sym);
			deleted->sym = NULL;
		}
		deleted->next = NULL;

		/* get rid of the actual ViewPoint */
		free(deleted);
	}
}

void wavetable_destroy(Cortex *core)
{
	/* clean up some stuff */
	wavetable_reset(core);

	/* get rid of the rest */
	free(core->wt.wfs);
	core->wt.wfs = NULL;
}

/* given a x,y location on the screen, figure out if it is in a section, if
	so, then which one and set the sec_row and sec_col to be the relative
	location in the section that was selected. */
Section* locate_section_by_coords(Cortex *core, int row, int col, 
	int *sec_row, int *sec_col)
{
	int i;
	int srow, erow, scol, ecol;

	for (i = 0; i < core->num_sec; i++)
	{
		srow = core->sec[i].y;
		scol = core->sec[i].x;

		erow = srow + som_get_rows(core->sec[i].som);
		ecol = scol + som_get_cols(core->sec[i].som);

		/* see if the click is within the bounds of the section */
		if ((row >= srow && row < erow) &&
			(col >= scol && col < ecol))
		{
			*sec_row = row - srow;
			*sec_col = col - scol;
			return &core->sec[i];
		}
	}

	return NULL;
}

/* find the location of the serial_id in question in the wavefront table */
int wavefront_locate_serial_id(Cortex *core, int serial_id)
{
	int i;

	for (i = 0; i < core->wt.num_sec; i++)
	{
		if (core->wt.wfs[i].serial_id == serial_id)
		{
			return i;
		}
	}

	return NOT_FOUND;
}

/* this function takes as input a cortex and an x,y location and returns back
	the input necessary (if applicable) to match the feature field on the
	section which was queried */
InputResTable* cortex_resolve(Cortex *core, int row, int col)
{
	InputResTable *irt = NULL;
	Section *sec = NULL;
	int sec_row, sec_col;
	int i, obindex;
	Symbol *pos;
	ViewPoint *vp;
	int wavloc;

	/* determine what section the row,col pair for the "cortex space" falls
		into and where in that section it explicitly lies. */
	sec = locate_section_by_coords(core, row, col, &sec_row, &sec_col);
	if (sec == NULL)
	{
		/* No section at this row, col location. */
		return NULL;
	}

/*	printf("Found section: %d (%d, %d)\n", sec->serial_id, sec_row, sec_col);*/

	/* Now that we know what section we have, lookup the observation table
		associated with it, and set up the wave table with this knowledge. */
	obindex = -1;
	for (i = 0; i < core->rt.num_roots; i++)
	{
		if (core->rt.root[i].serial_id == sec->serial_id)
		{
			obindex = i;
			break;
		}
	}
	if (obindex == -1)
	{
		printf("cortex_resolve(): logic error! No observation table found!\n");
		exit(EXIT_FAILURE);
	}

	/* This sets up how many times each node in the participating reverse
		lookup subtree(at obindex) must be viewed for a wavefront to get 
		merged when multiple views happen at a wavefront. */
	for (i = 0; i < core->rt.root[obindex].num_obs; i++)
	{
		/* find the sections/inputs which the wavefront will eventually pass */
		wavloc = wavefront_locate_serial_id(core, 
				core->rt.root[obindex].ob[i].serial_id);

		if (wavloc != NOT_FOUND)
		{
			/* and copy the number of observations into the wavefront */
			core->wt.wfs[wavloc].needed_observations = 
				core->rt.root[obindex].ob[i].obs;
		}
		else
		{
			printf("cortex_resolve(): Logic error, didn't find a wavefront\n");
			exit(EXIT_FAILURE);
		}
	}

	/* now that the needed observations have been set up, seed the wavetable
		with the initial mouse click position information */
	wavloc = wavefront_locate_serial_id(core, sec->serial_id);
	if (wavloc != NOT_FOUND)
	{
		/* found it */
		core->wt.wfs[wavloc].active = TRUE;

		/* inject the initial viewpoint, which is the user's viewpoint */

		pos = symbol_init(2);
		/* normalize the location with repsect to the section, this is how all
			of this type of information is stored in the Cortex proper */
		symbol_set_2(pos, 
							(double)sec_row / (double)som_get_rows(sec->som),
							(double)sec_col / (double)som_get_cols(sec->som));
					
		vp = (ViewPoint*)xmalloc(sizeof(ViewPoint) * 1);
		vp->num_sym = 1;
		vp->sym = (Symbol**)xmalloc(sizeof(Symbol*) * vp->num_sym);
		vp->sym[0] = pos;
		vp->next = NULL;

		/* Start the propogation */
		core->wt.wfs[wavloc].views = vp;
		/* I have observed it via the mouse click so mark that down */
		core->wt.wfs[wavloc].num_observations = 1;
	}
	else
	{
		printf("cortex_resolve(): Logic error, didn't find a wavefront\n");
		exit(EXIT_FAILURE);
	}
	
	/* ok, now perform the merge/expand procedure until there are no 
		more merges, and no more expansions. Once this is done, the only 
		active things left in the wavetable will be input sections which 
		were unexpandable since they have no parents. */
	while(wavetable_expanding(core) == TRUE)
	{
		/* merge all active serial_ids which have hit the max num of 
			observations, previously merged things will be ignored */
		wavetable_merge(core);

		/* for things which have hit the max num of observations and have
			been merged into a single view, then expand it. It is 
			possible that this can be a noop, in which case the wave front
			has stopped expanding and we exit the loop. */
		wavetable_expand(core);
	}

	/* now that that is finished, transform the wave front results into
		an input table form for the user. We give back the views associated
		with the inputs in the order of the inputs as defined in the cortex
		file. */
	irt = wavetable_generate_irt(core);
	
	/* After a wavefront expansion, there will be a LOT of garbage in the
		wavetable. This garbage collects all that shit */
	wavetable_reset(core);

	return irt;
}

/* if a merge is able to happen, or the wave front is still able to 
	expand, then return true, otherwise, return false */
int wavetable_expanding(Cortex *core)
{
	int i, kind;

	/* if any active serial_ids have section parents, then return true */
	for (i = 0; i < core->wt.num_sec; i++)
	{
		if (core->wt.wfs[i].active == TRUE)
		{
			/* found an active one, if it is not an input channel, then
				it has parents, so return true since the wavefronts must
				terminate at the inputs */
			kind = which_kind_of_emitter(core->wt.wfs[i].serial_id,
					core->sec, core->num_sec, core->input, core->num_input);
			switch(kind)
			{
				case CORTEX_KIND_SECTION:
					/* this must have parents, so the wave front can still
						propogate */
					return TRUE;
					break;

				case CORTEX_KIND_INPUT:
					/* ignore this one, if all are ignored, then later cases
						get checked */
					break;
				
				case CORTEX_KIND_UNKNOWN:
					printf("wavetable_expanding(): logic error! Unkown id\n");
					exit(EXIT_FAILURE);
					break;
			}
		}
	}

	/* if all active serial_ids are true inputs(and they have to be to reach 
		this part of the code), then if they have multiple views left, the 
		wave is still expanding (it needs merging) so return true */
	for (i = 0; i < core->wt.num_sec; i++)
	{
		if (core->wt.wfs[i].active == TRUE)
		{
			/* this is a logic error, an active serial_id MUST have a view */
			if (core->wt.wfs[i].views == NULL)
			{
				printf("wavetable_expanding(): "
						"active serial_id with no views!\n");
				exit(EXIT_FAILURE);
			}

			if (core->wt.wfs[i].views->next != NULL)
			{
				/* this serial_id had more than one view, meaning it is in
					need of merging, so wavefront is still advancing/merging. */
				return TRUE;
			}
		}
	}
	
	/* if only input nodes are active, and they all have one view, then 
		the wave has reached the inputs and finally merged the inputs to
		what they should be. At this point the wave is incapable of expanding
		anymore, it is finished */
	return FALSE;
}

/* if any active wave front in the wave table which has num_observations 
	equal to needed_observations with more than one view exists, then perform a 
	centroid joining algorithm on the views so there is only one view left. */
void wavetable_merge(Cortex *core)
{
	int i;
	ViewPoint *mvp;

	/* for each serial_id which has the needed number of observations, 
		perform a centroid join on all the views leaving only one view */
	for (i = 0; i < core->wt.num_sec; i++)
	{
		if (core->wt.wfs[i].active == TRUE)
		{
			if (core->wt.wfs[i].num_observations == 
				core->wt.wfs[i].needed_observations)
			{
				/* sanity check */
				if (core->wt.wfs[i].views == NULL)
				{
					printf("wavetable_merge(): Can't merge null views!\n");
					exit(EXIT_FAILURE);
				}

				/* only merge if I actually have to */
				if (core->wt.wfs[i].views->next != NULL)
				{
					/* do the centroid join, and replace the list of views with
						the merged view */
					mvp = wavetable_centroid_join(core->wt.wfs[i].views);
					wavetable_delete_views(core->wt.wfs[i].views);
					core->wt.wfs[i].views = mvp;
				}
			}
		}
	}
}

/* perform a centroid join on each time slot taking care to get the divisor
	right because varying time slots could have different amounts of 
	divisors based on how many symbols participated in the centroid
	calculation of that time slot */
ViewPoint* wavetable_centroid_join(ViewPoint *vlist)
{
	int max_time = 0;
	int num_participating;
	ViewPoint *current;
	int index;
	Symbol **sequence;
	Symbol *sym;
	int ok;
	ViewPoint *nvp;
	int dim;
	int num;

	/* check to make sure that all symbols in all views have the same 
		dimensionality. */
	ok = wavetable_views_consistant(vlist);
	if (ok == FALSE)
	{
		printf("wavetable_centroid_join(): A vlist did not have the same "
				"dmensional symbols in all viewpoints!\n");
		exit(EXIT_FAILURE);
	}

	/* record the dimensional size of the what will be the merged views */
	dim = symbol_get_dim(vlist->sym[0]);

	/* figure out the longest (in time) sequence there is, and make a
		divisor array which will hold how many time each symbol going
		backwards in time was joined. After I add up each symbol in time,
		then divide by the divisor calculated for that time. */

	/* figure out the longest time sequence */
	num = 0;
	for(current = vlist; current != NULL; current = current->next)
	{
		/* Here num_sym represents a look backwards in time... */
		if (current->num_sym > max_time)
		{
			max_time = current->num_sym;
		}
		num++;
	}

	/* now that I've figured out the longest time sequence, allocate a
		container array to hold all of the symbols I need, this memory will 
		end up in the final viewpoint, so I don't have to deallocate this in 
		this function. */
	sequence = (Symbol**)xmalloc(sizeof(Symbol*) * max_time);

	/* now, index will be a slice across the time slots in the viewpoints 
		where I add up the available symbols and divide by how many I found 
		at that index across the viewpoints. It is possible that the index is 
		greater than some time sequences in a view point in which case that 
		viewpoint does not participate in the join anymore. */
	for(index = 0; index < max_time; index++)
	{
		num_participating = 0;

		/* each index will have a new symbol associated with it, it should
			never be the case that this symbol gets made, but no viewpoints
			are able to use it. */
		sym = symbol_init(dim);
		symbol_zero(sym);

		/* check for pariticipating viewpoints at this index level... */
		for(current = vlist; current != NULL; current = current->next)
		{
			if (index < current->num_sym)
			{
				/* this viewpoint is participating, so include the symbol
					at the index in the centroid */
				symbol_add(sym, sym, current->sym[index]);
				num_participating++;
			}
		}

		/* I ALWAYS should have seen something participate. */
		if (num_participating == 0)
		{
			printf("wavetable_centroid_join(): No participating views?!?\n");
			exit(EXIT_FAILURE);
		}

		/* when this final step of the centroid is done, the 
			resulting symbol will still be within the convex
			hull of all points participating symbol at a
			particular index, meaning it will still be in
			the normalization range of 0 to 1, since all
			symbols are in that range */
		symbol_div(sym, num_participating);

		/* put the newly merged symbol into the merging sequence list */
		sequence[index] = sym;
	}

	/* finally make the single viewpoint which represents the time slice joined
		symbols */
	nvp = (ViewPoint*)xmalloc(sizeof(ViewPoint) * 1);
	nvp->num_sym = max_time;
	nvp->sym = sequence;
	nvp->next = NULL;

	return nvp;
}

/* Make sure that all symbols contained in all viewpoints in this list
	are of the exact same dimension. */
int wavetable_views_consistant(ViewPoint *vlist)
{
	ViewPoint *current;
	int dim;
	int i;

	/* small sanity check */
	if (vlist->sym == NULL || vlist->sym[0] == NULL)
	{
		printf("wavetable_views_consistant(): No symbols in view found!\n");
		exit(EXIT_FAILURE);
	}

	/* get the dimension of the first symbol in the first viewpoint */
	dim = symbol_get_dim(vlist->sym[0]);

	/* now all symbols in all viewpoints must be the same */
	for(current = vlist; current != NULL; current = current->next)
	{
		for (i = 0; i < current->num_sym; i++)
		{
			if (symbol_get_dim(current->sym[i]) != dim)
			{
				return FALSE;
			}
		}
	}

	return TRUE;
}

/* if a node has num_observations equal to needed_observations and there is
	only a single view and the node has parents(is not an input channel),
	then expand the wavefront to the parents(creating the required 
	number of views in the right time order), and mark the node as not active
	anymore since the wavefront has now passed it. The new nodes placed into 
	the system are marked active. Be careful that I don't start expanding
	nodes put into the system as a result of the expansion of the initial
	set of nodes. Only expand the initial set of nodes found. */
void wavetable_expand(Cortex *core)
{
	int i, ind;
	int num_expandable;
	int *expandable;

	/* count up how many are expandable, if none, then just return, and
		the loop performing the propogations will stop the expansion */
	num_expandable = 0;
	for (i = 0; i < core->wt.num_sec; i++)
	{
		if (wavefront_expandable(&core->wt.wfs[i], core) == TRUE)
		{
			num_expandable++;
		}
	}

	if (num_expandable == 0)
	{
		/* There isn't anything to expand, so just leave and let the caller
			figure it out, it could be that merging needs to happen... */
		return;
	}

	/* now, make a cache of indicies in the wavefront array of places that
		need expansion. I expand from this list only so I don't accidentily
		start expanding stuff I'm in the middle of expanding which is a danger
		if I just walked all of the possible wavefronts blindly */
	expandable = (int*)xmalloc(sizeof(int) * num_expandable);
	ind = 0;
	for (i = 0; i < core->wt.num_sec; i++)
	{
		if (wavefront_expandable(&core->wt.wfs[i], core) == TRUE)
		{
			/* record the location in the wfs array of the wavefront that needs
				expansion */
			expandable[ind] = i;
			ind++;
		}
	}

	/* walk the cache, expanding each wavefront one edge farther */
	for(i = 0; i < num_expandable; i++)
	{
		/* now, expand a single wavefront index */
		wavefront_expand_a_node(expandable[i], core);
	}

	free(expandable);
}

/* given an index in the wavefront table, expand that node back into
	the wavetable in terms of its parents. It should never be the case
	that this function is called on an input node.

	The symbols in the views (for non-input serial_ids) are actually
	locations on the SOM associated with the serial_id in question. So,
	decode the 2d locations into the actual som symbols found at those
	locations, and then for each one of those symbols, unabstract it
	for each slot, and then unabstract each unabstraction to regain the
	time sensitive symbol input to that slot for all slots. 
	
	NOTE: Be aware of the fact that each integration from a parent which is a
		section is always (2 * num_integrations) dimensions, but from a 
		true input serial id, it is (input dimension * num_integrations). 
		This is important when the various unabstractions of the value 
		contained in the section is performed. */

void wavefront_expand_a_node(int wfloc, Cortex *core)
{
	int loc, ploc;
	int i, j, index;
	float nrow, ncol; /* normalized form of row, col */
	int row, col; /* som space form of row, col */
	int kind;
	Symbol *lookup;
	/* the dimension of each integrated slot in a SOM, ordered by slots. I 
		need this to be able to cut up the som symbol into slot symbols */
	int *slotdims;
	/* the array used to break apart the integrations of a single slot into
		actual input symbols */
	int *intdims;
	Symbol **unabs;

	/* a holder for each slot's time sliced symbols */
	SlotExpansion *sexp;

	/* This is the core->sec index of the section we are expanding */
	loc = find_section_by_id(core->wt.wfs[wfloc].serial_id, core->sec, 
			core->num_sec);

	/* since we can only expand a section, and not an input, we initially
		assume that the symbols stored in the views are of positions on this 
		particular section. So, lets convert them first into actual symbols
		found at those sections. NOTE: Be careful of row/col order. Also,
		there should only be ONE view to work with since the merge phase
		should have set that up, but I'll check it just to be sure */

	if (core->wt.wfs[wfloc].views->next != NULL)
	{
		printf("wavefront_expand_a_node(): Can't expand nonmerged node!\n");
		exit(EXIT_FAILURE);
	}
	
	/* convert the location symbols in the view list of the wave front 
		into actual data symbols by looking them up in the SOM associated with
		this wavefront. */

	for(i = 0; i < core->wt.wfs[wfloc].views->num_sym; i++)
	{
		symbol_get_2(core->wt.wfs[wfloc].views->sym[i], &nrow, &ncol);
		/* denormalize the data into a true position on the 2d hypersurface */
		/* XXX possible fencepost error(array bounds) in the denormalization? */
		row = (int)(som_get_rows(core->sec[loc].som) * nrow);
		col = (int)(som_get_cols(core->sec[loc].som) * ncol);
		/* convert the view symbol into a slot symbol */
		lookup = symbol_copy(som_symbol_ref(core->sec[loc].som, row, col));
		symbol_free(core->wt.wfs[wfloc].views->sym[i]);
		core->wt.wfs[wfloc].views->sym[i] = lookup;
	}

	/* set up the SlotExpansion structures for each slot I'm going to expand */
	sexp = sexp_init(core->sec[loc].receptor.num_slot);

	/* figure out some information about each slot in the section */
	
	/* used for unabstracting the slots later */
	slotdims = (int*)xmalloc(sizeof(int) * core->sec[loc].receptor.num_slot);

	for(i = 0; i < core->sec[loc].receptor.num_slot; i++)
	{
		/* How I interpret the dimensionality of the slots depends upon
			what kind of entity wrote into it. If a section, then the
			dimension of this entry in the slot is simply the integrations of
			this slot * 2(row,col location on previous parent), but if it is 
			a true input node, then it is integrations * the input dimension. 
		*/

		kind = which_kind_of_emitter(core->sec[loc].receptor.slot[i].parent,
			core->sec, core->num_sec, core->input, core->num_input);
		switch(kind)
		{
			case CORTEX_KIND_SECTION:
				sexp[i].inputdim = 2;
				sexp[i].kind = CORTEX_KIND_SECTION;
				break;

			case CORTEX_KIND_INPUT:
				ploc = 
					find_input_by_id(core->sec[loc].receptor.slot[i].parent,
						core->input, core->num_input);
				sexp[i].inputdim = core->input[ploc].dim;
				sexp[i].kind = CORTEX_KIND_INPUT;
				break;

			case CORTEX_KIND_UNKNOWN:
				printf("wavefront_expand_a_node(): Unknown input/section?!?\n");
				exit(EXIT_FAILURE);
				break;

			default:
				printf("wavefront_expand_a_node(): Shouldn't happen\n");
				exit(EXIT_FAILURE);
				break;
		}

		/* copy over who the parent of this slot is */
		sexp[i].parent_id = core->sec[loc].receptor.slot[i].parent;

		/* copy over the number of integrations for this slot */
		sexp[i].ints = 
			/* yes, that is the SAME 'i' on both sides of the '=' because there
				are the same number of elements in the sexp array as there
				are slots in the section, and they are ordered the same way. */
			intqueue_integrations(core->sec[loc].receptor.slot[i].iq);

		/* figure out the dimension of the slot(this contains all integrations
			of the input) */
		sexp[i].dim = sexp[i].ints * sexp[i].inputdim;

		/* allocate storage for the time slices of individual integration
			slots */
		sexp[i].num_sym = core->wt.wfs[wfloc].views->num_sym;
		sexp[i].sym = (Symbol**)xmalloc(sizeof(Symbol*) * sexp[i].num_sym);

		/* now that everything above has been set up, set up a ViewPoint
			which will eventually contain the expanded symbols for a
			particular slot. This viewpoint will be an observation for the
			parent */
		sexp[i].expview = (ViewPoint*)xmalloc(sizeof(ViewPoint) * 1);

		/* in the view point, set up a container array for the fully expanded
			symbols for this slot (when they get completed, they will be 
			written into here) */
		sexp[i].expview->num_sym = sexp[i].ints * sexp[i].num_sym; 
		sexp[i].expview->sym = (Symbol**)xmalloc(sizeof(Symbol*) * 
			sexp[i].expview->num_sym);
		sexp[i].expview->next = NULL;

		/* this array is used for symbol_unabstract to convert the SOM 
			symbol into a set of slots symbols. */
		slotdims[i] = sexp[i].dim;

	}

	/* ok, now for each time slice, unabstract it into a set of symbols
		representing each actual slot and store it into the respective slot
		expansion structure. */

	for(i = 0; i < core->wt.wfs[wfloc].views->num_sym; i++)
	{
		/* the symbols and the container array are new memory */
		unabs = symbol_unabstract(core->wt.wfs[wfloc].views->sym[i], 
						slotdims, core->sec[loc].receptor.num_slot);

		/* Here I'm layering the unabstracted pieces into the slot expansion
			array. */
		for (j = 0; j < core->sec[loc].receptor.num_slot; j++)
		{
			sexp[j].sym[i] = unabs[j];
		}

		/* free the container, but leave the symbols in the sexp index */
		free(unabs);
	}

	/* since all of the slots have been broken up into individual slot
		symbols and put into the slot expansion array, I don't need this
		anymore */
	free(slotdims);
	slotdims = NULL;

	/* Ok, the SlotExpansion stuff is now truly initialized and I can break
		apart the symbols in each SlotExpansion node according to their
		integration number and input dimension and place them into the 
		expview field for each slot. */
	for (i = 0; i < core->sec[loc].receptor.num_slot; i++)
	{
		/* allocate the unabstraction array used to break apart a slot
			symbol into the true input for that slot */
		intdims = (int*)xmalloc(sizeof(int) * sexp[i].ints);
		/* initialize it, all symbols of input to this slot are of the same 
			dimension */
		for (j = 0; j < sexp[i].ints; j++)
		{
			intdims[j] = sexp[i].inputdim;
		}

		/* now, for every single entry for this slot, unabstract it and
			shove it in order into the expviews for this slot */
		for (j = 0; j < sexp[i].num_sym; j++)
		{
			/* in the case of a single integration, this basically performs
				a symbol_copy() */
			unabs = symbol_unabstract(sexp[i].sym[j], intdims, sexp[i].ints);
			
			for (index = 0; index < sexp[i].ints; index++)
			{
				/* write the fully expanded time slice of this slot into the
					ViewPoint holder */
				fflush(NULL);

				/* be very careful to write the unabstracting slot symbols
					correctly into the ViewPoint. There are a lot of 
					indicies to get used wrong in this statement. :) */
				sexp[i].expview->sym[(j*sexp[i].ints) + index] = unabs[index];
			}

			free(unabs);
		}

		free(intdims);
	}


	/* ok, now, take the fully expanded viewpoints from each slot, and 
		write it into the wavefront associated with the parent
		of each slot. Then, mark the wavefront active (even if
		it already is via another expansion), and then increase
		the number of observations for that wavefront. */
	
	for (i = 0; i < core->sec[loc].receptor.num_slot; i++)
	{
		ploc = wavefront_locate_serial_id(core, sexp[i].parent_id);

		/* parent of this slot has just become active */
		core->wt.wfs[ploc].active = TRUE;

		/* Add the newly computed ViewPoint to the parent */
		sexp[i].expview->next = core->wt.wfs[ploc].views;
		core->wt.wfs[ploc].views = sexp[i].expview;

		/* mark that I observed the parent in the reverse lookup */
		core->wt.wfs[ploc].num_observations++;
	}

	/* I am done, mark the original expanding node I was called with
		not active, since the wavefront has now passed this node.
		XXX This means no recursion for now */
	core->wt.wfs[wfloc].active = FALSE;

	/* Any memory left in the views for this expanding node will be garbage
		collected later in cortex_resolve(). */

	/* clean up the SlotExpansion array, this function does NOT clean up
		the expview field, which was handed off to other wavefronts above */
	sexp_destroy(sexp, core->sec[loc].receptor.num_slot);
}

/* the conditions for when a wavefront node is expandable */
int wavefront_expandable(WaveFront *wf, Cortex *core)
{
	if ((wf->active == TRUE) &&
		(wf->needed_observations == wf->num_observations) &&
		(which_kind_of_emitter(wf->serial_id, core->sec, core->num_sec, 
				core->input, core->num_input) == CORTEX_KIND_SECTION) &&
		((wf->views != NULL) && (wf->views->next == NULL)))
	{
		return TRUE;
	}

	return FALSE;
}


/* create a slot expansion array for a section */
SlotExpansion *sexp_init(int num_slots)
{
	int i;
	SlotExpansion *sexp = NULL;

	/* Now, make the slotexpansion array which will hold each slot's information
		needed for the expansion of the som symbols into the input symbols */
	sexp = (SlotExpansion*)xmalloc(sizeof(SlotExpansion) * num_slots);
	/* initialize them */
	for (i = 0; i < num_slots; i++)
	{
		sexp[i].dim = 0;
		sexp[i].ints = 0;
		sexp[i].inputdim = 0;
		sexp[i].parent_id = -1;
		sexp[i].kind = CORTEX_KIND_UNKNOWN;
		sexp[i].num_sym = 0;
		sexp[i].sym = NULL;
		sexp[i].expview = NULL;
	}

	return sexp;
}

/* if freesyms is true, then free the symbols in the symbol array, if false, 
	then don't. However, do not free the expview field since that memory
	will be handed off to someone else */
void sexp_destroy(SlotExpansion *sexp, int num_slots)
{
	int i, j;

	for (i = 0; i < num_slots; i++)
	{
		for (j = 0; j < sexp[i].num_sym; j++)
		{
			if (sexp[i].sym != NULL)
			{
				symbol_free(sexp[i].sym[j]);
				sexp[i].sym[j] = NULL;
			}
		}
		/* free the container array always */
		if (sexp[i].sym != NULL)
		{
			free(sexp[i].sym);
			sexp[i].sym = NULL;
		}
	}
	free(sexp);
}

/* Create a nice table representing all of the reverse lookup inputs, some
	inputs may not be available, others yes. */
InputResTable* wavetable_generate_irt(Cortex *core)
{
	InputResTable *irt = NULL;
	int i, j;
	int wfloc;

	/* sanity check, walk down the wavetable ensuring only true input channels
		are represented as active. If not, then something happened and the
		wavefront progression didn't function as normal. */
	for (i = 0; i < core->wt.num_sec; i++)
	{
		if (core->wt.wfs[i].active == TRUE)
		{
			if (which_kind_of_emitter(core->wt.wfs[i].serial_id, 
				core->sec, core->num_sec, core->input, core->num_input) !=
				CORTEX_KIND_INPUT)
			{
				printf("wavetable_generate_irt(): Not fully resolved!\n");
				printf("Wavefront stopped at id: %d\n", 
					core->wt.wfs[i].serial_id);
				exit(EXIT_FAILURE);
			}

			/* make sure it only has one view too */
			if (core->wt.wfs[i].views == NULL || core->wt.wfs[i].views->next 
				!= NULL)
			{
				printf("wavetable_generate_irt(): "
						"Input channel merge failure\n");
				exit(EXIT_FAILURE);
			}
		}
	}

	/* allocate up a table... */
	irt = (InputResTable*)xmalloc(sizeof(InputResTable) * 1);
	/* This is important, there is ALWAYS the same number of resolved
		inputs as normal inputs the cortex accepts. However, if some
		aren't available, we mark them as no active, and it is up to the
		user to perform the matching between inputs and resolved lookups */
	irt->num_inres = core->num_input;
	irt->inres = (InputResolution*)xmalloc(sizeof(InputResolution) * 
												irt->num_inres);

	/* ok, now that all active things have been checked to be actual inputs
		with the correct number of views (1), create the table
		in the same order as the inputs the cortex accepts,
		and fill it in with the view information.... */
	for (i = 0; i < core->num_input; i++)
	{
		/* find the input channel's serial id in the wave table */
		wfloc = wavefront_locate_serial_id(core, core->input[i].serial_id);

		irt->inres[i].serial_id = core->wt.wfs[wfloc].serial_id;

		/* if it is active, copy out the interesting bits from the view
			associated with it */
		if (core->wt.wfs[wfloc].active == TRUE)
		{
			irt->inres[i].active = TRUE;
			
			irt->inres[i].num_time_steps = core->wt.wfs[wfloc].views->num_sym;
			irt->inres[i].resolution = (Symbol**)xmalloc(sizeof(Symbol*) *
											irt->inres[i].num_time_steps);
			for (j = 0; j < core->wt.wfs[wfloc].views->num_sym; j++)
			{
				/* copy the symbol out so the input table owns memory nicely
					and the wavetable cleanup algorithms function with the
					garbage I leave in them */
				irt->inres[i].resolution[j] = 
					symbol_copy(core->wt.wfs[wfloc].views->sym[j]);
			}
		}
		else
		{
			/* otherwise mark it as not used */
			irt->inres[i].active = FALSE;
			irt->inres[i].num_time_steps = 0;
			irt->inres[i].resolution = NULL;
		}
	}

	return irt;
}

void inputrestable_destroy(InputResTable *irt)
{
	int i, j;

	for (i = 0; i < irt->num_inres; i++)
	{
		for (j = 0; j < irt->inres[i].num_time_steps; j++)
		{
			if (irt->inres[i].resolution[j] != NULL)
			{
				symbol_free(irt->inres[i].resolution[j]);
				irt->inres[i].resolution[j] = NULL;
			}
		}
		free(irt->inres[i].resolution);
	}

	free(irt->inres);

	free(irt);
}

InputResolution* inputrestable_input(InputResTable *irt, int index)
{
	if (index < 0 || index >= irt->num_inres)
	{
		printf("inputrestable_input(): Bounds Error!\n");
		printf("Asked for %d but it isn't between indicies 0 and %d\n", 
			index, irt->num_inres);
		exit(EXIT_FAILURE);
	}
	return &irt->inres[index];
}















