#include <stdio.h>
#include <math.h>
#include <stdlib.h>

#include "equalizer.h"
#include "my_std.h"
#include "complex.h"
#include "string.h"

// Values are in Hz
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

/*
 *  For given center frequency and Octave fraction
 *   counts upper frequency edge and returns its value.
 */
double upperEdge(double center, int frac) {
	return center*pow(10.0, (3.0/(10.0*2*frac)));
}

/*
 *  For given center frequency and Octave fraction
 *   counts lower frequency edge and returns its value.
 */
double lowerEdge(double center, int frac) {
	return center/pow(10.0, (3.0/(10.0*2*frac)));
}

/*
 *  Allocates new Octave structure and returns pointer
 *   to it. Values of this structure are also initialized.
 */
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

/*
 *  Counts new band from given "center" frequency
 *   and octave structure "oct" and adds it to the
 *   end of this structure as linked list.
 */
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

/*
 *  Sets bands of Octave structure "oct" with
 *   frequency centers bellow "center" from
 *   the lowest to the highest frequency.
 */
void recPrev(struct octave *oct, double center) {
	double prev = getPrevCenter(center, oct->frac);
	if (prev < FQ_HEARABLE_LOWER_BOUND) {
		return;
	}
	recPrev(oct, prev);
	// Post-order recursion
	addBand(oct, prev);
}

/*
 *  Sets bands of Octave structure "oct" with
 *   frequency centers higher then "center" from
 *   the lowest to the highest frequency.
 */
void recNext(struct octave *oct, double center) {
	double next = getNextCenter(center, oct->frac);
	if (next > FQ_HEARABLE_UPPER_BOUND) {
		return;
	}
	addBand(oct, next);
	// Pre-order recursion
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
	log_out(65, "Octave [1/%d] length is %d\n", oct->frac, oct->len);
	struct band *b = oct->head;
	while (b != NULL) {
		log_out(61, "%.2f (%.2f; %.2f)\n", b->center, b->lowerE, b->upperE);
		b = b->next;
	}

	return oct;
}

/*
 *  For given "band_id" returns pointer to apropriate
 *   band from this position in Octave structure,
 *   NULL if position is incorrect.
 *   "band_id" should be from range [1; oct->len-1].
 */
struct band *getBand(struct octave *oct, int band_id) {
	if (band_id <= 0 || band_id > oct->len) {
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

/*
 *	Frees allocated memory for the whole Octave structure.
 */
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

/*
 *  Sets complex number of specific position and its opposite.
 */
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

/*
 *  Returns "average" complex number from given array
 *   and interval in it. Average is computed separately
 *   for both real part and imaginary part of the complex
 *   number.
 */
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

/*
 *  For given frequency range, adds and multiplies by given constant
 *   both parts of each complex number from array "ca".
 */
void modulateFreq(C_ARRAY *ca, int st, int tg, double mult, double adit, int srate) {
	int fst = freqToIndex(st, ca->max, srate);
	int ftg = freqToIndex(tg, ca->max, srate);

	log_out(41, "modulate: st=%u, fst=%u; tg=%u, ftg=%u\n", st, fst, tg, ftg);
	modulate(ca, fst, ftg, mult, adit);
}

/*
 *  For given range in given array "ca", adds and multiplies
 *   by given constant both parts of each complex number
 *   from array "ca".
 */
void modulateBand(C_ARRAY *ca, struct octave *oct, int index, double mult, double adit, int srate) {
	struct band *b;
	b = getBand(oct, index);

	if (b != NULL) {
		modulateFreq(ca, b->lowerE, b->upperE, mult, adit, srate);
	}
}

/*
 *  Using flat function, modulates given band in array "ca" with
 *   gain in dBFS units.
 */
void flatBand(C_ARRAY *ca, struct band *b, int srate, double gain) {
	// Converts frequency to position in "ca" array
	int fst = freqToIndex(b->lowerE, ca->max, srate);
	int ftg = freqToIndex(b->upperE, ca->max, srate);

	log_out(45, "flatBand from %.2fHz to %.2fHz with gain %.2fdB\n", b->lowerE, b->upperE, gain);
	log_out(31, "fst = %d, ftg = %d\n", fst, ftg);

	int i;
	for (i=fst; i < ftg; i++) {
		// For every position, the gain is constant
		COMPLEX nc = gainToComplex(ca->c[i], gain);
		setCA(ca, i, nc.re, nc.im);
	}
}

/*
 *  Using peak function, modulates given band in array "ca" with
 *   gain in dBFS units.
 */
void peakBand(C_ARRAY *ca, struct band *b, int srate, double gain) {
	// Converts frequency to position in "ca" array
	int fst = freqToIndex(b->lowerE, ca->max, srate);
	int ftg = freqToIndex(b->upperE, ca->max, srate);

	log_out(45, "peakBand from %.2fHz to %.2fHz with gain %.2fdB\n", b->lowerE, b->upperE, gain);
	log_out(31, "fst = %d, ftg = %d\n", fst, ftg);

	double aktgain;
	int i;
	for (i=fst; i < ftg; i++) {
		// Counts how the gain should look like on this position
		//  quadratic polynomial is used here
		aktgain = gain - (gain/pow((ftg-fst)/2, 2))*pow(i-fst-(ftg-fst)/2, 2);
		COMPLEX nc = gainToComplex(ca->c[i], aktgain);
		setCA(ca, i, nc.re, nc.im);
	}
}

/*
 *  Using next function, modulates given band in array "ca" with
 *   gain in dBFS units.
 */
void nextBand(C_ARRAY *ca, struct band *b, int srate, double gain) {
	// Converts frequency to position in "ca" array
	int fst = freqToIndex(b->lowerE, ca->max, srate);
	int ftg = freqToIndex(b->upperE, ca->max, srate);

	log_out(45, "nextBand from %.2fHz to %.2fHz with gain %.2fdB\n", b->lowerE, b->upperE, gain);
	log_out(31, "fst = %d, ftg = %d\n", fst, ftg);

	int nfst = (ftg-fst)/2 + fst;
	int nftg = (ftg-fst) + nfst;

	double aktgain;
	int i;
	for (i=nfst; i < nftg; i++) {
		// End of samples, no next band to adjust
		if (ca->len < i) {
			return;
		}
		// Counts how the gain should look like on this position
		//  sin() function is used in this case
		aktgain = gain*sin(((i-nfst) * (M_PI/2.0))/(nftg-nfst));
		COMPLEX nc = gainToComplex(ca->c[i], aktgain);
		setCA(ca, i, nc.re, nc.im);
	}
}

/*
 *  Applies Hamming window function on given array of values.
 */
void hammingWindow(C_ARRAY *ca, double alpha, double beta) {
	int i;
	for (i=0; i<ca->len; i++) {
		ca->c[i].re *= alpha - beta*cos((2.0*M_PI*i)/(ca->len-1));
	}
}

/*
 *  Function used in computation of Planck window function.
 */
double planck(int n, double epsilon, int sgn, int N) {
	double ret;
	ret = 2.0*epsilon*(1.0/(1.0 + (sgn*2.0*n)/(N-1)) + 1.0/(1.0 - 2.0*epsilon + (sgn*2.0*n)/(N-1)));

	return ret;
}

/*
 *  Applies Planck window function on given array of values.
 */
void planckWindow(C_ARRAY *ca, double epsilon) {
	int i;
	for (i=0; i<ca->len; i++) {
		if (i < ca->len*epsilon) {
			double v;
			v = 1.0/(pow(M_E, planck(i-ca->len/2, epsilon, 1.0, ca->len))+1.0);
			ca->c[i].re *= v;
		} else if (i > ca->len*(1-epsilon)) {
			ca->c[i].re *= 1.0/(pow(M_E, planck(i-ca->len/2, epsilon, -1.0, ca->len))+1.0);
		}
	}
}

/*
 *  Function used in computation of Tukey window function.
 */
double tukey(int n, double alpha, int climb, int len) {
	return 1.0/2.0*(1.0 + cos(M_PI*((2.0*n)/(alpha*(len-1.0)) -1.0 + (1.0-climb)*(-2.0/alpha + 2.0))));
}

/*
 *  Applies Tukey window function on given array of values.
 */
void tukeyWindow(C_ARRAY *ca, double alpha) {
	int i;
	for (i=0; i<ca->len; i++) {
		if (i < (alpha*(ca->len-1))/2) {
			ca->c[i].re *= tukey(i, alpha, 1.0, ca->len/1); 
		} else if (i > (ca->len-1)*(1-alpha/2)) {
			ca->c[i].re *= tukey(i, alpha, 0, ca->len/1); 
		}
	}
}

/*
 *  Smooth discrete points on the edge of window function.
 */
void smooth(C_ARRAY *ca, int st, int tg) {
		COMPLEX av;
		av.im = ca->c[st-1].im + ca->c[tg+1].im;
		av.im /= 2.0;
		av.re = ca->c[st-1].re + ca->c[tg+1].re;
		av.re /= 2.0;
	if (st > 0 && st <= ca->len) {
		int i;
		for (i=st; i<tg; i++) {
			ca->c[i] = av;
		}
	}
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
 *  Computes invers Fourier transform and scales the data properly.
 */
C_ARRAY *ifft(C_ARRAY *ca) {
	conjugate(ca);
	C_ARRAY *car = recFFT(ca);
	conjugate(ca);
	int i;
	for (i=0; i<car->max; i++) {
		car->c[i].re /= 2.0;
		car->c[i].im /= 2.0;
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
	int n = get_pow(ca->len, 2);
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
	freeCA(ca_s); freeCA(ca_l);
	freeCA(cy_s); freeCA(cy_l);

	return cy;
}
