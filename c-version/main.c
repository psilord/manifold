#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>

#include "SDL.h"   /* All SDL App's need this */
#include <GL/gl.h>
#include <GL/glu.h>

#include "common.h"

#define HEIGHT 512
#define WIDTH 960

/* A simple test program to see if I can initialize the SDL library, 
	create a surface, and then show a bitmap on the surface. */

void HotKey_ToggleFullScreen(void)
{
    SDL_Surface *screen;

    screen = SDL_GetVideoSurface();
    if ( SDL_WM_ToggleFullScreen(screen) ) {
        printf("Toggled fullscreen mode - now %s\n",
            (screen->flags&SDL_FULLSCREEN) ? "fullscreen" : "windowed");
    } else {
        printf("Unable to toggle fullscreen mode\n");
    }
}

int process_events(int *row, int *col)
{
	SDL_Event event;

	while(SDL_PollEvent(&event))
	{
		switch(event.type)
		{
			case SDL_MOUSEBUTTONDOWN:
				*row = HEIGHT - event.button.y;
				*col = event.button.x;
				return STATE_MOUSE_DOWN;
				break;

			case SDL_KEYDOWN:
				switch(event.key.keysym.sym)
				{
					case SDLK_q:
						printf("Quitting....\n");
						return STATE_EXIT;
						break;
					case SDLK_a:
						return STATE_ACTUAL;
						break;
					case SDLK_s:
						return STATE_QUALITY;
						break;
					case SDLK_RETURN:
						if (event.key.keysym.mod & KMOD_ALT)
						{
							HotKey_ToggleFullScreen();
						}
					break;
					default:
						/* Do nothing */
						return STATE_RUNNING;
						;
				}

				break;

			case SDL_KEYUP:
				break;

		}
	}

	return STATE_RUNNING;
}

static void setup_opengl( int width, int height )
{
	SDL_Surface *screen;
	unsigned int flags;
	int bpp = 32;

	/* Initialize SDL defaults and video */
	if((SDL_Init(SDL_INIT_VIDEO)==-1)) { 
		printf("Could not initialize SDL: %s.\n", SDL_GetError());
		exit(-1);
	}

	/* Clean up on exit */
	atexit(SDL_Quit);

	/* Set up preliminary opengl stuff */
	SDL_GL_SetAttribute( SDL_GL_RED_SIZE, 8 );
	SDL_GL_SetAttribute( SDL_GL_GREEN_SIZE, 8 );
	SDL_GL_SetAttribute( SDL_GL_BLUE_SIZE, 8 );
	SDL_GL_SetAttribute( SDL_GL_ALPHA_SIZE, 8 );
	SDL_GL_SetAttribute( SDL_GL_DEPTH_SIZE, 16 );
	SDL_GL_SetAttribute( SDL_GL_DOUBLEBUFFER, 1 );

	flags = SDL_OPENGL;
	
	/* make a surface for OpenGL to use */
	screen = SDL_SetVideoMode(width, height, bpp, flags);
	if ( screen == NULL ) {
		fprintf(stderr, "Couldn't set 512x512x16 video mode: %s\n",
						SDL_GetError());
		exit(1);
	}

	SDL_WM_SetCaption("SOM", "SOM");

	/* setup opengl specific stuff */

	glShadeModel( GL_FLAT );
	glEnable(GL_DEPTH_TEST);
	glDepthFunc(GL_LESS);

	/* antialiasing */
/*	glEnable(GL_LINE_SMOOTH);*/
	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
/*	glHint(GL_LINE_SMOOTH_HINT, GL_NICEST);*/
/*	glLineWidth(1.5);*/

	glClearColor( 0.0, 0.0, 0.0, 0.0 );

	glViewport( 0, 0, width, height );
	glMatrixMode( GL_PROJECTION );
	glLoadIdentity( );

/*	gluOrtho2D(0.0, width, 0.0, height);*/
	glOrtho(0.0, width, 0.0, height, -100, 100);

	glMatrixMode( GL_MODELVIEW );
	glLoadIdentity( );

	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
}

#if 0
void run_som_test(void)
{
	int state = STATE_RUNNING;
	int sample, now;
	int i;
	int r, c;
	int mr, mc;
	Symbol *p, *lp, *ips;
	double error;
	int drawing_mode = SOM_STYLE_ACTUAL;
	int once = 1;
	char letter;
	Input *inp;
	SOM *s;


	s = som_init(1, 100000, 40, 40, 0, 0, NULL);

	/* set up the input parameters */
	inp = input_init(1);

	now = SDL_GetTicks();
	sample = now + 33;
	i = 0;
	printf("Learning...\n");
	while(state != STATE_EXIT)
	{
		p = input_random_choice(inp);
		if (som_learn(s, p, &r, &c) == SOM_CLASSIFYING)
		{
			if (once == 1)
			{
				printf("Classifying\n");
				once = 0;
			}
		}


		/* draw at 30fps or so to give the computer time to do the processing.
			Also, sample input events at this time.. */
		if (now > sample)
		{
			glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
			state = process_events(&mr, &mc);
			switch (state)
			{
				case STATE_MOUSE_DOWN:
					if (mr < 0 || mr >= som_get_rows(s) || 
						mc < 0 || mc >= som_get_cols(s))
					{
						break;
					}
					lp = som_symbol_ref(s, mr, mc);
					if (lp != NULL)
					{
						printf("Location : r = %d, c = %d\n", mr, mc);
						printf("Neuron   : ");
						symbol_stdout(lp);
						ips = input_decode_to_sym(inp, lp, &error);
						printf("DecodeS  : ");
						symbol_stdout(ips);
						letter = input_decode_to_char(inp, lp, &error);
						printf("DecodeC  : '%c'\n", letter);
						printf("Error    : %f\n", error);
						printf("\n");
					}
					else
					{
						printf("Location isn't on map\n");
					}
					break;
				case STATE_QUALITY:
					drawing_mode = SOM_STYLE_QUALITY;
					break;
				case STATE_ACTUAL:
					drawing_mode = SOM_STYLE_ACTUAL;
					break;
				default:
					;
			}

			if (drawing_mode == SOM_STYLE_QUALITY)
			{
				som_draw(s, SOM_STYLE_QUALITY);
			} 
			else if (drawing_mode == SOM_STYLE_ACTUAL)
			{
				som_draw(s, SOM_STYLE_ACTUAL);
			}

			SDL_GL_SwapBuffers();

			sample = now + 33;
		}

		now = SDL_GetTicks();
	}

	input_free(inp);
	som_free(s);
}
#endif

void test_symbol(void)
{
	Symbol *sym;
	Symbol **list;
	int dims[32];

	sym = symbol_init(10);
	symbol_randomize(sym);

	dims[0] = 1;
	dims[1] = 5;
	dims[2] = 1;
	dims[3] = 3;

	printf("Original symbol: ");
	symbol_stdout(sym);

	list = symbol_unabstract(sym, dims, 4);

	printf("unabstracted symbols:\n");

	printf("1 dimension : ");
	symbol_stdout(list[0]);

	printf("5 dimensions: ");
	symbol_stdout(list[1]);

	printf("1 dimension : ");
	symbol_stdout(list[2]);

	printf("3 dimensions: ");
	symbol_stdout(list[3]);

	symbol_free(list[0]);
	symbol_free(list[1]);
	symbol_free(list[2]);
	symbol_free(list[3]);
	free(list);
}

void test_intqueue(void)
{
	int i;
	Symbol *sym;
	IntQueue *iq;

	iq = intqueue_init(2, 4);

	for (i = 0; i < 8; i++)
	{
		sym = symbol_init(3);
		symbol_set_all(sym, i);

		intqueue_enqueue(iq, sym);
		intqueue_stdout(iq);
	}

	intqueue_free(iq);
}

void test_cortex_ascii(char *filename)
{
	Cortex *core = NULL;
	int state = STATE_RUNNING;
	int sample, now, incr = 1000;
	int i, j, k;
	int mr, mc;
	int drawing_mode = SOM_STYLE_ACTUAL;
	int glerr;
	int iter = 0;
	char revc;
	double revcerr;

	char *alpha = "I am very happy that my reverse lookup code seems to work. "
					"It really fills me with joy that after such a long and "
					"hard amount of work, everything finally seems to be "
					"working. Let us see if the vision processor works.";

	Input *inp;
	InputResTable *irt;
	InputResolution *ires;
	Symbol *channels[10];
	int num_channels = 1;

	inp = input_init(1);
	core = cortex_init(filename);
/*	cortex_stdout(core);*/

	now = SDL_GetTicks();
	sample = now + incr;
	i = 0;
	while(state != STATE_EXIT)
	{
		/* get something from the input */
		channels[0] = symbol_copy(input_xlate_char(inp, alpha[i]));
/*		channels[0] = symbol_copy(input_random_choice_special(inp,2));*/

		/* make the cortex learn it */
		cortex_process(core, channels, num_channels, CORTEX_REQUEST_LEARN);

		
		/* figure out what to do, if anything */
		state = process_events(&mr, &mc);
		switch (state)
		{
			case STATE_MOUSE_DOWN:
/*				printf("Clicked row: %d, col %d\n", mr, mc);*/
				irt = cortex_resolve(core, mr, mc);
				if (irt != NULL)
				{
					/* print out the interesting stuff, then destroy it */
					for (j = 0; j < irt->num_inres; j++)
					{
						/* for each time slice going backwards in time for
							this input, print it out */
						ires = inputrestable_input(irt, j);
						if (ires->active == TRUE)
						{
							/* look at all of the time slices */
							for (k = 0; k < ires->num_time_steps; k++)
							{
								revc = input_decode_to_char(inp, 
										ires->resolution[k], &revcerr);
								printf("For input %d, time step -%d: ", j, k);
								printf("char '%c' error %f\n", revc, revcerr);

							}
						}
					}

					inputrestable_destroy(irt);
				}
				break;
			case STATE_QUALITY:
				drawing_mode = SOM_STYLE_QUALITY;
				break;
			case STATE_ACTUAL:
				drawing_mode = SOM_STYLE_ACTUAL;
				break;
			default:
				;
		}

		/* draw at 15fps or so to give the computer time to do the work */
		if (now > sample)
		{
			glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
/*			printf("Input char: %c, Iter: %d\n", alpha[i], iter);*/
			if (drawing_mode == SOM_STYLE_QUALITY)
			{
				cortex_draw(core, SOM_STYLE_QUALITY);
			} 
			else if (drawing_mode == SOM_STYLE_ACTUAL)
			{
				cortex_draw(core, SOM_STYLE_ACTUAL);
			}

			SDL_GL_SwapBuffers();

			sample = now + incr;
			printf("Glyphs per second: %d\n", iter);
			iter = 0;
		}

		now = SDL_GetTicks();

		glerr = glGetError();
		if (glerr != GL_NO_ERROR)
		{
			printf("Opengl Error: %s\n", gluErrorString(glerr));
		}

		i++;
		if (!(i%strlen(alpha))) {
			i = 0;
		}

		iter++;
	}

	cortex_free(core);
	input_free(inp);
}

void test_vinput(void)
{
	VInput *vinp;
	int r, c;
	int bitval;

	vinp = vinput_init(16, 16, 16, 16);

	/* print out the first glyph */

	for (r = 0; r < 16; r++) {
		for (c = 0; c < 16; c++) {
			bitval = vinput_glyph_bit(vinp, 1, 4, r, c);
			if (bitval == 0) {
				printf(".");
			} else {
				printf("#");
			}
		}
		printf("\n");
	}

	vinput_destroy(vinp);
}

void test_cortex_vision(char *filename)
{
	Cortex *core = NULL;
	CortexOutputTable *ctxout = NULL;
	int state = STATE_RUNNING;
	int sample, now, incr = 1000;
	int i;
	int mr, mc;
	int drawing_mode = SOM_STYLE_ACTUAL;
	int glerr;
	int iter = 0;
	int gindex;
	float ctx_row, ctx_col, row, col, n_row, n_col;

	VInput *vinp;
	InputResTable *irt = NULL;
	InputResTable *self = NULL;
	Symbol **channels;
	int num_channels = 16;

	vinp = vinput_init(16, 16, 16, 16);
	core = cortex_init(filename);
/*	cortex_stdout(core);*/

	now = SDL_GetTicks();
	sample = now - 1; /* draw one frame immediately */
	i = 0;
	gindex = 0;
	while(state != STATE_EXIT)
	{
		if (now > sample) 
		{
			glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
		}

		/* get something from the input */
		channels = vinput_glyph(vinp, gindex);
		gindex++;
		/* keep it in bounds of useable stuff in the glyph file */
		gindex %= 9 * 16;
/*		gindex %= 2 * 16;*/
/*		gindex %= 1;*/

		/* corrupt it a little */
		vinput_draw_glyph(vinp, channels, 300, 300);
		/* pixels, range, chance */
/*		vinput_corrupt(vinp, channels, 1, 1, 1, VINPUT_RANGE_RANDOM);*/
		vinput_draw_glyph(vinp, channels, 400, 300);

		/* make the cortex learn it */
		ctxout = 
			cortex_process(core, channels, num_channels, CORTEX_REQUEST_LEARN);

/*		cortex_output_table_stdout(ctxout);*/
		

		/* I must free the container, but the cortex owns the symbols */
		free(channels);

		/* figure out what to do, if anything */
		state = process_events(&mr, &mc);
		switch (state)
		{
			case STATE_MOUSE_DOWN:
				printf("Clicked: %d, %d\n", mc, mr);
				if (irt != NULL)
				{
					inputrestable_destroy(irt);
				}
				/* if I clicked on nothing, this goes to null, which
					effectively erases the glyph from the screen */
				irt = cortex_resolve(core, mr, mc);
				break;
			case STATE_QUALITY:
				drawing_mode = SOM_STYLE_QUALITY;
				break;
			case STATE_ACTUAL:
				drawing_mode = SOM_STYLE_ACTUAL;
				break;
			default:
				;
		}

		/* draw at the sample rate or so to give the computer time to do the 
			work */
		if (now > sample)
		{
			cortex_draw(core, drawing_mode);

			/* if the user clicked on the map draw the reverse lookup */
			if (irt != NULL)
			{
				vinput_draw_irt(vinp, irt, 600, 300);
			}

			/* reverse lookup the output channel (if available) and draw it */
			if (ctxout->output[0].active == TRUE) {
				symbol_get_6(ctxout->output[0].osym,
					&ctx_row, &ctx_col, &row, &col, &n_row, &n_col);

				self = cortex_resolve(core, ctx_row, ctx_col);
				if (self != NULL) {
					vinput_draw_irt(vinp, self, 500, 300);
					inputrestable_destroy(self);
				}
			}

			SDL_GL_SwapBuffers();

			sample = now + incr;
			printf("Glyphs per second: %d\n", iter);
			iter = 0;
		}

		/* get rid of the cortex output table */
		if (ctxout != NULL) {
			cortex_output_table_free(ctxout);
			ctxout = NULL;
		}

		now = SDL_GetTicks();

		if (now > sample) {
			glerr = glGetError();
			if (glerr != GL_NO_ERROR)
			{
				printf("Opengl Error: %s\n", gluErrorString(glerr));
			}
		}

		iter++;
	}

	cortex_free(core);
	vinput_destroy(vinp);
}

/*#define VISION_DEMO*/
#define TURING_DEMO
/*#define FS_DEMO*/

int main(int argc, char **argv)
{
	int width;
	int height;

/* The #if 0 in this function are for the vision cortex behavior */
#if defined(VISION_DEMO)
	char buf[2048];
	char filename[2048];
#endif

	width = WIDTH;
	height = HEIGHT;

#if defined(VISION_DEMO)
	/* call mojify on the cortex file */
	if (argc == 2) {
		sprintf(buf, "./mojify %s", argv[1]);
		if (system(buf) != 0)
		{
			printf("Problem running mojify...%d(%s)\n", errno, strerror(errno));
			exit(EXIT_FAILURE);
		}
	} else {
		printf("Supply a file please\n");
		exit(EXIT_FAILURE);
	}

	sprintf(filename, "a.lctx");
#endif

	setup_opengl(width, height);

	srand48(getpid());

#if defined(VISION_DEMO)
	test_cortex_vision(filename);
#endif

#if defined(TURING_DEMO)
	test_turing_machine();
#endif

#if defined(FS_DEMO)
	test_filesystem();
#endif
	
	exit(0);
}


















