
#ifndef EQUALIZER_H_
#define EQUALIZER_H_

#include "complex.h"
#include "wave.h"

/*
 *	One band structure has information about specific band
 *	 in Octave, which is controllable. Variables are in Hz units
 */
struct band {
	double center;
	double upperE;
	double lowerE;
	struct band *next;
};

/* Stores informations about selected Octave, which includes
 *  number of all bands, selected fraction and bands itself
 *   as linked list
 */
struct octave {
	struct band *head;	// Head of linked list
	int frac;	// Selected Octave fraction (bigger means more bands to control)
	int len;	// Number of elements in linked list
};


extern struct band *getBand(struct octave *oct, int band_id);

extern void peakBand(C_ARRAY *samples, struct band *b, int sample_rate, double gain);
extern void flatBand(C_ARRAY *samples, struct band *b, int sample_rate, double gain);
extern void nextBand(C_ARRAY *samples, struct band *b, int sample_rate, double gain);

extern struct octave *initOctave(int from, int frac);
extern void freeOctave(struct octave *oct);

extern void hammingWindow(C_ARRAY *ca, double alpha, double beta);
extern void planckWindow(C_ARRAY *ca, double epsilon);
extern void tukeyWindow(C_ARRAY *ca, double alpha);

extern C_ARRAY *fft(C_ARRAY *ca);
extern C_ARRAY *ifft(C_ARRAY *ca);
extern C_ARRAY *recFFT(C_ARRAY *ca);

#endif
