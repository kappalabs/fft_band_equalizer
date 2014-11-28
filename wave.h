
#ifndef WAVE_H_
#define WAVE_H_

#include "complex.h"


typedef enum {
	LE = 0,
	BE = 1,
} ENDIAN;

/*
 *  This structure represents one line.
 */
struct element {
	ENDIAN endian; // 1 - big endian, 0 - little endian
	int offset;    // in number of bytes
	short size;    // in number of bytes
	char *name;    // few characters specifying element name
	char *data;    // stores the data of this element itself
};


extern unsigned int getNumChannels(struct element *h);
extern unsigned long getSampleRate(struct element *h);
extern unsigned long getSubchunk2Size(struct element *h);

extern struct element *readWav(C_ARRS *cas, char *path);
extern void freeHeader(struct element *header);
extern void writeWav(struct element *h, C_ARRS *cas, char *fpath);

#endif
