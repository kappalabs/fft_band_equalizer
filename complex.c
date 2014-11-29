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
 *       Filename:  complex.c
 *
 *    Description:  Functions useful for storing sound information in array
 *                  of complex numbers and basic operations with this arrays.
 *                  It also has some functions connected with operations with
 *                  complex numbers in general.
 *
 *         Author:  Vojtech Vasek
 *
 * ==============================================================================
 */


#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <errno.h>

#include "complex.h"
#include "my_std.h"
#include "string.h"


/*
 *  Returns COMPLEX number as a result of addition
 *   of two given COMPLEX numbers "c1" and "c2".
 */
COMPLEX complexAdd(COMPLEX c1, COMPLEX c2) {
	COMPLEX c = {c1.re + c2.re, c1.im + c2.im};

	return c;
}

/*
 *  Returns COMPLEX number as a result of subtraction
 *   of two given COMPLEX numbers "c1" and "c2".
 */
COMPLEX complexSub(COMPLEX c1, COMPLEX c2) {
	c2.re *= -1; c2.im *= -1;
	COMPLEX c = complexAdd(c1, c2); 

	return c;
}

/*
 *  Returns COMPLEX number as a result of multiplication
 *   of two given COMPLEX numbers "c1" and "c2".
 */
COMPLEX complexMult(COMPLEX c1, COMPLEX c2) {
	double re = c1.re*c2.re - c1.im*c2.im;
	double im = c1.re*c2.im + c1.im*c2.re;
	COMPLEX c = {re, im};

	return c;
}

/*
 *  Conversion to polar coordinate system. Output
 *   coordinates are put in COMPLEX number.
 */
COMPLEX polarToComplex(double r, double fi) {
	COMPLEX c;
	c.re = r * cos(fi);
	c.im = r * sin(fi);

	return c;
}

/*
 *  Returns COMPLEX number with parts set so that decibel
 *   value is moved with value gain from previous.
 */
COMPLEX gainToComplex(COMPLEX cin, double gain) {
	COMPLEX cret;
	double aktdec = decibel(cin);
	double MC = 1.0;
	if (cin.im != 0 || cin.re == 0) {
		MC = sqrt(pow(10.0, gain*0.1));
	}
	cret.re = cin.re * MC;
	cret.im = cin.im * MC;

	log_out(11, "before=%.2fdB after=%.2fdB, dif = %.2f\n", aktdec, decibel(cret), decibel(cret)-aktdec);
	return cret;
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
 *  Returns real part of given COMPLEX number.
 */
double getRe(COMPLEX comp) {
	return comp.re;
}

/*
 *  Returns imaginary part of given COMPLEX number.
 */
double getIm(COMPLEX comp) {
	return comp.im;
}

/*
 *  Returns magnitude of given complex number.
 */
double magnitude(COMPLEX c) {
	return sqrt(c.re*c.re + c.im*c.im);
}

/*
 *  Returns phase of given complex number.
 */
double phase(COMPLEX comp) {
	return atan(comp.im/comp.re);
}

/*
 *  Returns representation in decibels (dBFS) value.
 */
double decibel(COMPLEX comp) {
	return 20.0 * log10(magnitude(comp));
}


/*
 *  SECTION OF FUNCTIONS FOR CHANGING C_ARRAY VALUES
 */

/*
 *  Set every element of given array to conjugate complex number.
 */
void conjugate(C_ARRAY *ca) {
	int i;
	for (i=0; i<ca->len; i++) {
		ca->c[i].im *= -1.0;
	}
}

/*
 *  Set complex number of specific position to given value.
 */
void setCA(C_ARRAY *ca, int pos, double re, double im) {
	ca->c[pos].re = re;
	ca->c[pos].im = im;
}


/*
 *  FUNCTIONS FOR WORKING WITH ARRAY OF COMPLEX NUMBERS i.e. SOUND TRACK
 */

/*
 *  Initialization of elements in given array "ca"
 *   from "start" to the end (ca->max) of this array,
 *   which will be firstly set to the value "len".
 */
void initCA(C_ARRAY *ca, unsigned int len, unsigned int start) {
	ca->len=start; ca->max=len;

	int i;
	for (i=ca->len; i < ca->max; i++) {
		ca->c[i].re = 0;
		ca->c[i].im = 0;
	}
}

/*
 *  Allocate and initialize C_ARRAY structure with "len"
 *   COMPLEX numbers in it, all of them are set to zero.
 *   Return pointer to this structure if allocation was
 *   successful.
 */
C_ARRAY *allocCA(unsigned int len) {
	C_ARRAY *n_arr;
	if ((n_arr = (C_ARRAY *) malloc(sizeof(C_ARRAY))) == NULL) {
		perror("malloc");
		return NULL;
	}

	if ((n_arr->c = (COMPLEX *) malloc(len * sizeof(COMPLEX))) == NULL) {
		perror("malloc");
		return NULL;
	}

	initCA(n_arr, len, 0);

	return n_arr;
}

/*
 *  Reallocate given C_ARRAY structure, so that it has "new_len"
 *   COMPLEX numbers in it.
 */
void reallocCA(C_ARRAY *ca, unsigned int new_len) {
	int olen = ca->max;					// save previous length
	
	if ((ca->c = (COMPLEX *) realloc(ca->c, new_len * sizeof(COMPLEX))) == NULL) {
		perror("malloc");
	}

	initCA(ca, new_len, olen);
}

/*
 *  Free allocated memory for given C_ARRAY structure and set
 *   its pointers to NULL.
 */
void freeCA(C_ARRAY *ca) {
	free(ca->c);
	ca->c = NULL;
	free(ca);
	ca = NULL;
}

/*
 *  Copy "len" elements from "ca_in" starting at index "st_in" to
 *   array "ca_out" starting from index "st_out".
 */
void copyCA(C_ARRAY *ca_in, int st_in, C_ARRAY *ca_out, int st_out, int len) {
	int i;
	for (i=0; i<len; i++) {
		ca_out->c[i+st_out] = ca_in->c[st_in+i];
	}
	ca_out->len += len;
}

/*
 *  FUNCTIONS FOR WORK WITH ARRAY OF COMPLEX SAMPLES
 */

/*
 *  Basic initialization of structure C_ARRS, only its
 *   values "len" and "max" are set.
 */
C_ARRS *initCAS(C_ARRS *cas, unsigned int len) {
	cas->len = 0;
	cas->max = len;

	return cas;
}

/*
 *  Allocate and initialize C_ARRS structure with memory for "len"
 *   pointers to C_ARRAY structures.
 *   Return pointer to this structure, NULL if allocation fails.
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

/*
 *  Allocate "new_len" C_ARRAY structures in given C_ARRS structure.
 */
void reallocCAS(C_ARRS *cas, unsigned int new_len) {
	cas->max = new_len;
	if ((cas->carrs = (C_ARRAY **) realloc(cas->carrs, cas->max * sizeof(C_ARRAY *))) == NULL) {
		perror("realloc");
	}
}

/*
 *  Release memory allocated for whole C_ARRS structure,
 *   set all pointers to NULL after that.
 */
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
