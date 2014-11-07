
#ifndef EQUALIZER_H_
#define EQUALIZER_H_

#include "complex.h"
#include "wave.h"

struct band {
	double center;
	double upperE;
	double lowerE;
	struct band *next;
};

struct octave {
	struct band *head;
	int frac;
	int len;
};


extern void modulate(C_ARRAY *ca, int st, int tg, double mult, double adit);
extern void modulateFreq(C_ARRAY *ca, int st, int tg, double mult, double adit, int sample_rate);
extern void flatFreq(C_ARRAY *ca, int st, int tg, int srate);

extern struct octave *initCenters(int from, int frac);
extern void freeOctave(struct octave *oct);

extern C_ARRAY *fft(C_ARRAY *ca);
extern C_ARRAY *ifft(C_ARRAY *ca);
extern C_ARRAY *recFFT(C_ARRAY *ca);

#endif
