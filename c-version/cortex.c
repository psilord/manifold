#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <GL/gl.h>
#include <GL/glu.h>
#include "common.h"

#define BUF_SIZE 2048
#define NAME_SIZE 256

enum
{
	/* This MUST be -1 since it represents an index into an array */
	CORTEX_NOT_FOUND = -1
};

/* When I ask wether a serial_id is a true section or just an input channel, 
	this lets me know which one it is */
enum
{
	CORTEX_EMITTER_KIND_SECTION,
	CORTEX_EMITTER_KIND_INPUT,
	CORTEX_EMITTER_KIND_UNKNOWN
};

enum 
{
	/* This must not be a valid array index or serial number. This value
		represents the parent of the input channel, of which there is no
		parent */
	CORTEX_NO_PARENT = -1
};

/* functions to help me read the .lctx file */
static char* read_lctx_line(char *buf, int size, FILE *f, char *desc);
static char* lowlevel_get_lctx_line(char *buf, int size, FILE *f);

/* stuff to help me collate section results while I'm simulating the cortex */
static Symbol* abstract_receptor(Section *sec);


/* Load a cortex description from a .lctx file. There could be some buffer 
	overflows in this code, but feh, I just need it to work in the common 
	cases */
Cortex* cortex_init(char *file)
{
	Cortex *core = NULL;
	FILE *lctx = NULL;
	char buf[BUF_SIZE] = {'\0'};
	int i, j;
	int serial_id, dim, x, y, rows, cols, iter, prop;
	char name[NAME_SIZE];
	int num, location;
	int slot, int_num, slice_num;
	int kind, acceptor_id, loc_acceptor;

	lctx = fopen(file, "r");
	if (lctx == NULL)
	{
		printf ("Could not open cortex file: %s : %d(%s)\n", file, errno,
			strerror(errno));
		exit(EXIT_FAILURE);
	}

	core = (Cortex*)xmalloc(sizeof(Cortex) * 1);

	/* read how many sections I'm going to need */
	read_lctx_line(buf, BUF_SIZE, lctx, "Number of sections");

	/* initialize the section array */
	sscanf(buf, "%d\n", &core->num_sec);
	core->sec = (Section*)xmalloc(sizeof(Section) * core->num_sec);

	/* read the SOM data for each section */
	for (i = 0; i < core->num_sec; i++)
	{
		/* read section info */
		read_lctx_line(buf, BUF_SIZE, lctx, "A section");
		sscanf(buf, "%d %s %d %d %d %d %d %d %d\n", 
			&serial_id, name, &dim, &x, &y, &rows, &cols, &iter, &prop);

		/* set up the basic info (not receptors or emitters) for the section */

		core->sec[i].serial_id = serial_id;
		core->sec[i].som = som_init(dim, iter, rows, cols, NULL);
		core->sec[i].x = x;
		core->sec[i].y = y;
		core->sec[i].mode = (prop==1) ? SECTION_PROPOGATE : SECTION_CONSUME;
		core->sec[i].name = strdup(name);

		/* automatically start out as learning */
		core->sec[i].state = SOM_LEARNING;

		/* this stuff gets set up later */
		core->sec[i].receptor.num_slot = 0;
		core->sec[i].receptor.slot = NULL;
		core->sec[i].emitter.num_con = 0;
		core->sec[i].emitter.con = NULL;

		/* graphics stuff */
		core->sec[i].secdisp.learn_row = 0;
		core->sec[i].secdisp.learn_col = 0;
	}

	/* read the input channel description */
	read_lctx_line(buf, BUF_SIZE, lctx, "Number of input channels");
	sscanf(buf, "%d\n", &core->num_input);

	core->input = (CortexInput*)xmalloc(sizeof(CortexInput) * core->num_input);
	
	/* initialize the inputs array */
	for (i = 0; i < core->num_input; i++)
	{
		read_lctx_line(buf, BUF_SIZE, lctx, "An input channel");
		sscanf(buf, "%d %s %d\n", &serial_id, name, &dim);

		/* set up most of the input channel */
		core->input[i].serial_id = serial_id;
		core->input[i].name = strdup(name);
		core->input[i].dim = dim;
		core->input[i].sym = NULL;

		/* this stuff gets set up later */
		core->input[i].emitter.num_con = 0;
		core->input[i].emitter.con = NULL;
	}

	/* read the number of execution serial ids I need to read */
	read_lctx_line(buf, BUF_SIZE, lctx, "Number of execution serial ids");
	sscanf(buf, "%d\n", &core->num_exec);

	core->exec = (int*)xmalloc(sizeof(int) * core->num_exec);

	/* read the execution ids and store them */
	for (i = 0; i < core->num_exec; i++)
	{
		read_lctx_line(buf, BUF_SIZE, lctx, "Execution serial id");
		sscanf(buf, "%d\n", &core->exec[i]);
	}
	
	/* now read how many sections there are which need to have their receptors
		set up */

	read_lctx_line(buf, BUF_SIZE, lctx, "Number of sections needing receptors");
	sscanf(buf, "%d\n", &num);

	/* read the receptor information for each emitting section */
	for (i = 0; i < num; i++)
	{
		/* which section id is the one I need to set up? */
		read_lctx_line(buf, BUF_SIZE, lctx, "Recepting serial id");
		sscanf(buf, "%d\n", &serial_id);

		/* translate the serial number into a section array index number */
		location = find_section_by_id(serial_id, core->sec, core->num_sec);
		if (location == CORTEX_NOT_FOUND) {
			printf("cortex_init(): Acceptor id[%d] not found!\n",
				serial_id);
			exit(EXIT_FAILURE);
		}

		/* how many slots do I need to set up? */
		read_lctx_line(buf, BUF_SIZE, lctx, "Number of receptor slots");
		sscanf(buf, "%d\n", &core->sec[location].receptor.num_slot);

		/* make the slot array */
		core->sec[location].receptor.slot = 
			(Slot*)xmalloc(sizeof(Slot) * 
			core->sec[location].receptor.num_slot);

		/* now initialize each slot according to what I read */
		for (j = 0; j < core->sec[location].receptor.num_slot; j++)
		{
			read_lctx_line(buf, BUF_SIZE, lctx, "Slot info");
			sscanf(buf, "%d %d %d\n", &slot, &int_num, &slice_num);

			/* 'slot' is never going to be greater than the number of 
				recepting slots, or less than zero */
			core->sec[location].receptor.slot[slot].iq =
				intqueue_init(int_num, slice_num);

			/* This will be set up later when the connections are made */
			core->sec[location].receptor.slot[slot].parent = CORTEX_NO_PARENT;
			core->sec[location].receptor.slot[slot].parent_kind = 
				CORTEX_KIND_UNKNOWN;
		}
	}

	/* Now read the number of sections/inputs which are emitters and the
		connection information that goes with each one of them */
	read_lctx_line(buf, BUF_SIZE, lctx, "Number of emitter/inputs");
	sscanf(buf, "%d\n", &num);
	
	/* read each emitter and record the connection information */
	for (i = 0; i < num; i++)
	{
		read_lctx_line(buf, BUF_SIZE, lctx, "Emitter/input serial id");
		sscanf(buf, "%d\n", &serial_id);

		/* a serial number can be an input channel OR a section, so figure
			out which one it is right here */
		kind = which_kind_of_emitter(serial_id, core->sec, core->num_sec,
				core->input, core->num_input);

		/* depending on what kind of emitter I have, set up the connection 
			array for it */
		switch(kind)
		{
			case CORTEX_EMITTER_KIND_SECTION:
				location = find_section_by_id(serial_id, core->sec, 
					core->num_sec);

				read_lctx_line(buf, BUF_SIZE, lctx, 
					"Number of connection slots");
				sscanf(buf, "%d\n", &core->sec[location].emitter.num_con);

				core->sec[location].emitter.con = 
					(Connection*)xmalloc(sizeof(Connection) *
						core->sec[location].emitter.num_con);

				/* now, read the connection information */
				for(j = 0; j < core->sec[location].emitter.num_con; j++)
				{
					read_lctx_line(buf, BUF_SIZE, lctx, "A connection slot");
					sscanf(buf, "%d %d\n", &acceptor_id, &slot);

					/* set up the connection */
					core->sec[location].emitter.con[j].section_id = 
						acceptor_id;
					core->sec[location].emitter.con[j].slot = slot;

					/* set up the parent link */
					loc_acceptor = find_section_by_id(acceptor_id, core->sec,
						core->num_sec);
					core->sec[loc_acceptor].receptor.slot[slot].parent = 
						serial_id;
					core->sec[loc_acceptor].receptor.slot[slot].parent_kind = 
						CORTEX_KIND_SECTION;
				}

				break;
			
			case CORTEX_EMITTER_KIND_INPUT:

				location = find_input_by_id(serial_id, core->input, 
					core->num_input);

				read_lctx_line(buf, BUF_SIZE, lctx, 
					"Number of connection slots");
				sscanf(buf, "%d\n", &core->input[location].emitter.num_con);

				core->input[location].emitter.con = 
					(Connection*)xmalloc(sizeof(Connection) *
						core->input[location].emitter.num_con);

				/* now, read the connection information */
				for(j = 0; j < core->input[location].emitter.num_con; j++)
				{
					read_lctx_line(buf, BUF_SIZE, lctx, "A connection slot");
					sscanf(buf, "%d %d\n", &acceptor_id, &slot);

					/* set up the connection */
					core->input[location].emitter.con[j].section_id = 
						acceptor_id;
					core->input[location].emitter.con[j].slot = slot;

					/* set up the parent link */
					loc_acceptor = find_section_by_id(acceptor_id, core->sec,
						core->num_sec);
					core->sec[loc_acceptor].receptor.slot[slot].parent = 
						serial_id;
					core->sec[loc_acceptor].receptor.slot[slot].parent_kind = 
						CORTEX_KIND_INPUT;
				}

				break;

			case CORTEX_EMITTER_KIND_UNKNOWN:
				printf("cortex_init(): Attempting to connect unknown "
					"emitter[%d] to something!\n", serial_id);
				exit(EXIT_FAILURE);
				break;

			default:
				printf("cortex_init(): This shouldn't ever happen!\n");
				exit(EXIT_FAILURE);
				break;
		}
	}

	/* read the pre-calculated reverse lookup observation tables for each
		lookupable section */
	read_lctx_line(buf, BUF_SIZE, lctx, "Number of root nodes in obtable");
	sscanf(buf, "%d\n", &core->rt.num_roots);

	/* allocate the obtable memory for all of the possible roots */
	core->rt.root = (ObservationTable*)
		xmalloc(sizeof(ObservationTable) * core->rt.num_roots);

	/* read the observation table for each root node */
	for (i = 0; i < core->rt.num_roots; i++)
	{
		read_lctx_line(buf, BUF_SIZE, lctx, "A root node in the obtable");
		sscanf(buf, "%d\n", &core->rt.root[i].serial_id);
		read_lctx_line(buf, BUF_SIZE, lctx, "Num of observations for a root");
		sscanf(buf, "%d\n", &core->rt.root[i].num_obs);

		/* allocate the memory desired for the observation for this particular
			root node */
		core->rt.root[i].ob = (Observation*)
			xmalloc(sizeof(Observation) * core->rt.root[i].num_obs);

		/* read the observations for this particular root */
		for (j = 0; j < core->rt.root[i].num_obs; j++)
		{
			read_lctx_line(buf, BUF_SIZE, lctx, "An observation");
			sscanf(buf, "%d %d\n", &core->rt.root[i].ob[j].serial_id,
				&core->rt.root[i].ob[j].obs);
		}
	}

	/* read the output channel identification/ordering table */
	core->outchan = NULL;
	read_lctx_line(buf, BUF_SIZE, lctx, "Number of Output Channels");
	sscanf(buf, "%d\n", &core->num_outchan);
	if (core->num_outchan != 0)
	{
		core->outchan = 
			(CortexOutputChannel*)xmalloc(sizeof(CortexOutputChannel) * 
				core->num_outchan);

		/* read each output channels description */
		for (i = 0; i < core->num_outchan; i++)
		{
			read_lctx_line(buf, BUF_SIZE, lctx, "An output channel");
			sscanf(buf, "%d %s\n", &core->outchan[i].serial_id, name);
			core->outchan[i].name = strdup(name);
		}
	}

	/* read the section to output_channel mapping table */
	core->outmap = NULL;
	read_lctx_line(buf, BUF_SIZE, lctx, "Number of Output Channel Maps");
	sscanf(buf, "%d\n", &core->num_outmap);
	if (core->num_outmap != 0)
	{
		core->outmap = 
			(CortexOutputMapping*)xmalloc(sizeof(CortexOutputMapping) * 
				core->num_outmap);

		/* read each output channel map description */
		for (i = 0; i < core->num_outmap; i++)
		{
			read_lctx_line(buf, BUF_SIZE, lctx, "An output channel map");
			sscanf(buf, "%d %d\n", &core->outmap[i].sec_id, 
				&core->outmap[i].ochan_id);
		}
	}

	fclose(lctx);

	/* initialize the wave propogation table used for reverse lookups */
	wavetable_init(core);

	return core;
}

void cortex_free(Cortex *core)
{
	int i, j;

	/* free the sections */
	for (i = 0; i < core->num_sec; i++)
	{
		/* remove the human readable name of this section */
		free(core->sec[i].name);
		core->sec[i].name = NULL;

		/* get rid of the SOM */
		som_free(core->sec[i].som);
		core->sec[i].som = NULL;

		/* free the receptor */
		for (j = 0; j < core->sec[i].receptor.num_slot; j++)
		{
			intqueue_free(core->sec[i].receptor.slot[j].iq);
			core->sec[i].receptor.slot[j].iq = NULL;
		}
		/* free the slot array */
		free(core->sec[i].receptor.slot);
		core->sec[i].receptor.slot = NULL;

		/* free the emitter */
		free(core->sec[i].emitter.con);
		core->sec[i].emitter.con = NULL;
	}

	/* free the section array */
	free(core->sec);
	core->sec = NULL;

	/* free the inputs */
	for(i = 0; i < core->num_input; i++)
	{
		/* get rid of the human readable name */
		free(core->input[i].name);
		core->input[i].name = NULL;

		/* If any symbols are currently waiting for processing, then remove
			them since I own this memory */
		if (core->input[i].sym != NULL)
		{
			symbol_free(core->input[i].sym);
			core->input[i].sym = NULL;
		}

		/* get rid of the emitter */
		free(core->input[i].emitter.con);
		core->input[i].emitter.con = NULL;
	}

	/* get rid of the input container array */
	free(core->input);
	core->input = NULL;

	/* get rid of the output channel ordering container array */
	if (core->num_outchan != 0 && core->outchan != NULL)
	{
		/* free the name of each output channel */
		for (i = 0; i < core->num_outchan; i++)
		{
			if (core->outchan[i].name != NULL)
			{
				free(core->outchan[i].name);
				core->outchan[i].name = NULL;
			}
		}
		free(core->outchan);
		core->outchan = NULL;
	}

	/* get rid of the output channel map container array */
	if (core->num_outmap != 0 && core->outmap != NULL)
	{
		free(core->outmap);
		core->outmap = NULL;
	}

	/* get rid of the order of execution array */
	free(core->exec);
	core->exec = NULL;

	/* get rid of the precalculated reverse lookup table of observations */
	for (i = 0; i < core->rt.num_roots; i++)
	{
		free(core->rt.root[i].ob);
	}
	free(core->rt.root);

	/* get rid of the wavetable */
	wavetable_destroy(core);

	/* now, finally, free the cortex */
	free(core);
}

/* print a reasonably readable traversal of the Cortex structure */
void cortex_stdout(Cortex *core)
{
	int i, j, location, kind;

	printf("Cortex\n");
	printf("------\n");

	/* dump the sections */
	printf("Section Dump:\n");
	for (i = 0; i < core->num_sec; i++)
	{
		printf("\tSection Array Index: %d\n", i);
		printf("\t--------------------\n");
		/* print the human readable name of this section */
		printf("\tSection Name: %s\n", core->sec[i].name);
		printf("\tSection ID: %d\n", core->sec[i].serial_id);

		/* just give the pointer */
		printf("\tSOM: %p\n", core->sec[i].som);

		/* dump the receptor */
		printf("\tReceptor:\n");
		for (j = 0; j < core->sec[i].receptor.num_slot; j++)
		{
			printf("\t\tSlot: %d\n", j);
			printf("\t\tParent ID: %d (", core->sec[i].receptor.slot[j].parent);
			switch(core->sec[i].receptor.slot[j].parent_kind)
			{
				case CORTEX_KIND_INPUT:
					printf("An Input Channel)\n");
					break;
				case CORTEX_KIND_SECTION:
					printf("A Section)\n");
					break;
				case CORTEX_KIND_UNKNOWN:
					printf("UNKOWN!)\n");
					break;
				default:
					printf("cortex_stdout(): This should never happen!\n");
					exit(EXIT_FAILURE);
					break;
			}
			intqueue_stdout(core->sec[i].receptor.slot[j].iq);
		}

		/* dump the emitter */
		printf("\tEmitter:\n");
		if (core->sec[i].mode == SECTION_PROPOGATE)
		{
			for (j = 0; j < core->sec[i].emitter.num_con; j++)
			{
				printf("\t\tsection_id: %d, slot %d\n", 
					core->sec[i].emitter.con[j].section_id,
					core->sec[i].emitter.con[j].slot);
			}
		} else {
			printf("\t\tConsuming.\n");
		}
		printf("\n");
	}

	/* dump the inputs */
	printf("Inputs:\n");
	for(i = 0; i < core->num_input; i++)
	{
		printf("\tInput array index: %d\n", i);
		printf("\t------------------\n");
		printf("\tInput Channel: %s\n", core->input[i].name);
		printf("\tID: %d\n", core->input[i].serial_id);

		printf("\tSymbol(if any): ");
		if (core->input[i].sym != NULL)
		{
			symbol_stdout(core->input[i].sym);
		} else {
			printf("NONE\n");
		}

		printf("\tEmitter:\n");
		for (j = 0; j < core->input[i].emitter.num_con; j++)
		{
			printf("\t\tsection_id: %d, slot %d\n", 
				core->input[i].emitter.con[j].section_id,
				core->input[i].emitter.con[j].slot);
		}
		printf("\n");
	}

	printf("Execution Order:\n");
	for (i = 0; i < core->num_exec; i++)
	{
		/* be kind, and actually spit out the names along with the serial
			numbers for each of checking */

		/* I can only execute sections and not inputs, so this is valid */
		location = find_section_by_id(core->exec[i], core->sec, core->num_sec);
		printf("\t%d : %s\n", core->exec[i], core->sec[location].name);
	}
	printf("\n");

	printf("Precalculated Reverse Lookup Observation Table:\n");
	for (i = 0; i < core->rt.num_roots; i++)
	{
		
		location = find_section_by_id(core->rt.root[i].serial_id, 
			core->sec, core->num_sec);
		printf("\tRoot id: %d(%s)\n", core->rt.root[i].serial_id,
			core->sec[location].name);
		for (j = 0; j < core->rt.root[i].num_obs; j++)
		{
			/* determine if the node I'm looking at is a section or an
				input */
			kind = which_kind_of_emitter(core->rt.root[i].ob[j].serial_id,
				core->sec, core->num_sec, core->input, core->num_input);
			switch(kind)
			{
				case CORTEX_KIND_SECTION:
					printf("\t\tSECTION: ");
					break;
				case CORTEX_KIND_INPUT:
					printf("\t\t  INPUT: ");
					break;
				case CORTEX_KIND_UNKNOWN:
					printf("\t\tUNKNOWN: ");
					break;
				default:
					printf("cortex_stdout(): This should never happen!\n");
					exit(EXIT_FAILURE);
					break;
			}

			/* dump the observation information for this particular root node */
			printf("%d Obs: %d\n", core->rt.root[i].ob[j].serial_id,
				core->rt.root[i].ob[j].obs);
		}
	}
	printf("\n");
}

void cortex_draw(Cortex *core, int style)
{
	int i;
	Section *sec;
	int row, col;
	int draw_boxes = 1;

	/* ok, let's draw the SOMs at the location required */

	for (i = 0; i < core->num_sec; i++)
	{
		sec = &core->sec[i];
		som_draw(sec->som, style, sec->x, sec->y);

		/* draw the marker box around the WTA */
		if (draw_boxes == 1)
		{
			row = core->sec[i].secdisp.learn_row;
			col = core->sec[i].secdisp.learn_col;

			/* mark winning neuron */
			glBegin(GL_POINTS);
				glColor3f(1, 1, 1);
				glVertex3f(col+sec->x, row+sec->y, 0.5);
			glEnd();

			/* draw a little box around the last learning location */
			glBegin(GL_LINE_LOOP);
			switch(core->sec[i].state)
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
			glVertex3f(col-10+sec->x, row-10+sec->y, 0.5);
			glVertex3f(col-10+sec->x, row+10+sec->y, 0.5);
			glVertex3f(col+10+sec->x, row+10+sec->y, 0.5);
			glVertex3f(col+10+sec->x, row-10+sec->y, 0.5);
			glEnd();
		}
	}
}


/* Take some input, process it either learning or classifying it, then return
	the output channels if any */
CortexOutputTable* cortex_process(Cortex *core, Symbol **inputs, 
	int num_inputs, int request)
{
	int i, j, location, acc_loc, emission_slot;
	Symbol *sym, *output;
	int pcol, prow;
	CortexOutputTable *ctxout = NULL;
	int omindex;
	int outputindex;

	if (num_inputs != core->num_input)
	{
		printf("cortex_process(): Number of input symbol mismatch!\n");
		exit(EXIT_FAILURE);
	}

	/* move the input into the Cortex. I am taking control of the passed
		in memory. */
	for (i = 0; i < core->num_input; i++)
	{
		core->input[i].sym = inputs[i];

		/* sanity check the dimension of each input channel */
		if (core->input[i].dim != symbol_get_dim(inputs[i]))
		{
			printf("cortex_process(): Input channel %d is of dimension %d, "
				"but the cortex was expecting dimension %d\n", i, 
				symbol_get_dim(inputs[i]), core->input[i].dim);
			exit(EXIT_FAILURE);
		}
	}

	/* ok, now propogate all inputs into the accepting sections */
	for (i = 0; i < core->num_input; i++)
	{
		/* distribute this symbol to each accepting section */
		for (j = 0; j < core->input[i].emitter.num_con; j++)
		{
			/* We only forward copies of things to the accepting slots */
			sym = symbol_copy(core->input[i].sym);
			
			/* find the location in the section array of the serial number
				of the section we want to give the information to */
			location = find_section_by_id(
					core->input[i].emitter.con[j].section_id, core->sec, 
					core->num_sec);
			if (location == CORTEX_NOT_FOUND)
			{
				printf("cortex_process(): Can't emit input to acceptor!\n");
				exit(EXIT_FAILURE);
			}
			emission_slot = core->input[i].emitter.con[j].slot;
			
			/* physically put it into the slot in the section requested */
			intqueue_enqueue(
				core->sec[location].receptor.slot[emission_slot].iq,
				sym);
		}
	}

	/* set up some of the output table */
	ctxout = cortex_output_table_init(core);
	ctxout->request = request;

	/* now that we are done, remove the memory stored in the input array since
		we made copies of it for the accepting sections */
	for (i = 0; i < core->num_input; i++)
	{
		symbol_free(core->input[i].sym);
		core->input[i].sym = NULL;
	}

	/* now, execute the sections in the order specified. Be careful to 
		only propogate the "result" of a SOM if it is in the CLASSIFYING
		state AND it is propogating. This results in a propogating "convergance
		wave" through out the system, and might need to be revisited if 
		recursion is ever implemented. However, even though this in happening,
		each section is ALWAYS executed, it just "knows" it doesn't have work
		to do if the concatenation of the integration queues doesn't yield a
		new symbol to learn. */

	for (i = 0; i < core->num_exec; i++)
	{
		/* I'm going to work with a single section at a time */
		location = find_section_by_id(core->exec[i], core->sec, core->num_sec);
		if (location == CORTEX_NOT_FOUND)
		{
			printf("cortex_process(): Can't execute unknown section!\n");
			exit(EXIT_FAILURE);
		}

		/* if this function returns to me something, it means this section is
			ready for its input */
		if ((sym = abstract_receptor(&core->sec[location])) != NULL)
		{
			/* Have the SOM learn/classify this symbol as per the caller's 
				wishes (and draw how it is doing) */
			switch(request)
			{
				case CORTEX_REQUEST_CLASSIFY:
					core->sec[location].state = 
						som_learn(core->sec[location].som, sym, &prow, &pcol,
						core->sec[location].x, core->sec[location].y,
						SOM_REQUEST_CLASSIFY, FALSE);
					break;

				case CORTEX_REQUEST_LEARN:
					core->sec[location].state = 
						som_learn(core->sec[location].som, sym, &prow, &pcol,
						core->sec[location].x, core->sec[location].y,
						SOM_REQUEST_LEARN, FALSE);
					break;

				default:
					printf("cortex_process(): Invalid request! %d\n", request);
					exit(EXIT_FAILURE);
					break;
			}

			/* Smooth/sharpen the SOM by some XXX arbitrary amount. 
				Figure out if I can do this both for classify or learn
				requests. */
			if (core->sec[location].state == SOM_LEARNING)
			{
				/* try and figure out of I can get the amount of 
					softening/sharpening to be autodiscovered */

				/* Blur out some noise from the system */
/*				conv_apply_easy(core->sec[location].som, CONV_BLUR_SMEAR, .5, .1);*/

				/* Sharpen the edges a little bit more than I blurred them
					between the regions */
/*				conv_apply_easy(core->sec[location].som, CONV_LAPLACIAN, 1, 1.0);*/

			}

			/* record the bmu for later display */
			core->sec[location].secdisp.learn_row = prow;
			core->sec[location].secdisp.learn_col = pcol;
			
			/* now that I've learned the symbol, get rid of it */
			symbol_free(sym);
			
			/* If this section is in classification stage, and it is
				propogating information, then copy the dimensionally reduced
				output to all of its children in the emitter list. */
			if (core->sec[location].state == SOM_CLASSIFYING && 
				core->sec[location].mode == SECTION_PROPOGATE)
			{
				/* copy the result to all of the connection's intqueues */
				for (j = 0; j < core->sec[location].emitter.num_con; j++)
				{
					output = symbol_init(2);
					symbol_set_2(output, 
					(double)prow/(double)som_get_rows(core->sec[location].som),
					(double)pcol/(double)som_get_cols(core->sec[location].som));

					/* where in the section array is the acceptor in 
						question */
					acc_loc = find_section_by_id(
						core->sec[location].emitter.con[j].section_id, 
							core->sec, core->num_sec);
					emission_slot = core->sec[location].emitter.con[j].slot;

					/* ok, add the output to the correct intqueue slot for the
						acceptor. This function takes ownership of the memory.
						*/
					intqueue_enqueue(
						core->sec[acc_loc].receptor.slot[emission_slot].iq,
						output);
					output = NULL;
				}
			}

			/* This next part dealing with the output channels of sections that
				have them will happen no matter if the section was classifying 
				or learning.  */

			/* find the output channel serial id associated with this 
				section id. */
			omindex = find_outmap_by_sec_id(core->exec[i], core->outmap, 
							core->num_outmap);

			if (omindex != CORTEX_NOT_FOUND) {
				/* This section has an output, so set it up into the output
					table */

				/* find it in the output table */
				outputindex = cortex_output_table_find_output_by_outchan_id(
									core->outmap[omindex].ochan_id, ctxout);

				/* make the output representation symbol:
					dim 0,1 contain the physical location in the cortex
					dim 2,3 contain the physical location in the section
					dim 4,5 contain the normalized location for the section
				*/
				output = symbol_init(6);
				symbol_set_6(output,
					core->sec[location].y + prow,
					core->sec[location].x + pcol,
					prow, 
					pcol,
					(double)prow/(double)som_get_rows(core->sec[location].som),
					(double)pcol/(double)som_get_cols(core->sec[location].som));

				/* now, place the symbol into the output table and mark the
					output channel as active. The output table gains owner
					ship of the symbol memory. */
				ctxout->output[omindex].active = TRUE;
				ctxout->output[omindex].osym = output;

				output = NULL;
			}
		}

		/* if I got a NULL from trying to abstract the receptor, then just go
			to the next section in the execution order */
	}

	/* XXX wrong */
	ctxout->mode = CORTEX_LEARNING;
	
	/* the result of what I asked to do, for the available output channels */
	return ctxout;
}


/* ------------------------------------------------------------------------- */
/* some helper functions to deal with the cortex structure */

/* if all of the slots in the receptor are ready, then ask each of them for
	their concatenated symbols, and then concatenate all of those results
	together into a super symbol which is then the input for this section. If
	for whatever reason(not all queues are ready, etc) a full receptor
	abstraction can't hapen, then return NULL */
static Symbol* abstract_receptor(Section *sec)
{
	int i;
	Symbol **abs_list;
	Symbol *sym;

	/* check to see if all receptors are ready */
	for (i = 0; i < sec->receptor.num_slot; i++)
	{
		if (intqueue_ready(sec->receptor.slot[i].iq) == FALSE)
		{
			/* looks like something wasn't ready, so I can't abstract this
				receptor. If some things had been ready, but others not, then
				when everything finally IS ready, information had been lost.
				For now, I can live with this... */
			return NULL;
		}
	}

	/* ok, if I made it here, it means that the entire receptor is ready for
		abstraction... */

	abs_list = (Symbol**)xmalloc(sizeof(Symbol*) * sec->receptor.num_slot);

	/* gather the results from each integration queue */
	for (i = 0; i < sec->receptor.num_slot; i++)
	{
		/* this returns a new piece of memory, so remember to free it later */
		abs_list[i] = intqueue_dequeue(sec->receptor.slot[i].iq);

		/* sanity check */
		if (abs_list[i] == NULL)
		{
			printf("abstract_receptor(): I couldn't dequeue when I thought I "
				"could!\n");
			exit(EXIT_FAILURE);
		}
	}

	/* produce the super abstraction of the entire receptor */
	sym = symbol_abstract(abs_list, sec->receptor.num_slot);

	/* get rid of my little container structure, and the symbols inside it */
	for (i = 0; i < sec->receptor.num_slot; i++)
	{
		symbol_free(abs_list[i]);
		abs_list[i] = NULL;
	}
	free(abs_list);

	return sym;
}

int which_kind_of_emitter(int serial_id, Section *sec, int num_sec,
	CortexInput *input, int num_input)
{
	int loc_sec;
	int loc_inp;

	loc_sec = find_section_by_id(serial_id, sec, num_sec);
	if (loc_sec != CORTEX_NOT_FOUND) {
		return CORTEX_EMITTER_KIND_SECTION;
	}

	loc_inp = find_input_by_id(serial_id, input, num_input);
	if (loc_inp != CORTEX_NOT_FOUND) {
		return CORTEX_EMITTER_KIND_INPUT;
	}

	/* This should only happen in case of error */
	return CORTEX_EMITTER_KIND_UNKNOWN;
}

/* given a section id, return the index in the array it lives in, or 
	CORTEX_NOT_FOUND if not found */
int find_section_by_id(int serial_id, Section *sec, int num_sec)
{
	int i;

	for (i = 0; i < num_sec; i++) {
		if (sec[i].serial_id == serial_id) {
			return i;
		}
	}

	return CORTEX_NOT_FOUND;
}

/* given an input id, return the index in the array it lives in, or NOT_FOUND
	if not found */
int find_input_by_id(int serial_id, CortexInput *input, int num_input)
{
	int i;

	for (i = 0; i < num_input; i++) {
		if (input[i].serial_id == serial_id) {
			return i;
		}
	}

	return CORTEX_NOT_FOUND;
}

/* given an output channel serial id, find the index in the output channel
	array, or NOT_FOUND if not found */
int find_outchan_by_id(int serial_id, CortexOutputChannel *outchan, 
	int num_outchan)
{
	int i;

	for (i = 0; i < num_outchan; i++) {
		if (outchan[i].serial_id == serial_id) {
			return i;
		}
	}

	return CORTEX_NOT_FOUND;
}

/* given a section id, find the output id this section must output to if
	it exists */
int find_outmap_by_sec_id(int sec_id, CortexOutputMapping *outmap, 
	int num_outmap)
{
	int i;

	for (i = 0; i < num_outmap; i++) {
		if (outmap[i].sec_id == sec_id) {
			return i;
		}
	}

	return CORTEX_NOT_FOUND;
}

/* given an output channel serial id, find the index in the output table's
	output field which corresponds to the serial id */
int cortex_output_table_find_output_by_id(CortexOutputTable *ctxout, 
	int serial_id)
{
	int i;
	
	for (i = 0; i < ctxout->num_output; i++)
	{
		if (ctxout->output[i].ochan.serial_id == serial_id)
		{
			return i;
		}
	}

	return CORTEX_NOT_FOUND;
}

/* create an empty inactive output table for cortex_process to return */
CortexOutputTable* cortex_output_table_init(Cortex *core)
{
	int i;
	CortexOutputTable *ctxout = NULL;

	ctxout = (CortexOutputTable*)xmalloc(sizeof(CortexOutputTable) * 1);
	ctxout->num_output = core->num_outchan;
	ctxout->output = NULL;
	/* WARNING: request and mode are undefined at this point! */

	if (ctxout->num_output != 0) {
		ctxout->output = (CortexOutput*)
						xmalloc(sizeof(CortexOutput) * ctxout->num_output);
		
		/* set up the outputs with default values to be filled in later */
		for (i = 0; i < ctxout->num_output; i++)
		{
			ctxout->output[i].active = FALSE;
			ctxout->output[i].ochan.serial_id = core->outchan[i].serial_id;
			ctxout->output[i].ochan.name = NULL;
			if (core->outchan[i].name != NULL) {
				ctxout->output[i].ochan.name = strdup(core->outchan[i].name);
			}
				
			ctxout->output[i].osym = NULL;
		}
	}

	return ctxout;
}

/* get rid of the output table structure, lock stock and barrel */
void cortex_output_table_free(CortexOutputTable *ctxout)
{
	int i;

	if (ctxout->num_output != 0) {
		for (i = 0; i < ctxout->num_output; i++) {
			if (ctxout->output[i].ochan.name != NULL) {
				free(ctxout->output[i].ochan.name);
				ctxout->output[i].ochan.name = NULL;
			}

			if (ctxout->output[i].osym != NULL) {
				symbol_free(ctxout->output[i].osym);
				ctxout->output[i].osym = NULL;
			}
		}
	}

	free(ctxout->output);
	ctxout->output = NULL;

	free(ctxout);
}

/* print out an output table to stdout */
void cortex_output_table_stdout(CortexOutputTable *ctxout)
{
	int i;

	if (ctxout == NULL) {
		printf("cortex_output_table_display(): passed NULL!\n");
		return;
	}

	printf("CortexOutputTable:\n");
	printf("------------------\n");

	printf("\tPrevious request: %s\n", 
		ctxout->request==CORTEX_REQUEST_CLASSIFY?"CORTEX_REQUEST_CLASSIFY":
			ctxout->request==CORTEX_REQUEST_LEARN?"CORTEX_REQUEST_LEARN":
				"UNKNOWN PREVIOUS REQUEST");

	printf("\tMode: %s\n",
		ctxout->mode==CORTEX_LEARNING?"CORTEX_LEARNING":
			ctxout->mode==CORTEX_CLASSIFYING?"CORTEX_CLASSIFYING":
				"UNKNOWN MODE");

	printf("\tOutput Channels:\n");
	printf("\t----------------\n");

	for (i = 0; i < ctxout->num_output; i++)
	{
		printf("\t\tActive %s\n", 
			ctxout->output[i].active==TRUE?"TRUE":"FALSE");

		printf("\t\tOutput Channel %d:[%s(%d)]\n", i, 
			ctxout->output[i].ochan.name, ctxout->output[i].ochan.serial_id);

		printf("\t\tSymbol 0x%p\n", ctxout->output[i].osym);
	}
}

/* Find the index into the output array if the output channel serial id */
int cortex_output_table_find_output_by_outchan_id(int ochanid, 
	CortexOutputTable *ctxout)
{
	int i;

	for (i = 0; i < ctxout->num_output; i++) {
		if (ctxout->output[i].ochan.serial_id == ochanid)
		{
			return i;
		}
	}

	return CORTEX_NOT_FOUND;
}

/* ------------------------------------------------------------------------ */
/* some file parsing helper functions */

static char* read_lctx_line(char *buf, int size, FILE *f, char *desc)
{
	char *ptr = NULL;

	if ((ptr = lowlevel_get_lctx_line(buf, size, f)) == NULL) {
		printf("cortex_init(): Short read while reading: %s\n", desc);
		exit(EXIT_FAILURE);
	}

/*	printf("READ(%s): %s", desc, ptr);*/

	return ptr;
}

/* Return a line which is not a comment/whitespace line */
static char* lowlevel_get_lctx_line(char *buf, int size, FILE *f)
{
	int meat;
	char *str;
	char *ptr;

	do {
		/* assume I'm going to read something valid at first */
		meat = TRUE;

		str = fgets(buf, size, f);

		/* check for end of input */
		if (str == NULL) {
			return NULL;
		}

		/* check to see if it isn't a comment/whitespace line */
		ptr = str;
		while(*ptr == ' ' || *ptr == '\t' || *ptr == '\n') {
			ptr++;
		}
		if (*ptr == '#' || *ptr == '\0') {
			/* this is a comment, or line of whitespace */
			meat = FALSE;
		}

	} while(meat == FALSE);

	return buf;
}










