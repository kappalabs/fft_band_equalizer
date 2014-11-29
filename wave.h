/*
 * Copyright (c) 2014, Vojtech Vasek
 *

 *  This program is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation, either version 3 of the License, or
 *  (at your option) any later version.

 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.*
 *  
 *  You should have received a copy of the GNU General Public License
 *  along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 */
/*
 * ==============================================================================
 *
 *       Filename:  wave.h
 *
 *    Description:  This module is for work with WAV files.
 *                  It allows to read and write in this format.
 *                  This module defines structure ELEMENT, which is useful
 *                  for storing data from head of the WAV file, and enum
 *                  ENDIAN, which is useful for handeling data on byte-level.
 *
 *         Author:  Vojtech Vasek
 *
 * ==============================================================================
 */


#ifndef WAVE_H_
#define WAVE_H_

#include "complex.h"


/*
 *  This element simply represents type of endian.
 */
typedef enum {
	LE = 0,  /* Little Endian */
	BE = 1,  /* Big Endian */
} ENDIAN;

/*
 *  This structure represents one line.
 */
typedef struct {
	ENDIAN endian; /* BE=1 ~ big endian, xor 0=LE ~ little endian */
	int offset;    /* In number of bytes */
	short size;    /* In number of bytes */
	char *name;    /* Few characters specifying element name */
	char *data;    /* Stores the data of this element itself */
} ELEMENT;


extern unsigned int getNumChannels(ELEMENT *h);
extern unsigned long getSampleRate(ELEMENT *h);
extern unsigned long getSubchunk2Size(ELEMENT *h);

extern ELEMENT *readWav(C_ARRS *cas, char *path);
extern void freeHeader(ELEMENT *header);
extern void writeWav(ELEMENT *h, C_ARRS *cas, char *fpath);

#endif
