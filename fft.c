#include <stdio.h>
#include <math.h>
#include <stdlib.h>

#include "fft.h"
#include "main.h"
#include "complex.h"
#include "string.h"

//#undef M_PI
//#define M_PI 3.14159265358979323846264338327950288


/*
 *	Counts Fourier transform, also makes scaling
 */
C_ARRAY *fft(C_ARRAY *ca) {
	C_ARRAY *car;
	car = recFFT(ca);
	int len2 = car->max;
	int i;
	for (i=0; i<len2; i++) {
		car->c[i].re /= len2/2;
		car->c[i].im /= len2/2;
	}

	return car;
}

/*
 * Computes invers Fourier transform and scales the data properly
 *
 */
C_ARRAY *ifft(C_ARRAY *ca) {
	conjugate(ca);
	C_ARRAY *car = recFFT(ca);
	conjugate(ca);
	int i;
	for (i=0; i<car->max; i++) {
		car->c[i].re /= 2;
		car->c[i].im /= 2;
	}

	return car;
}

/*
 *	Recursively computes Fourier transform performed on input array "ca"
 *
 * 	C_ARRAY *ca	- input array of complex numbers ~ sample
 *
 */
C_ARRAY *recFFT(C_ARRAY *ca) {
	// Round the length of input array to the nearest power of 2
	int n = getPow(ca->len, 2);
	C_ARRAY *cy = allocCA(n);
	cy->len = n;

	if (n == 1) {
		cy->c[0] = ca->c[0];
		return cy;
	}

	C_ARRAY *ca_s = allocCA(n/2);
	C_ARRAY *ca_l = allocCA(n/2);
	C_ARRAY *cy_s, *cy_l;
	
	/*
	 *	Initialization of arrays:
	 *		"ca_s" - elements with even index from array "ca"
	 *		"ca_l" - elements with odd index from array "ca"
	 */
	int i;
	for (i=0; i<n/2; i++) {
		ca_s->c[ca_s->len++] = ca->c[2*i];
		ca_l->c[ca_l->len++] = ca->c[2*i + 1];
	}
	
	// Recursion ready to run
	cy_s = recFFT(ca_s);
	cy_l = recFFT(ca_l);
	
	// Use data computed in recursion
	for (i=0; i < n/2; i++) {
		// Multiply entries of cy_l by the twiddle factors e^(-2*pi*i/N * k)
		cy_l->c[i] = complexMult(polarToComplex(1, -2*M_PI*i/n), cy_l->c[i]);
	}
	for (i=0; i < n/2; i++) {
		cy->c[i] = complexAdd(cy_s->c[i], cy_l->c[i]);
		cy->c[i + n/2] = complexSub(cy_s->c[i], cy_l->c[i]);
	}

	// Release unnecessary memory
	freeCA(ca_s);	freeCA(ca_l);
	freeCA(cy_s); freeCA(cy_l);

	return cy;
}
