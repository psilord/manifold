#ifndef CORTEX_H
#define CORTEX_H

enum
{
	/* if the section produces output, I can just consume it and do nothing */
	SECTION_CONSUME,
	/* or I could pass the (dimensionally collapsed) symbol on to the specified
		sections */
	SECTION_PROPOGATE
};

/* The kind of a thing a serial number could be */
enum
{
	CORTEX_KIND_SECTION,
	CORTEX_KIND_INPUT,
	CORTEX_KIND_UNKNOWN
};

enum
{
	/* what is the cortex in the middle of doing */
	CORTEX_LEARNING,
	CORTEX_CLASSIFYING
};

/* when I ask the cortex to perceive something, do I want it to learn it, or
	just to classify it. */
enum
{
	CORTEX_REQUEST_LEARN,
	CORTEX_REQUEST_CLASSIFY
};

/* -------------------------------------------------------------------------- */

/* a place where an emitter can write */
typedef struct Slot_s
{
	/* The integration queue for this slot */
	IntQueue *iq;

	/* which emitter(either a section or an input) writes into this slot? */
	int parent;

	/* what kind of emitter writes into me, a section or an input? */
	int parent_kind;

} Slot;

/* A receptor combines all of the integrations comming into this section.
	The order matters since a abstract symbol will be constructed which
	contains all of the integrations in question for 0 to num_slot */
typedef struct Receptor_s
{
	int num_slot;
	Slot *slot;

} Receptor;

/* A connection is a structure which describes what slot in which section I am
	going to write something into */
typedef struct Connection_s
{
	/* the section id who should accept an output symbol */
	int section_id;

	/* Which slot in the integration queue array for the section id accepts
		the output */
	int slot;

} Connection;

/* The emitter is the collection of sections/slots that a section will write
	into */
typedef struct Emitter_s
{
	/* how many connections out of this section exist */
	int num_con;
	
	/* an array of specific connections to other sections */
	Connection *con;

} Emitter;

/* This will keep track of information needed to draw graphics about a various
	section, like where the latest learning location was, etc. */
typedef struct SecDisp_s
{
	/* computed location of the latest call to som_learn() */
	int learn_row, learn_col;

} SecDisp;

/* a Section keeps track of integrations comming into a SOM, and where to write
	integrations leaving a SOM */
typedef struct Section_s
{
	/* who this som is */
	int serial_id; 
	char *name;

	/* the SOM this section controls */
	SOM *som;

	/* where on the screen the lower left hand corner of the som is drawn */
	int x, y;

	/* do I consume the output silently, or forward it on? */
	unsigned int mode;

	/* Am I in classifying mode, or learning mode */
	unsigned int state;

	/* Here is where I receive all of the integrations from any section that
		wishes to give me something */
	Receptor receptor;

	/* This describes the sections to whom I should integrate the output I am
		generating */
	Emitter emitter;

	/* Some grphical information I collect as I progress for informational
		purposes */
	SecDisp secdisp;

} Section;

/* -------------------------------------------------------------------------- */

typedef struct CortexInput_s
{
	/* who is this input */
	int serial_id;
	char *name;

	/* dimension of the input channel */
	int dim;

	/* the input symbol I need to propogate */
	Symbol *sym;

	/* who gets these input symbols */
	Emitter emitter;
} CortexInput;

/* -------------------------------------------------------------------------- */

/* Identification of an output channel */
typedef struct CortexOutputChannel_s
{
	/* who is this output */
	int serial_id;
	char *name;

} CortexOutputChannel;

/* Since, as of now, there can only be ONE output channel per section, 
	this simply pairs the output sections (which have output channels) to
	the output channel */
typedef struct CortexOutputMapping_s
{
	/* serial number of the output section */
	int sec_id;

	/* serial number of the accepting output channel */
	int ochan_id;

} CortexOutputMapping;

/* there will always be the same number of these as there are output channels */
typedef struct CortexOutput_s
{
	/* true if an output exists at this time step, false if not */
	int active;

	/* to which output channel does this particular output bind. The char
		pointer in this structure is strdup()'ed from the CortexOutputChannel
		structure so it needs to be freed then the table this is used in below
		is freed. */
	CortexOutputChannel ochan;

	/* The normalized 2d output of the section outputting to this channel */
	Symbol *osym;

} CortexOutput;

/* This is the structure that gets produced and returned as output from
	cortex_resolve() at the current time step in the integrations. The
	user inspects it to see what is available on what output channel
	NULL in the osyms means that that particular output channel was
	not available. The output channels are in the same order as described
	in the ctx file. This is a publicly useable structure. */
typedef struct CortexOutputTable_s
{
	/* what did I request the cortex to do?
		The mode is not affected by the request. The request of
		the perception merely says should the cortex learn it
		(if possible) or should it just classify the perception
		(even if it could learn it. The request does not alter
		the natural state of the cortex--only what to do with
		the current perception. */
	int request;

	/* what mode is the cortex in? learning or classifying? */
	int mode;

	/* how many output channels are there at this time? */
	int num_output;
	CortexOutput *output;

} CortexOutputTable;

/* -------------------------------------------------------------------------- */

/* Begin stuff for reverse lookup */

/* A subjective reverse lookup viewpoint of a section by another section */
typedef struct ViewPoint_s
{
	int num_sym;

	/* each array element goes backwards in time. These Symbols could be
		positions on other SOMs or learned input data */
	Symbol **sym;

	/* the next subjective viewpoint, if any */
	struct ViewPoint_s *next;

} ViewPoint;

/* a structure representing all computed subjective viewpoints for a section 
	or input */
typedef struct WaveFront_s
{
	/* something that could be participating in a wave front propogation,
		could be either a section or an input channel. */
	int serial_id;

	/* This determines whether or not this particular section is actively
		participating in a wavefront calculation */
	int active;

	/* How many observations this node must have in order for a wavefront
		to propogate beyond this node. This is precalculated by the compiler
		and filled in when the root node is chosen by the user */
	int needed_observations;

	/* how many observations have been done */
	int num_observations;

	/* How many subjective viewpoints of this node have been seen? 
		When merging, all viewpoints get merged into a single viewpoint. */
	ViewPoint *views;

} WaveFront;

/* This table represents all WaveFronts as they are propogating backwards from
	the start root node to each of the inputs. There can be multiple viewpoints
	of a serial_id, and when the merge phase happens, the symbols associated
	with multiple serial_ids are merged using the centroid algorithm.
	Multiple serial_ids represents two viewpoints of an identical node
	in time by multiple future nodes looking backwards in time. */
typedef struct WaveTable_s
{
	/* each section(and input) gets its own wavefront location
		regardless if it is used or not. When a wavefront is
		propogating, we mark ones as used as they are expanded. */
	int num_sec;
	WaveFront *wfs;
	
} WaveTable;

/* End stuff for reverse lookup */

/* -------------------------------------------------------------------------- */
/* The user is meant to dig around in here to get out the resolution info */

/* When the reverse propogation is finished, then convert the each section_id
	viewpoint into one of these structures for user digestion */
typedef struct InputResolution_s
{
	/* for which input channel is this a resolution? */
	int serial_id;

	/* If this is true, then this input had data associated with it */
	int active;

	/* each symbol represents a time step going backwards in time(starting
		at zero), so how many time steps do I have for this
		serial_id (which is effectively an input channel) */
	int num_time_steps;

	/* These symbols are the same dimensions as the original input channel
		specified in the serial_id (when the wave front has gotten that far) */
	Symbol **resolution;

} InputResolution;

/* The table of inputs which have been fully resolved. */
typedef struct InputResTable_s
{
	/* number of available inputs to you, should always be the same as the
		number of inputs the cortex accepts. */
	int num_inres;

	/* The actual resolutions, some may be actve, others not. */
	InputResolution *inres;

} InputResTable;

/* end user futzable shit */
/* -------------------------------------------------------------------------- */

/* This pairs a node with how many times it has been observed during a 
	reverse lookup construction */
typedef struct Observation_s
{
	int serial_id;
	int obs;

} Observation;

/* for a single node, record how many observations there are for each node
	participating in the reverse lookup of the single node */
typedef struct ObservationTable_s
{
	int serial_id;
	int num_obs;
	Observation *ob;

} ObservationTable;

/* This keeps track of each reverse lookup observation table for every 
	lookupable section in the graph */
typedef struct ReverseTable_s
{
	int num_roots;
	ObservationTable *root;

} ReverseTable;

/* -------------------------------------------------------------------------- */

/* This is the big cortex structure which holds everything needed to build a
	noncyclic statistical feature classifier */
typedef struct Cortex_s
{
	/* The sections I'm simulating */
	int num_sec;
	Section *sec;

	/* Where input from the outside gets stored before I consume it */
	int num_input;
	CortexInput *input;

	/* the order of execution of the section id numbers */
	int num_exec;
	int *exec;

	/* The available output channels */
	int num_outchan;
	CortexOutputChannel *outchan;

	/* Number of mappings from sections to output channels */
	int num_outmap;
	CortexOutputMapping *outmap;

	/* For each section, aka root, we precalculated which nodes are part of 
		its reverse lookup and how many observations(the reachability number) 
		each participating node and input has. */
	ReverseTable rt;

	/* the wave propogation table used when doing reverse lookups */
	WaveTable wt;

} Cortex;

/* -------------------------------------------------------------------------- */

/* BEGIN private stuff */

/* initialize the reverse lookup tables for a cortex */
void wavetable_init(Cortex *core);

/* destroy the memory associated with a wave table for a coretex */
void wavetable_destroy(Cortex *core);

/* given a serial id, what kind of emitter is it? */
int which_kind_of_emitter(int serial_id, Section *sec, int num_sec,
    CortexInput *input, int num_input);

/* find the array index of the serial numbers in question is the places where 
	I look for them */
int find_section_by_id(int serial_id, Section *sec, int num_sec);
int find_input_by_id(int serial_id, CortexInput *input, int num_input);

/* find an output channel via a serial number */
int find_outchan_by_id(int serial_id, CortexOutputChannel *outchan, 
	int num_outchan);

/* given a section id, find the index of the corresponding output channel
	if one exists in the mapping table */
int find_outmap_by_sec_id(int sec_id, CortexOutputMapping *outmap,
	int num_outmap);

/* initialize an output table structure which is returned by the cortex */
CortexOutputTable* cortex_output_table_init(Cortex *core);

/* get rid of an output table structure */
void cortex_output_table_free(CortexOutputTable *ctxout);

/* given an output channel serial id, find its index in the output table */
int cortex_output_table_find_output_by_outchan_id(int outchanid, 
	CortexOutputTable *ctxout);

void cortex_output_table_stdout(CortexOutputTable *ctxout);

/* END private stuff */

/* PUBLIC stuff */

/* -------------------------------------------------------------------------- */
/* construct a corex given a file which contains an .lctx description */
Cortex* cortex_init(char *file);

/* Take whatever input I have and give it to the cortex. The cortex takes 
	control of this memory, so pass it malloc()'ed stuff, also the inputs
	must be in the same order as the .ctx file specified it. The return
	value is an output table which gives various available output channel 
	information in terms of the plane the cortex is embedded and other
	things about the cortex like what did we request it do the previous time
	step(learn or classify). */
CortexOutputTable* cortex_process(Cortex *core, Symbol **inputs, 
	int num_inputs, int request);

/* draw the cortex */
void cortex_draw(Cortex *core, int style);

/* get rid of it all */
void cortex_free(Cortex *core);

/* dump it to screen so I can make sure it is correct */
void cortex_stdout(Cortex *core);

/* Given an x,y location in the cortex space, return a set of input
	readings that would match that feature field location. NULL is returned
	if the x,y location wasn't on any section. The returned memory is 
	a seperately allocates object and the caller is responsible for 
	freeing it. */
InputResTable* cortex_resolve(Cortex *core, int row, int col);

/* -------------------------------------------------------------------------- */

/* This allows you to examine the reverse lookup outputs in the same order as
	the inputs were passed into the cortex. E.G., if Color was input 0,
	and audio was input 1, then calling thins function with an input of 1
	should give you the audio input, if any. Be aware of the activeness
	of the input, there might not be anything for that input if it wasn't
	reachable from the node you clicked on. This is READ ONLY data, do not
	free the pointer you get back. */
InputResolution* inputrestable_input(InputResTable *irt, int index);

/* get rid of an InputResTable when you are finished looking at it */
void inputrestable_destroy(InputResTable *irt);

/* -------------------------------------------------------------------------- */

/* 
	TODO 
	
	DONE
	1. Implement stuff to look up indecies of output channels given
		the section id or the outchan_id or the name.

	DONE
	2. Implement the change to the cortex_resolve API. Deal with the new
		enums.

	3. Implement returning of a CortexOutputTable at every time step.

	4. Implement the knowledge of when you compute a sections output, to check
		and see if it should go to an input channel and update the output table.

*/



/* -------------------------------------------------------------------------- */


#endif







