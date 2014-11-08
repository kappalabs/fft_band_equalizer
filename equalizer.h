
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


extern struct band *getBand(struct octave *oct, int band_id);

extern void peakBand(C_ARRAY *samples, struct band *b, int sample_rate, double gain);
extern void flatBand(C_ARRAY *samples, struct band *b, int sample_rate, double gain);
extern void nextBand(C_ARRAY *samples, struct band *b, int sample_rate, double gain);

//TODO: MP nepotrebna
extern void modulateBand(C_ARRAY *ca, struct octave *oct, int index, double mult, double adit, int srate);

extern struct octave *initOctave(int from, int frac);
extern void freeOctave(struct octave *oct);

extern C_ARRAY *fft(C_ARRAY *ca);
extern C_ARRAY *ifft(C_ARRAY *ca);
extern C_ARRAY *recFFT(C_ARRAY *ca);

#endif
