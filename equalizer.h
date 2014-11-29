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
 *       Filename:  equalizer.h
 *
 *    Description:  This module contains functions for work with Octave, bands,
 *                  sound modification and the Fourier transform itself.
 *                  Octave and its bands are counted here from given
 *                  base frequency and fraction denominator.
 *                  Three window functions are Hamming(hammingWindow),
 *                  Planck(planckWindow) and Tukey(tukeyWindow). Sound
 *                  modification functions are called Flat(flatBand),
 *                  Peak(peakBand) and Next(nextBand).
 *
 *         Author:  Vojtech Vasek
 *
 * ==============================================================================
 */

#ifndef EQUALIZER_H_
#define EQUALIZER_H_

#include "complex.h"
#include "wave.h"


/*
 *  One band structure contains information about specific band
 *   of frequencies, which are controllable together. Variables
 *   are in Hz units.
 */
struct band {
	double center;
	double upperE;
	double lowerE;
	struct band *next;
};

/*  
 *  Stores informations about selected Octave, which includes
 *   number of all bands, selected fraction and bands itself
 *   as a linked list.
 */
struct octave {
	struct band *head;  // Head of the linked list
	int frac;           // Selected Octave fraction (bigger means more bands to control)
	int len;            // Number of elements in linked list
};


extern struct octave *initOctave(int base, int frac);
extern void freeOctave(struct octave *);

extern struct band *getBand(struct octave *, int band_id);

extern void modulateFreq(C_ARRAY *, int st, int tg, double mult, double adit, int srate);
extern void modulateBand(C_ARRAY *, struct octave *, int index, double mult, double adit, int sample_rate);

extern void flatBand(C_ARRAY *, struct band *, int sample_rate, double gain);
extern void peakBand(C_ARRAY *, struct band *, int sample_rate, double gain);
extern void nextBand(C_ARRAY *, struct band *, int sample_rate, double gain);

extern void hammingWindow(C_ARRAY *ca, double alpha, double beta);
extern void planckWindow(C_ARRAY *ca, double epsilon);
extern void tukeyWindow(C_ARRAY *ca, double alpha);

extern C_ARRAY *fft(C_ARRAY *ca);
extern C_ARRAY *ifft(C_ARRAY *ca);

#endif
