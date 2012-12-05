#include <stdio.h>
#include <stdlib.h>
#include "common.h"

void* xmalloc(unsigned long size)
{
	void *space;
	
	space = malloc(size);
	if (space == NULL)
	{
		printf("Out of memory!\n");
		exit(EXIT_FAILURE);
	}

	return space;
}
