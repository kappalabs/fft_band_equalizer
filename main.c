#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <string.h>
#include <unistd.h>
#include <libgen.h>
#include <fcntl.h>
#include <errno.h>

// interpret data in chart
#include "gnuplot_i.h"
// my parts
#include "main.h"
#include "fft.h"
#include "complex.h"
#include "string.h"

#define MIN(a,b) ((a) < (b) ? (a) : (b))


#define DEBUG 50
static int debug = 0;
#ifdef DEBUG
//debug = DEBUG;

void logOut(char *s, int level, int rec) {
	if (level > debug) {
		int i; 
		int indl = 128;
		char *r_s = allocString(indl);
		for (i=0; i<rec; i++) strcat(r_s, "    ");

		(strlen(s)>0) ? (printf("%s%s", r_s, s)) : (printf("%s", s));
		free(r_s);
	}
}
#endif
#ifndef DEBUG
void logOut(char *s, int level, int rec) {}
#endif


/*
 *	Returns 1 if val is power of 2, 0 otherwise.
 */
int isPowOf2(int val) {
	return (val & (val-1)) == 0;
}

/*
 *	Returns nearest power of "base" (pow: 1,2,...), bigger or equal to "val"
 */
int getPow(int val, int base) {
	int temp = base;
	while(temp < val) {
		temp *= base;
	}

	return temp;
}

static void initDoubles(double *ds, int len) {
	int i;
	for (i=0; i<len; i++) {
		ds[i] = 0.0f;
	}
}

static double *allocDoubles(int len) {
	double *d;
	if ((d = (double *) malloc(len * sizeof(double))) == NULL) {
		perror("malloc");
		exit (ERROR_EXIT_CODE);
	}

	initDoubles(d, len);

	return d;
}

static C_ARRAY *readLine(FILE *fin) {
	char c;
	C_ARRAY *in_arr;
	//TODO: vyresit default pocet prvku - navic musi byt mocnina 2-ou
	in_arr = allocCA(1024);
	//TODO: # prvku?
	STRING *token = allocStr(32);
	// skip comments and empty lines
	c = getc(fin);
	while ((c == '#' || c == ' ' || c == '\t' || c == '\n') && c != EOF) {
		if (c == '#') {
			while ((c = getc(fin)) != '\n' && c != EOF);
		}
		c = getc(fin);
	}
	// read one line of input file
	while (c != '\n' && c != EOF && c != '#') {
		//initString(token, 32);
		initStr(token, token->max+1, 0);
		while (c == ' ' || c == '\t') {
			c = getc(fin);
		}
		// read one whole token
		while (c != ' ' && c != '\n' && c != '\t') {
			//strcat(token, c); 
			token->text[token->len++] = c;
			c = getc(fin);
		}
		// add new token to "in_arr"
		if (token->text != '\0') {
			double din = atof(token->text);
			if (in_arr->max - in_arr->len <= 20) {
				reallocCA(in_arr, getPow(in_arr->len + 1024, 2));
			}
			in_arr->c[in_arr->len].re = din;
			// printf("nacten token %.2f\n", in_arr.c[in_arr.len].re);
			in_arr->len++;
		}

		if (c == '\n') break;
	}
	freeStr(token);

	return in_arr;
}

static C_ARRS *readInput(char *file_name) {
	FILE *fin;

	if ((fin = fopen(file_name, "r")) == NULL) {
		logOut("Unable to open input file for reading.", 100, 0);
		exit (ERROR_EXIT_CODE);
	}

	//TODO: proper default max value (# of max input samples/lines)
	C_ARRS *inputs = allocCAS(8);
	do {
		// we need to allocate more space for input samples
		if (inputs->max - inputs->len <= 1) {
			reallocCAS(inputs, inputs->max + 8);
		}
		inputs->carrs[inputs->len] = readLine(fin);
	} while(inputs->carrs[inputs->len++]->len > 0);
	// We do not need the last one, it's empty
	freeCA(inputs->carrs[--inputs->len]);

	if (fclose(fin) == EOF) {
		logOut("Unable to close input file.", 100, 0);
		exit (ERROR_EXIT_CODE);
	}

	return inputs;
}

static void writeOutput(char *file_name, C_ARRAY *ca) {
	FILE *fout;

	if ((fout = fopen(file_name, "w")) == NULL) {
		logOut("Unable to open output file for writing.", 100, 0);
		exit (ERROR_EXIT_CODE);
	}

	fprintf(fout, "# Created by KAFOURIER, @timestamp <kappa@kappa>\n");
	fprintf(fout, "# name:\tcompl\n");
	fprintf(fout, "# type:\tcomplex matrix\n");
	fprintf(fout, "# rows:\t1\n");
	fprintf(fout, "# columns:\t%d\n", ca->len);


	int i;
	for (i=0; i<ca->len; i++) {
		fprintf(fout, " (%.14f,%.14f)", ca->c[i].re, ca->c[i].im);
	}

	if (fclose(fout) == EOF) {
		logOut("Unable to close output file.", 100, 0);
		exit (ERROR_EXIT_CODE);
	}
}


int main(int argc, char **argv) {
	C_ARRS *ins;
	
	int opt;
	while ((opt = getopt(argc, argv, "d:f:")) != -1) {
		switch(opt) {
			case 'd':
				//TODO: je argument opravdu cislo?
				debug = atoi(optarg);
				printf("Changing debug level to %d\n", debug);
				break;
			case 'f':
				printf("Reading input from file \"%s\"...\n", optarg);
				ins = readInput(optarg);
				break;
		}
	}
	printf("Got %d input samples\n", ins->len);

	gnuplot_ctrl * g;
	C_ARRAY *re, *ire;
	int i, j;
	for (i=0; i < ins->len; i++) {
		// input samples must have length equal to power of 2
		//ins->carrs[i]->len = getPow(ins->carrs[i]->len, 2);
		int ilen = ins->carrs[i]->len;
		int ilen2 = getPow(ilen, 2); //ins->carrs[i]->len;
		int imax = ins->carrs[i]->max;
		//TODO: vyresit rozsamplovani po castech 4096
//		ilen = 4*4096;
//		ilen2 = ilen;
//		imax = ilen;
		double *x = allocDoubles(imax);
		double *y = allocDoubles(imax);

		g = gnuplot_init();
		gnuplot_cmd(g, "set terminal png");
		gnuplot_setstyle(g, "lines");

		printf("INPUT %d, length&2 of %d:\n", i+1, ilen2);
		gnuplot_cmd(g, "set output \"input_%d.png\"", i+1);
		for (j=0; j<ilen; j++) {
			x[j] = j; y[j] = ins->carrs[i]->c[j].re;
		}
		gnuplot_plot_xy(g, x, y, ilen, "Input");
		gnuplot_close(g);

		char *fname = allocString(20);
		sprintf(fname, "input_%d.mat", i+1);
		writeOutput(fname, ins->carrs[i]);


		/*
		 *	COUNTS FOURIER TRANSFORM
		 */
		g = gnuplot_init();
		gnuplot_cmd(g, "set terminal png");
		printf("OUTPUT FOURIER %d:\n", i+1);
		gnuplot_cmd(g, "set output \"fourier_%d.png\"", i+1);
		gnuplot_setstyle(g, "lines");
		initDoubles(x, imax);
		initDoubles(y, imax);
		re = fft(ins->carrs[i]);
		for (j=0; j < ilen2/2; j++) {
//			x[j] = j*ilen/ilen2; y[j] = 0;
//			double mag = magnitude(re->c[j]);
//			if (mag==mag) {
//				y[j] = mag;
//			}
			x[j] = j;
			y[j] = decibel(re->c[j]);
		}
		gnuplot_set_ylabel(g, "dBFS");
		gnuplot_plot_xy(g, x, y, ilen2/2, "Fourier");
		gnuplot_close(g);

		sprintf(fname, "fourier_%d.mat", i+1);
		writeOutput(fname, re);

		/*
		 *	COUNTS INVERS FOURIER TRANSFORM
		 */
		g = gnuplot_init();
		gnuplot_cmd(g, "set terminal png");
		gnuplot_setstyle(g, "lines");
		printf("OUTPUT INVERS FOURIER %d:\n", i+1);
		gnuplot_cmd(g, "set output \"invers_%d.png\"", i+1);
		initDoubles(x, imax);
		initDoubles(y, imax);
		ire = ifft(re);
		for (j=0; j < ilen; j++) {
//			ire->c[j].im /= ilen/2;
//			ire->c[j].re /= ilen/2;
//
			x[j] = j; y[j] = ire->c[j].re;
		}
		char *dc = allocString(32);
		char *nq = allocString(32);
		formatComplex(ire->c[0], dc);
		formatComplex(ire->c[ilen2/2], nq);
		printf("DC slot = %s, Nyquist slot = %s\n", dc, nq);
		gnuplot_plot_xy(g, x, y, ilen, "Invers");
		gnuplot_close(g);

		sprintf(fname, "invers_%d.mat", i+1);
		writeOutput(fname, ire);

		printf("\n\n");

		free(fname);
		free(x); free(y);
		freeCA(ire); freeCA(re);
		//freeCA(ins->carrs[i]);
	}
	//free(ins->carrs);
	freeCAS(ins);

	return (0);
}
