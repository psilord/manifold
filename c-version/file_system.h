#ifndef VISION_H

/* A representation of a real File we'll be using as input to the mainfold
	file system */
typedef struct File_s
{
	char *filename;

/* These two comprise a effectively a primary key. */
	unsigned int uid;
	unsigned int fid;
	/* how big is the total file. */
	unsigned int size;

	/* From where are we about to read from this file? */
	unsigned int curr_pos;

	/* The actual file data we'll be learning then later classifying. */
	int fd;
} File;

typedef struct FileSystem_s
{
	/* The number of total files in the file system */
	int num_files;

	/* from which index in the files am I currently about to learn something? */
	int learning_index;

	/* all of the actual files. */
	File **files;

	/* Contains <uid, fid, pos> */
	SOM *location;

	/* Contains <d0, d1, d2, d3, d4, d5, d6, d7> */
	SOM *block;

	/* Contains <location> : <block> */
	SOM *map;

} FileSystem;

/* create a new file structure (with real file-names), ready to read. */
File* file_init(char *filename, unsigned int uid, unsigned int fid);

/* destroy a file object and close the fd */
void file_destroy(File *file);

/* Create the general file system object */
FileSystem* filesystem_init(int num_files, File **files, int iterations);

/* Destroy the file system object, closing all files inside of it and releasing
all memory. */
void filesystem_destroy(FileSystem *fs);

/* This returns an array of symbol pointers based upon learning_index.
	The zeroth element of the array is the symbol <uid, fid, pos> and
	the first element of the array is the symbol 
	<d0, d1, d2, d3, d4, d5, d6, d7> which was found at the zeroth's location.

	This function *side effects* the FileSystem structure by
	first reading 8 bytes from the curr_pos (padding with zeros
	if a short read happens) and then incrementing the learning
	index in the FileSystem to the next file.  
	
	NOTE: There are statistical edge cases in dealing with large
	and small files, etc, etc, etc. I havne't considered those
	edge cases yet.
	
	EXAMPLE: The above favors symbols in small files statistically,
	but it round robins the map 1 information.
*/
Symbol** filesystem_chunk(FileSystem *fs);

/* Here we actually train the SOMs. Return of we are classifying or not. */
int filesystem_train(FileSystem *fs);

/* TODO: draw the input resolution of a reverse lookup */
void filesystem_draw_irt(FileSystem *fs, InputResTable *irt, int x, int y);

/* TODO: draw a chunk of data */
void filesystem_stdout_chunk(FileSystem *fs, Symbol **chunk, int x, int y);

/* TODO: Corrupt a chunk of data. */
void filesystem_corrupt_symbol(FileSystem *vinp, Symbol **chunk, 
	float per_dimensions, float per_range, float per_chance, int range_style);


/* test the whole thing */
void test_filesystem(void);
#endif


