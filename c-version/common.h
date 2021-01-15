#ifndef COMMON_H
#define COMMON_H

enum {
	FALSE = 0,
	TRUE = 1
};

enum
{
	/* often used for describing whether or not I found something in an array */
	NOT_FOUND = -1
};

enum 
{
	STATE_RUNNING,
	STATE_QUALITY,
	STATE_ACTUAL,
	STATE_MOUSE_DOWN,
	STATE_EXIT
};

enum 
{
	NOISE_RANGE_DOWN,
	NOISE_RANGE_UP,
	NOISE_RANGE_RANDOM
};

#include "utils.h"
#include "symbol.h"
#include "slq.h"
#include "som.h"
#include "input.h"
#include "intqueue.h"
#include "cortex.h"
#include "reverse.h"
#include "conv.h"

/* the various input modalities (which amount to demos) */
#include "vinput.h"
#include "turing_machine.h"
#include "file_system.h"

#endif
