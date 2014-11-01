
#ifndef FFT_H_
#define FFT_H_

#include "complex.h"
#include "wave.h"

extern void modulate(C_ARRAY *ca, int st, int tg, int mult);
extern void modulateFreq(C_ARRAY *ca, int st, int tg, int mult, int sample_rate);

extern C_ARRAY *fft(C_ARRAY *ca);
extern C_ARRAY *ifft(C_ARRAY *ca);
extern C_ARRAY *recFFT(C_ARRAY *ca);

#endif
