#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <errno.h>

#include "complex.h"
#include "string.h"


COMPLEX complexAdd(COMPLEX c1, COMPLEX c2) {
	COMPLEX c = {c1.re + c2.re, c1.im + c2.im};

	return c;
}

COMPLEX complexSub(COMPLEX c1, COMPLEX c2) {
	COMPLEX c = {c1.re - c2.re, c1.im - c2.im};

	return c;
}

COMPLEX complexMult(COMPLEX c1, COMPLEX c2) {
	double re = c1.re*c2.re - c1.im*c2.im;
	double im = c1.re*c2.im + c1.im*c2.re;
	COMPLEX c = {re, im};

	return c;
}

COMPLEX polarToComplex(double r, double fi) {
	COMPLEX c;
	c.re = r * cos(fi);
	c.im = r * sin(fi);

	return c;
}

double getRe(COMPLEX comp) {
	return comp.re;
}

double getIm(COMPLEX comp) {
	return comp.im;
}

/*
 *	Returns magnitude of given complex number
 */
double magnitude(COMPLEX c) {
	return sqrt(c.re*c.re + c.im*c.im);
}

/*
 *	Returns phase of given complex number
 */
double phase(COMPLEX comp) {
	return atan(comp.im/comp.re);
}

/*
 *	Returns representation in decibels (dBFS) value
 */
double decibel(COMPLEX comp) {
	return 20*log10(magnitude(comp));
}


void formatComplex(COMPLEX c, char *str) {
	sprintf(str, "(%.3f; %.3fi)", c.re, c.im);
}

void conjugate(C_ARRAY *ca) {
	int i;
	for (i=0; i<ca->len; i++) {
		ca->c[i].im *= -1;
	}
}


/*
 *	FUNCTIONS FOR WORKING WITH ARRAY OF COMPLEX NUMBERS / SAMPLES
 */

/*
 *	Initialization of elements in given array "ca"
 *		from "start" to the end of this array
 */
void initCA(C_ARRAY *ca, unsigned int len, unsigned int start) {
	ca->len=start; ca->max=len;

	int i;
	for (i=ca->len; i < ca->max; i++) {
		ca->c[i].re = 0;
		ca->c[i].im = 0;
	}
}

C_ARRAY *allocCA(unsigned int nlen) {
	C_ARRAY *n_arr;
	if ((n_arr = (C_ARRAY *) malloc(sizeof(C_ARRAY))) == NULL) {
		perror("malloc");
		return NULL;
	}

	if ((n_arr->c = (COMPLEX *) malloc(nlen * sizeof(COMPLEX))) == NULL) {
		perror("malloc");
		return NULL;
	}

	initCA(n_arr, nlen, 0);

	return n_arr;
}

void reallocCA(C_ARRAY *ca, unsigned int nlen) {
	int olen = ca->max;					// save previous length
	
	if ((ca->c = (COMPLEX *) realloc(ca->c, nlen * sizeof(COMPLEX))) == NULL) {
		perror("malloc");
	}

	initCA(ca, nlen, olen);
}

void freeCA(C_ARRAY *ca) {
	free(ca->c);
	ca->c = NULL;
	free(ca);
	ca = NULL;
}


/*
 *	FUNCTIONS FOR WORK WITH ARRAY OF COMPLEX SAMPLES
 */

/*
 *	Initialization of structure C_ARRS to given length
 */
C_ARRS *initCAS(C_ARRS *cas, unsigned int len) {
	cas->len = 0;
	cas->max = len;

	return cas;
}

/*
 *	int len - length of array of samples to allocate
 *
 *	returns pointer to allocated and initialized C_ARRS structure
 */
C_ARRS *allocCAS(unsigned int len) {
	C_ARRS *cas;

	if ((cas = (C_ARRS *) malloc(sizeof(C_ARRS))) == NULL) {
		perror("malloc");
		return NULL;
	}
	initCAS(cas, len);

	if ((cas->carrs = (C_ARRAY **) malloc(len * sizeof(C_ARRAY *))) == NULL) {
		perror("malloc");
		return NULL;
	}

	return cas;
}

void reallocCAS(C_ARRS *cas, unsigned int nlen) {
	cas->max = nlen;
	if ((cas->carrs = (C_ARRAY **) realloc(cas->carrs, cas->max * sizeof(C_ARRAY *))) == NULL) {
		perror("realloc");
	}
}

void freeCAS(C_ARRS *cas) {
	int i;
	for (i=0; i<cas->len; i++) {
		freeCA(cas->carrs[i]);
		cas->carrs[i]=NULL;
	}
	free(cas->carrs);
	cas->carrs = NULL;
	free(cas);
	cas = NULL;
}
