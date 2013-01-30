#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>

#include "SDL.h"   /* All SDL App's need this */
#include <GL/gl.h>
#include <GL/glu.h>

#include "common.h"

extern int process_events(int *row, int *col);

// What states can I write to the tape?
#define ONE 1.0
#define ZERO 0.0
#define BLANK 0.5

// Which way does the head move?
#define RIGHT 0.0
#define LEFT 1.0

// Do I change the cell the head is under?
#define NO_CHANGE 0.0
#define CHANGE 1.0

TuringMachine* turing_machine_init(int iterations)
{
	int i;
	TuringMachine *tmach = NULL;

	tmach = (TuringMachine*)xmalloc(sizeof(TuringMachine) * 1);

	// hand allocate the perception states, there are only 6 of them

	tmach->perceptions = xmalloc(sizeof(Symbol*) * 6);

	// Here we specify the perceptions we're learning. The first index 
	// (in the symbol) represents reads from the state variable, 
	// the second value represents reads from the head off the tape.
	tmach->perceptions[0] = symbol_init(2);
	symbol_set_2(tmach->perceptions[0], NO_CHANGE, ONE);
	tmach->perceptions[1] = symbol_init(2);
	symbol_set_2(tmach->perceptions[1], NO_CHANGE, ZERO);
	tmach->perceptions[2] = symbol_init(2);
	symbol_set_2(tmach->perceptions[2], NO_CHANGE, BLANK);

	tmach->perceptions[3] = symbol_init(2);
	symbol_set_2(tmach->perceptions[3], CHANGE, ZERO);
	tmach->perceptions[4] = symbol_init(2);
	symbol_set_2(tmach->perceptions[4], CHANGE, ONE);
	tmach->perceptions[5] = symbol_init(2);
	symbol_set_2(tmach->perceptions[5], CHANGE, BLANK);

	// ///////////////////////////////////////////////////////
	// Here we specify the actions that will be associated with the
	// above perceptions. perception 0 will result in action 0, and
	// so on when the TuringMachine is working properly.
	// ///////////////////////////////////////////////////////

	// The first index (in the symbol) represents the change state
	// we'll be writing into the state variable. The second index represents
	// the value we'll be writing to the tape. The third index represents
	// the direction we'll move the head.

	tmach->actions = xmalloc(sizeof(Symbol*) * 6);

	tmach->actions[0] = symbol_init(3);
	symbol_set_3(tmach->actions[0], NO_CHANGE, ONE, RIGHT);
	tmach->actions[1] = symbol_init(3);
	symbol_set_3(tmach->actions[1], NO_CHANGE, ZERO, RIGHT);
	tmach->actions[2] = symbol_init(3);
	symbol_set_3(tmach->actions[2], CHANGE, BLANK, LEFT);
	tmach->actions[3] = symbol_init(3);
	symbol_set_3(tmach->actions[3], NO_CHANGE, ONE, RIGHT);
	tmach->actions[4] = symbol_init(3);
	symbol_set_3(tmach->actions[4], CHANGE, ZERO, LEFT);
	tmach->actions[5] = symbol_init(3);
	symbol_set_3(tmach->actions[5], NO_CHANGE, ONE, RIGHT);
	
	// Allocate the SOMs
	tmach->perception = som_init(2, iterations, 128, 128, NULL);
	tmach->action = som_init(3, iterations, 128, 128, NULL);

	// We start learning at the first perception/action rule.
	tmach->symdex = 0;

	// Turing Machine internal state
	tmach->state = 0;
	tmach->head = TAPE_INITIAL_POSITION;
	for (i = 0; i < TAPE_SIZE; i++) {
		tmach->tape[i] = BLANK;
	}

	tmach->step = 0;

	return tmach;
}

int turing_machine_train(TuringMachine *tmach)
{
	int state;
	int bmu_row, bmu_col;

	// we round robin across the perception and action symbols.
	// First, learn the perception and get the BMU from the learning.
	// NOTE: we don't save the state since both SOMs are lock stepped in
	// when they'll change to classifying.
	som_learn(tmach->perception, tmach->perceptions[tmach->symdex], 
		&bmu_row, &bmu_col, 0, 128, SOM_REQUEST_LEARN, FALSE);

	// Then, take the BMU from the perception SOM, and use it as the
	// direct place we are to learn the action in the actions SOM.
	state = som_learn(tmach->action, tmach->actions[tmach->symdex], 
		&bmu_row, &bmu_col, 128, 128, SOM_REQUEST_LEARN, TRUE);
	
	tmach->symdex++;
	tmach->symdex %= 6;

	return state;
}

void turing_machine_step(TuringMachine *tmach)
{
	int bmu_row, bmu_col;
	Symbol *precept = NULL;
	Symbol *act = NULL;
	float new_state, write_value, head_move;

	precept = symbol_init(2);

	symbol_set_2(precept, tmach->state, tmach->tape[tmach->head]);
	printf("Precept: ");
	symbol_stdout(precept);

	// find the BMU in the perception SOM for the precept
	som_learn(tmach->perception, precept,
		&bmu_row, &bmu_col, 0, 128, SOM_REQUEST_CLASSIFY, FALSE);

	// look it up in the action SOM to find the relation
	act = som_symbol_ref(tmach->action, bmu_row, bmu_col);
	symbol_get_3(act, &new_state, &write_value, &head_move);
	printf("Action: ");
	symbol_stdout(act);

	// Don't quantify these, leave them as is numerically.
	// This is to show the turing machine can work in the presence of noise.
	tmach->state = new_state;
	tmach->tape[tmach->head] = write_value;
	if (head_move < .5) {
		tmach->head++;
		if (tmach->head == TAPE_SIZE) {
			printf("Turing machine off tape boundary.\n");
			exit(EXIT_FAILURE);
		}
	} else {
		tmach->head--;
		if (tmach->head < 0) {
			printf("Turing machine off tape boundary.\n");
			exit(EXIT_FAILURE);
		}
	}

	symbol_free(precept);

	tmach->step++;
}

void turing_machine_free(TuringMachine *tmach)
{
	int i;

	// Free perception/action symbols
	for (i = 0; i < 6; i++) {
		symbol_free(tmach->perceptions[i]);
		symbol_free(tmach->actions[i]);
	}
	free(tmach->perceptions);
	free(tmach->actions);

	som_free(tmach->perception);
	som_free(tmach->action);
	
	free(tmach);
}

void turing_machine_stdout(TuringMachine *tmach)
{
	int i;

	printf("---] Step %d\n", tmach->step);
	printf("State: %f\n", tmach->state);

	// emit tape
	for (i = 0; i < TAPE_SIZE; i++) {
		if (tmach->tape[i] >= 0 && tmach->tape[i] < .25) {
			printf("0");
		}
		if (tmach->tape[i] >= .25 && tmach->tape[i] < .75) {
			printf(".");
		}
		if (tmach->tape[i] >= .75) {
			printf("1");
		}
	}
	printf("\n");

	// emit head location
	for (i = 0; i < TAPE_SIZE; i++) {
		if (i == tmach->head) {
			printf("^");
		} else {
			printf(" ");
		}
	}
	printf("\n");
}

// Since this 
void test_turing_machine(void)
{
	int state = STATE_RUNNING;
	int tmach_state = SOM_LEARNING;
	int drawing_mode = SOM_STYLE_ACTUAL;
	int sample, now, incr = 1000;
	int mr, mc;
	int iter;
	int glerr;
	TuringMachine *tmach = turing_machine_init(3000);
	Symbol *lookup = NULL;
	float val0, val1, val2;

	now = SDL_GetTicks();
	sample = now - 1; /* draw one frame immediately */
	iter = 0;
	turing_machine_stdout(tmach);
	while(state != STATE_EXIT)
	{
		if (now > sample || tmach_state == SOM_CLASSIFYING) 
		{
			glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
		}

		/* figure out what to do, if anything */
		state = process_events(&mr, &mc);
		switch (state)
		{
			case STATE_MOUSE_DOWN:
				printf("Clicked: %d, %d\n", mc, mr);
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

		/* do the processing for learning */
		tmach_state = turing_machine_train(tmach);

		if (state == STATE_MOUSE_DOWN) {
			if (mc >= 0 && mc < 128 && mr >= 0 && mr < 128) {
				printf("Perception symbol: ");
				lookup = som_symbol_ref(tmach->perception, mr, mc);
				symbol_get_2(lookup, &val0, &val1);
				printf("<state=%f, head=%f>\n", val0, val1);

				printf("  Associated request: ");
				lookup = som_symbol_ref(tmach->action, mr, mc + 128);
				symbol_get_3(lookup, &val0, &val1, &val2);
				printf("<state=%f, write=%f, move=%f>\n", val0, val1, val2);
			}

			if (mc >= 128 && mc < 256 && mr >= 0 && mr < 128) {
				printf("Action symbol: ");
				lookup = som_symbol_ref(tmach->action, mr, mc - 128);
				symbol_get_3(lookup, &val0, &val1, &val2);
				printf("<state=%f, write=%f, move=%f>\n", val0, val1, val2);
			}
		}

		if (tmach_state == SOM_CLASSIFYING) {
			// Perceive the machine state, execute a step of the Turing
			// machine, and print out the new state.
			printf("Execute a step.\n");
			turing_machine_step(tmach);
			turing_machine_stdout(tmach);
			usleep(250);
		}

		if (now > sample || tmach_state == SOM_CLASSIFYING)
		{
			som_draw(tmach->perception, drawing_mode, 0, 0);
			som_draw(tmach->action, drawing_mode, 128, 0);
			glerr = glGetError();
			if (glerr != GL_NO_ERROR) {
				printf("Opengl Error: %s\n", gluErrorString(glerr));
			}

			SDL_GL_SwapBuffers();

			sample = now + incr;
			//printf("Iterations per second: %d\n", iter);
			iter = 0;
		}

		now = SDL_GetTicks();
		iter++;
	}

	turing_machine_free(tmach);
}
