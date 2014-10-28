#include <stdio.h>
#include <math.h>
#include <stdlib.h>

#include "fft.h"
#include "main.h"
#include "complex.h"
#include "string.h"

#undef M_PI
#define M_PI 3.14159265358979323846264338327950288


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
 * Computes invers Fourier transform
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
	static int rec = 0;
	rec++;

	int n = getPow(ca->len, 2);
	//TODO: # znaku chybove hlasky?
	char *s = allocString(64);
	sprintf(s, "Part 1, n=%d\n", n);
	logOut(s, 10, rec);

	C_ARRAY *cy = allocCA(n);
	cy->len = n;

	if (isPowOf2(ca->max) != 1 || ca->len > ca->max) {
		sprintf(s, "Incorrect array length (%d)\n",n);
		logOut(s, 100, 0);
		free(s);

		exit (ERROR_EXIT_CODE);
	}
	if (ca->len == 1) {
		free(s);
		cy->c[0] = ca->c[0];
		logOut("RETURNING BACK UP\n", 10, rec);

		rec--;
		return cy;
	}

	C_ARRAY *ca_s = allocCA(n/2);
	C_ARRAY *ca_l = allocCA(n/2);
	C_ARRAY *cy_s, *cy_l;
	
	sprintf(s, "Part 2; %d/2 ~ %d\n", n, n/2);
	logOut(s, 10, rec);

	/*
	 *	initialization of array "ca_s" - elements with even index from array "ca"
	 *		and "ca_l" - elements with odd index from input array "ca"
	 */
	int i;
	for (i=0; i<n/2; i++) {
		ca_s->c[ca_s->len++] = ca->c[2*i];
		ca_l->c[ca_l->len++] = ca->c[2*i + 1];
	}
	sprintf(s, "i=%d, sude_len=%d, liche_len=%d\n", i, ca_s->len, ca_l->len);
	logOut(s, 3, rec);

	logOut("POLE a: ", 5, rec);
	for (i=0; i<n; i++) {
		formatComplex(ca->c[i], s);
		logOut(s, 5, 0);
	} logOut("\n", 5, 0);

	free(s);
	
	// Recursion ready to run
	logOut("Suda cast:\n", 9, rec);
	cy_s = recFFT(ca_s);
	logOut("Licha cast:\n", 9, rec);
	cy_l = recFFT(ca_l);
	
	// Use data computed in recursion
	s = allocString(64);

	logOut("Part 3:\n", 10, rec);
	for (i=0; i < n/2; i++) {
		/* Multiply entries of cy_l by the twiddle factors e^(-2*pi*i/N * k) */		
		cy_l->c[i] = complexMult(polarToComplex(1, -2*M_PI*i/n), cy_l->c[i]);
	}

	for (i=0; i < n/2; i++) {
		cy->c[i] = complexAdd(cy_s->c[i], cy_l->c[i]);
		cy->c[i + n/2] = complexSub(cy_s->c[i], cy_l->c[i]);
	}


	freeCA(ca_s);	freeCA(ca_l);
	freeCA(cy_s); freeCA(cy_l); // MP uvolnuje prvky *ca (mozna ca_s, ca_l), coz AC v tuto chvili nechci...
	// rozhodne vyhazuje chybu pri prvnim vynorovani

	free(s);
	rec--;
	return cy;
}
