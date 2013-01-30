#ifndef _T_INPUT_H_
#define _T_INPUT_H_

#define TAPE_SIZE 70
#define TAPE_INITIAL_POSITION (TAPE_SIZE / 2)

typedef struct TuringMachine_s {
	// Used for learning the steps the machine must take
	Symbol **perceptions;
	Symbol **actions;

	// The current index into the perception/actions that we're learning
	int symdex;

	// The world in which the state machine perceives and acts
	float state;
	int head;
	float tape[TAPE_SIZE];

	// The maps themselves;
	SOM *perception;
	SOM *action;

	// Keep track of the execution step for display
	int step;

} TuringMachine;

void test_turing_machine(void);
TuringMachine* turing_machine_init(int iterations);
int turing_machine_train(TuringMachine *tmach);

#endif
