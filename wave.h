
#ifndef WAVE_H_
#define WAVE_H_

#include "complex.h"


struct element {
	unsigned short int endian;	// 1 - big endian, 0 - little endian
	unsigned int offset;				// in number of bytes
	unsigned short int size;		// in mumber of bytes
	char *name;									// few characters specifiing name
	short int *data;		// stores the data on this position itself
};



extern void readWav(C_ARRS *cas, char *path);

#endif
