#include <stdio.h>
#include <math.h>
#include <stdlib.h>

#include "equalizer.h"
#include "main.h"
#include "complex.h"
#include "string.h"

#define FQ_HEARABLE_UPPER_BOUND 22000
#define FQ_HEARABLE_LOWER_BOUND 20

/*
 *	For given frequency and desired octave fraction
 *	 returns next frequency center
 */
double getNextCenter(double from, int frac) {
	return from*pow(10.0, 3.0/(10.0*frac));
}

/*
 *	For given frequency and desired octave fraction
 *	 returns previous frequency center
 */
double getPrevCenter(double from, int frac) {
	return from/pow(10.0, 3.0/(10.0*frac));
}

double upperEdge(double center, int frac) {
	return center*pow(10.0, (3.0/(10.0*2*frac)));
}

double lowerEdge(double center, int frac) {
	return center/pow(10.0, (3.0/(10.0*2*frac)));
}

struct octave *allocOctave(int frac) {
	struct octave *oct;
	if ((oct = (struct octave *) malloc(sizeof(struct octave))) == NULL) {
		perror("malloc");
		return NULL; 
	}
	oct->head = NULL;
	oct->frac = frac;
	oct->len = 0;

	return oct;
}

void addBand(struct octave *oct, double center) {
	struct band *b;
	if ((b = (struct band *) malloc(sizeof(struct band))) == NULL) {
		perror("malloc");
		exit(ERROR_EXIT_CODE);
	}

	b->center = center;
	b->lowerE = lowerEdge(center, oct->frac);
	b->upperE = upperEdge(center, oct->frac);
	b->next = NULL;

	if (oct->len == 0) {
		oct->head = b;
	} else {
		struct band *aktb = oct->head;
		while (aktb->next != NULL) {
			aktb = aktb->next;
		}
		aktb->next = b;
	}
	oct->len++;
}

void recPrev(struct octave *oct, double center) {
	double prev = getPrevCenter(center, oct->frac);
	if (prev < FQ_HEARABLE_LOWER_BOUND) {
		return;
	}
	recPrev(oct, prev);
	addBand(oct, prev);
}

void recNext(struct octave *oct, double center) {
	double next = getNextCenter(center, oct->frac);
	if (next > FQ_HEARABLE_UPPER_BOUND) {
		return;
	}
	addBand(oct, next);
	recNext(oct, next);
}

/*
 *	Creates structure containing bands specific to the given frac
 *	 base frequency "base" is usually 1000Hz (standard base for
 *	 ISO Octaves)
 */
struct octave *initOctave(int base, int frac) {
	struct octave *oct;
	oct = allocOctave(frac);

	recPrev(oct, base);
	addBand(oct, base);
	recNext(oct, base);

//TODO: EXPERIMENTAL: vypis spoctenych pasem teto Octave
	printf("Octave [1/%d] length is %d\n", oct->frac, oct->len);
	struct band *b = oct->head;
	while (b != NULL) {
		printf("%.2f (%.2f; %.2f)\n", b->center, b->lowerE, b->upperE);
		b = b->next;
	}

	return oct;
}

struct band *getBand(struct octave *oct, int band_id) {
	printf("%d\n", band_id);
	if (band_id <= 0 || band_id >= oct->len) {
					return NULL;
	}

	int i=0;
	struct band *b;
	b = oct->head;
	while (++i < band_id) {
		b = b->next;
	}

	return b;
}

void freeOctave(struct octave *oct) {
	struct band *b, *nb;
	b = oct->head;
	while (b != NULL) {
		nb = b->next;
		free(b);
		b = NULL;
		b = nb;
	}

	free(oct);
	oct = NULL;
}

void setCA(C_ARRAY *ca, int pos, double re, double im) {
	ca->c[pos].re = re;
	ca->c[pos].im = im;

//	ca->c[ca->len-pos].re = re;
//	ca->c[ca->len-pos].im = im;
}

/*
 *	Function useful for finding index in CA array containing
 *	 Fourier transform of given frequency.
 *	Returns index, which corresponds to given frequency
 *	 in given sample rate.
 */
int freqToIndex(int freq, int len, int rate) {
	return ((unsigned int)(freq*len))/(unsigned int)rate;
}

/*
 *	Adds aditive constant or multiplies by multiplicative constant
 *	 every unit of "ca" starting from index "st" to index "tg" in given
 *	 C_ARRAY structure.
 */
void modulate(C_ARRAY *ca, int st, int tg, double mult, double adit) {
	int i;
	for (i=st; i<tg; i++) {
		setCA(ca, i, ca->c[i].re*mult, ca->c[i].im*mult);
		setCA(ca, i, ca->c[i].re+adit, ca->c[i].im+adit);
	}
}

COMPLEX average(C_ARRAY *ca, int st, int len) {
	COMPLEX av;
	av.re = 0.0;
	av.im = 0.0;

	int i;
	for (i=st; i < st+len; i++) {
		av.re += ca->c[i].re;
		av.im += ca->c[i].im;
	}
	av.re /= len;
	av.im /= len;

	return av;
}

void modulateFreq(C_ARRAY *ca, int st, int tg, double mult, double adit, int srate) {
	int fst = freqToIndex(st, ca->max, srate);
	int ftg = freqToIndex(tg, ca->max, srate);

	printf("modulate: st=%u, fst=%u; tg=%u, ftg=%u\n", st, fst, tg, ftg);
	modulate(ca, fst, ftg, mult, adit);
}

void modulateBand(C_ARRAY *ca, struct octave *oct, int index, double mult, double adit, int srate) {
	struct band *b;
	b = getBand(oct, index);

	if (b != NULL) {
		modulateFreq(ca, b->lowerE, b->upperE, mult, adit, srate);
	}
}

void peakBand(C_ARRAY *ca, struct band *b, int srate, double gain) {
	//TODO: modify ca
	printf("peakBand from %.2f to %.2f\n", b->lowerE, b->upperE);
	printf("Uninmplemented method: peakBand");
}

void flatBand(C_ARRAY *ca, struct band *b, int srate, double gain) {
	//TODO: compute value for gain
	COMPLEX av = average(ca, b->lowerE, b->upperE - b->lowerE);

	int i;
	for (i = b->lowerE; i < b->upperE; i++) {
		setCA(ca, i, av.re, av.im);
	}
}

void nextBand(C_ARRAY *ca, struct band *b, int srate, double gain) {
	//TODO: modify ca
	printf("nextBand from %.2f to %.2f\n", b->lowerE, b->upperE);
	printf("Uninmplemented method: nextBand");
}
/*
 *	Counts Fourier transform, also makes scaling
 */
C_ARRAY *fft(C_ARRAY *ca) {
	C_ARRAY *car;
	car = recFFT(ca);
	int len2 = car->max;
	int i;
	for (i=0; i<len2; i++) {
		if (i<len2/2) {
			car->c[i].re /= len2/4;///2;
			car->c[i].im /= len2/4;///2;
		}
		else {
			car->c[i].re = 0.0;
			car->c[i].im = 0.0;
		}
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
