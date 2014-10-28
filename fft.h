
#ifndef FFT_H_
#define FFT_H_

#include "complex.h"

extern C_ARRAY *fft(C_ARRAY *ca);
extern C_ARRAY *ifft(C_ARRAY *ca);
extern C_ARRAY *recFFT(C_ARRAY *ca);

#endif
