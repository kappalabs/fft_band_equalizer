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
 *       Filename:  complex.h
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

#ifndef COMPLEX_H_
#define COMPLEX_H_

/*
 *  This structure represents single complex number.
 */
typedef struct {
	double re;   // real part of this number
	double im;   // imaginary part of this number
} COMPLEX;

/*
 *  This structure represents array of complex numbers
 *   useful for one input sound track.
 */
typedef struct {
	COMPLEX *c;         // array of complex numbers
	unsigned int len;   // number of COMPLEX numbers in array (*c)
	unsigned int max;   // allocated length of the array (*c)
} C_ARRAY;

/*
 *  This structure represents array of sound tracks 
 *   e.g. multiple sound channels.
 */
typedef struct {
	C_ARRAY **carrs;    // array of samples (sound tracks)
	unsigned int len;   // number of samples arrays (sound tracks) (**carrs)
	unsigned int max;   // allocated length of the array (**carrs)
} C_ARRS;


extern COMPLEX complexAdd(COMPLEX, COMPLEX);
extern COMPLEX complexSub(COMPLEX, COMPLEX);
extern COMPLEX complexMult(COMPLEX, COMPLEX);
extern COMPLEX polarToComplex(double, double);
extern COMPLEX gainToComplex(COMPLEX cin, double gain);
extern COMPLEX average(C_ARRAY *, int, int);

extern double getRe(COMPLEX comp);
extern double getIm(COMPLEX comp);
extern double magnitude(COMPLEX comp);
extern double phase(COMPLEX comp);
extern double decibel(COMPLEX comp);

extern void conjugate(C_ARRAY *);
extern void setCA(C_ARRAY *, int pos, double re, double im);

extern void initCA(C_ARRAY *, unsigned int len, unsigned int start);
extern C_ARRAY *allocCA(unsigned int len);
extern void reallocCA(C_ARRAY *, unsigned int new_len);
extern void freeCA(C_ARRAY *);
extern void copyCA(C_ARRAY *ca_in, int st_in, C_ARRAY *ca_out, int st_out, int len);

extern C_ARRS *initCAS(C_ARRS *, unsigned int len);
extern C_ARRS *allocCAS(unsigned int len);
extern void reallocCAS(C_ARRS *, unsigned int new_len);
extern void freeCAS(C_ARRS *);


#endif
