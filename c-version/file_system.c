#include <GL/gl.h>
#include <GL/glu.h>
#include <string.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <limits.h>
#include <ctype.h>
#include <math.h>

#include "SDL.h"   /* All SDL App's need this */
#include <GL/gl.h>
#include <GL/glu.h>

#include "common.h"

extern int process_events(int *row, int *col);

/* TODO:
	I can't use the Cortex language for this concept since I
	don't have a representation of the "association" operator. That
	would probably look something like:

	B, C, D -> E:1/1/1 @ A

	which states:
		At location A on E, learn the abstraction of B, C, D, integrating 1/1/1
	
	I should implement that, but it may require some heavy changes, especially
	in reverse lookup.
*/

/* Represent a source File from which we're going to train. */
File* file_init(char *filename, unsigned int uid, unsigned int fid)
{
	File *file = NULL;
	struct stat s;

	file = (File*)xmalloc(sizeof(File) * 1);

	file->filename = strdup(filename);
	file->uid = uid;
	file->fid = fid;

	file->fd = open(file->filename, O_RDONLY);
	if (file->fd < 0) {
		printf("Can't open file: '%s': %d(%s)\n",
			file->filename, errno, strerror(errno));
		exit(EXIT_FAILURE);
	}

	if (fstat(file->fd, &s) < 0) {
		printf("Can't fstat() file: '%s': %d(%s)\n",
			file->filename, errno, strerror(errno));
		exit(EXIT_FAILURE);
	}

	file->size = (unsigned int)s.st_size;

	file->curr_pos = 0; /* ensure we know we're at start of file */

	printf("file_init(): name='%s', uid=0x%x, fid=0x%x\n",
		filename, uid, fid);

	return file;
}

void file_destroy(File *file)
{
	close(file->fd);
	free(file->filename);
	free(file);
}

/* This function takes ownership of memory in files. */
FileSystem* filesystem_init(int num_files, File **files, int iterations)
{
	FileSystem *fs = NULL;

	fs = (FileSystem*)xmalloc(sizeof(FileSystem) * 1);

	fs->num_files = num_files;
	fs->learning_index = 0;
	fs->files = files;

	fs->location = som_init(12, iterations, 256, 256, NULL);
	/* make block bigger since it holds a lot of data */
	fs->block = som_init(8, iterations, 256, 512, NULL);
	fs->map = som_init(2, iterations, 256, 256, NULL);

	return fs;
}

void filesystem_destroy(FileSystem *fs)
{
	int i;	

	for(i = 0; i < fs->num_files; i++) {
		file_destroy(fs->files[i]);
	}

	som_free(fs->location);
	som_free(fs->block);
	som_free(fs->map);

	free(fs);
}

/* Return an array containing a <location> symbol and a <block> symbol
	from the FileSystem. Side effect the fs for the next read.
*/
Symbol** filesystem_chunk(FileSystem *fs)
{
	Symbol **container = NULL;
	Symbol *block = NULL;
	Symbol *location = NULL;
	unsigned char data[8] = {0};
	size_t bytes_read = 0;
	int i;

	File *file = fs->files[fs->learning_index];

	container = (Symbol**)xmalloc(sizeof(Symbol*) * 2);

	/* First we read some data */
	block = symbol_init(8);
	symbol_zero(block);

	from_beginning:
	bytes_read = readn(file->fd, data, 8);
	/*
	printf("Read %u bytes from file %s.\n", 
		(unsigned int)bytes_read, file->filename);

	printf("data is: "
		"[0x%02x(%c) 0x%02x(%c) 0x%02x(%c) 0x%02x(%c) "
		"0x%02x(%c) 0x%02x(%c) 0x%02x(%c) 0x%02x(%c)]\n",
		data[0], 
		isprint(data[0])?(int)data[0]:'.',
		data[1],
		isprint(data[1])?(int)data[1]:'.',
		data[2], 
		isprint(data[2])?(int)data[2]:'.',
		data[3], 
		isprint(data[3])?(int)data[3]:'.',
		data[4], 
		isprint(data[4])?(int)data[4]:'.',
		data[5], 
		isprint(data[5])?(int)data[5]:'.',
		data[6], 
		isprint(data[6])?(int)data[6]:'.',
		data[7],
		isprint(data[7])?(int)data[7]:'.'
		);
	*/

	switch(bytes_read) {
		case 0:
			/* EOF read */

			/* we wanted to read, but were immediately at end of file, so
			 	reseek to the beginning and reissue the command if the 
				file size is greater than zero.
				NOTE: We don't "wrap" the read to the beginning.
			*/
			if (file->size > 0) {
/*				printf("Retrying at start of file!\n");*/
				if (lseek(file->fd, 0, SEEK_SET) < 0) {
					printf("Can't lseek to beginning of file '%s': %d(%s)\n",
							file->filename, errno, strerror(errno));
					exit(EXIT_FAILURE);
				}
				file->curr_pos = 0;
				goto from_beginning;
			}
			break;

		default:
			/* full or short read */

			/* put however much we read into the symbol. */
			for (i = 0; i < bytes_read; i++) {
				symbol_set_index(block, data[i] / 255.0, i);
			}

			break;
	}

	container[1] = block;

	/* Then, we construct the location symbol given the learning_index. 
		This takes into consideration that we may have restarted the read
		due to finding EOF previously, etc, etc, etc.
	*/

	location = symbol_init(12);

	/* The encoding we do here is to fit each byte into one dimension.
		This allows the value to be isotropic in all
		dimensions as opposed to just say dividing curr_pos
		/ (float)UNIT_MAX and shoving that into a single
		dimension. If we did that, in the symbol space, there
		might be a few meaningful symbols in one dimension
		and thousands in another. I don't like that since the
		learning updates of assume isotropism of the information
		density in all dimensions.

		When we decode this, we must perform the same type of decoding as
		the block_stdout() function in order to deal with the fuzzy symbol.
	*/

	/* encode the uid of the file */
	symbol_set_index(location, ((file->uid & 0xFF000000) >> 24) / 255.0, 0);
	symbol_set_index(location, ((file->uid & 0x00FF0000) >> 16) / 255.0, 1);
	symbol_set_index(location, ((file->uid & 0x0000FF00) >> 8) / 255.0, 2);
	symbol_set_index(location, ((file->uid & 0x000000FF) >> 0) / 255.0, 3);

	/* encode the fid of the file */
	symbol_set_index(location, ((file->fid & 0xFF000000) >> 24) / 255.0, 4);
	symbol_set_index(location, ((file->fid & 0x00FF0000) >> 16) / 255.0, 5);
	symbol_set_index(location, ((file->fid & 0x0000FF00) >> 8) / 255.0, 6);
	symbol_set_index(location, ((file->fid & 0x000000FF) >> 0) / 255.0, 7);

	/* encode the curr_pos of the file */
	symbol_set_index(location,((file->curr_pos & 0xFF000000) >> 24) / 255.0, 8);
	symbol_set_index(location,((file->curr_pos & 0x00FF0000) >> 16) / 255.0, 9);
	symbol_set_index(location,((file->curr_pos & 0x0000FF00) >> 8) / 255.0, 10);
	symbol_set_index(location,((file->curr_pos & 0x000000FF) >> 0) / 255.0, 11);

	/* Now that we recorded the curr_pos from which we actually got the
		data, we add how many bytes we read to it to let us know where the
		next position will be the next time we read. */
	file->curr_pos += bytes_read;

	container[0] = location;

	/* Lastly, we iterate to the next file to learn and will read the next
		slice of data from the right pos from that file when come back to
		this function.
	*/
	fs->learning_index = (fs->learning_index + 1) % fs->num_files;

	return container;
}

/* Here we actually train the SOMs. Return of we're still training or have
	entered classification stage.
*/
int filesystem_train(FileSystem *fs)
{
	int state;
	Symbol **chunk = NULL;
	Symbol *location = NULL;
	Symbol *block = NULL;
	Symbol *map = NULL;
	int location_bmu_row, location_bmu_col;
	int block_bmu_row, block_bmu_col;

	chunk = filesystem_chunk(fs);

	/* the constiuent pieces if what we are about to learn. */
	location = chunk[0];
	block = chunk[1];

	/*
		****************************************
		Learn the block and location SOMs first until they converge.
		****************************************
	*/

	/* First we learn the block at any old place in the block som. We do it
		this way so if there are identical blocks in different files they
		get a modicum of compression. 
	*/
	som_learn(fs->block, block,
		&block_bmu_row, &block_bmu_col, 
						256, 0, SOM_REQUEST_LEARN, FALSE);
	
	som_draw_reticule(fs->block, 256, 0, block_bmu_row, block_bmu_col);

	/* Then we learn the uid/fid/pos at any old place in the location som. */
	state = som_learn(fs->location, location,
						&location_bmu_row, &location_bmu_col, 
						0, 0, SOM_REQUEST_LEARN, FALSE);

	som_draw_reticule(fs->block, 0, 0, location_bmu_row, location_bmu_col);

	/*
		****************************************
		Then, once the above is converged, learn the association.
		****************************************
	*/
	if (state == SOM_CLASSIFYING) {
		/* Now, for the map som, we learn the <block> position symbol at the
			<location> position on the map som. This is the association we
			desire.
		*/
		map = symbol_init(2);
		symbol_set_index(map, block_bmu_row / 255.0, 0);
		symbol_set_index(map, block_bmu_col / 511.0, 1);
		/* reset state to be the state of THIS map now that the other ones
			are done learning.
		*/
		state = som_learn(fs->map, map, &location_bmu_row, &location_bmu_col, 
						128, 256, SOM_REQUEST_LEARN, TRUE);
		som_draw_reticule(fs->block, 128, 256, location_bmu_row, 
							location_bmu_col);
		symbol_free(map);
		map = NULL;
	}

	/* and clean up */
	symbol_free(block);
	block = NULL;
	symbol_free(location);
	location = NULL;
	free(chunk);
	chunk = NULL;

	/* when all maps are done converging, we're SOM_CLASSIFYING */
	return state;
}

#if 0
void test_filesystem(void)
{
	File *files[4];
	FileSystem *fs = NULL;
	int i;

	files[0] = file_init("/etc/passwd", 0, 0);
	files[1] = file_init("/etc/hosts", 0, 1);
	files[2] = file_init("/etc/rsnapshot.conf", 0, 2);
	files[3] = file_init("/home/psilord/.bashrc", 1, 0);

	fs = filesystem_init(4, files, 100000);

	for (i = 0; i < 20; i++) {
		printf("Reading a chunk....\n");

		printf("location: ");
		symbol_stdout(chunk[0]);

		printf("block: ");
		symbol_stdout(chunk[1]);

		/* clean up memory */
		symbol_free(chunk[0]);
		symbol_free(chunk[1]);
		free(chunk);
	}	

	filesystem_destroy(fs);
}
#endif

void block_stdout(Symbol *block)
{
	unsigned int data[8];
	float val, integral, fractional;
	int i;

	for (i = 0; i < 8; i++) {
		symbol_get_index(block, &val, i);
		/* convert the value back into byte space (but it is continuous
			and we have to round it properly to the centers of the byte
			symbols. */
		val *= 255.0;

		fractional = modff(val, &integral);

		/* the symbol is centered around the integral part, so let's ensure
			we error correct the noise away in the right way. */
		data[i] = (unsigned int)(fractional > .5 ? ceil(val) : floor(val));

		if (data[i] > 255) {
			printf("data integrity failure\n");
			exit(EXIT_FAILURE);
		}
	}

	printf("<0x%02x 0x%02x 0x%02x 0x%02x 0x%02x 0x%02x 0x%02x 0x%02x :",
			data[0], 
			data[1],
			data[2], 
			data[3], 
			data[4], 
			data[5], 
			data[6], 
			data[7]
	);
	printf(" |%c%c%c%c%c%c%c%c|>\n",
			isprint(data[0])?data[0]:'.',
			isprint(data[1])?data[1]:'.',
			isprint(data[2])?data[2]:'.',
			isprint(data[3])?data[3]:'.',
			isprint(data[4])?data[4]:'.',
			isprint(data[5])?data[5]:'.',
			isprint(data[6])?data[6]:'.',
			isprint(data[7])?data[7]:'.'
	);
}

void location_stdout(Symbol *block)
{
	unsigned int data[12];
	float val, integral, fractional;
	int i;
	unsigned uid, fid, curr_pos;

	for (i = 0; i < 12; i++) {
		symbol_get_index(block, &val, i);
		/* convert the value back into byte space (but it is continuous
			and we have to round it properly to the centers of the byte
			symbols. */
		val *= 255.0;

		fractional = modff(val, &integral);

		/* the symbol is centered around the integral part, so let's ensure
			we error correct the noise away in the right way. */
		data[i] = (unsigned int)(fractional > .5 ? ceil(val) : floor(val));

		if (data[i] > 255) {
			printf("data integrity failure\n");
			exit(EXIT_FAILURE);
		}
	}

	uid = (data[0] << 24) | (data[1] << 16) | (data[2] << 8) | data[3];
	fid = (data[4] << 24) | (data[5] << 16) | (data[6] << 8) | data[7];
	curr_pos = (data[8] << 24) | (data[9] << 16) | (data[10] << 8) | data[11];

	printf("<uid=%u, fid=%u, curr_pos=%u>\n", uid, fid, curr_pos);
}

void test_filesystem(void)
{
	int state = STATE_RUNNING;
	int fs_state = SOM_LEARNING;
	int drawing_mode = SOM_STYLE_ACTUAL;

	File *files[10];
	FileSystem *fs = NULL;

	int iterations = 3000000;
	int iter_to_go = iterations * 2; /* cause I have two layers */
	int sample, now, incr = 1000;
	int mr, mc;
	int iter;
	int glerr;
	Symbol *lookup = NULL;
	int once = 0;

	files[0] = file_init("/etc/passwd", 0, 0);
	files[1] = file_init("/etc/hosts", 0, 1024);
	files[2] = file_init("/etc/rsnapshot.conf", 0, 2048);
/*	files[3] = file_init("/home/psilord/.bashrc", 1024, 0);*/

	fs = filesystem_init(3, files, iterations);

	now = SDL_GetTicks();
	sample = now - 1; /* draw one frame immediately */
	iter = 0;

	while(state != STATE_EXIT)
	{
		if (now > sample || fs_state == SOM_CLASSIFYING) 
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
		fs_state = filesystem_train(fs);

		if (state == STATE_MOUSE_DOWN) {
			/* location som (256 rows by 256 columns) */
			if (mc >= 0 && mc < 256 && mr >= 0 && mr < 256) {
				printf("Location symbol: \n");
				lookup = som_symbol_ref(fs->location, mr, mc);

				location_stdout(lookup);

				/* TODO: Add associated lookup in the map, then the block */
			}

			/* block som (256 rows by 512 columns) */
			if (mc >= 256 && mc < 768 && mr >= 0 && mr < 256) {
				printf("block som: \n");
				lookup = som_symbol_ref(fs->block, mr, mc - 256);

				block_stdout(lookup);

				/* TODO: Add associated map lookup */
			}
			
			/* map som (256 rows by 256 columns) */
			if (mc >= 128 && mc < 384 && mr >= 256 && mr < 512) {
				printf("map symbol: \n");

				/* TODO: Add associated location and block lookups */
			}
		}

		if (now > sample)
		{
			switch (fs_state) {
				case SOM_LEARNING:
					printf("Filesystem: SOM_LEARNING\n");
					break;
				case SOM_CLASSIFYING:
					if (once == 0) {
						printf("Filesystem: SOM_CLASSIFYING\n");
						once = 1;
					}
					break;
				default:
					break;
			}

			som_draw(fs->location, drawing_mode, 0, 0);
			som_draw(fs->map, drawing_mode, 128, 256);
			som_draw(fs->block, drawing_mode, 256, 0);

			glerr = glGetError();
			if (glerr != GL_NO_ERROR) {
				printf("Opengl Error: %s\n", gluErrorString(glerr));
			}

			SDL_GL_SwapBuffers();

			sample = now + incr;

			if (fs_state == SOM_LEARNING) {
				iter_to_go -= iter;
			} else { 
				iter_to_go = 0;
			}
			if (fs_state == SOM_LEARNING) {
				printf("Iterations per second: %d, to go: %d\n",
					iter, iter_to_go);
			}
			iter = 0;
		}

		now = SDL_GetTicks();
		iter++;
	}

	filesystem_destroy(fs);
}
