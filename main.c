#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <string.h>
#include <unistd.h>
#include <libgen.h>
#include <fcntl.h>
#include <errno.h>
#include <ctype.h>

// interpret data in chart
#include "gnuplot_i.h"
// My header files
#include "main.h"
#include "fft.h"
#include "complex.h"
#include "string.h"
#include "wave.h"


#define DEBUG 90
static int debug = 0;
#ifdef DEBUG
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

// this will store name of this program
char const * program_name;

/*
 *	Returns 1 if given integer "val" is power of 2, 0 otherwise.
 */
int isPowOf2(int val) {
	return (val & (val-1)) == 0;
}

/*
 *	Returns nearest power of integer value "base", which is bigger
 *	 or equal to given value "val".
 */
int getPow(int val, int base) {
	int nearest = 1;	// initialization to the first power of every integer
	while (nearest < val) {
		nearest *= base;
	}

	return nearest;
}

static void initDoubles(double *ds, unsigned int len) {
	int i;
	for (i=0; i<len; i++) {
		ds[i] = 0.0f;
	}
}

static double *allocDoubles(unsigned int len) {
	double *d;
	if ((d = (double *) malloc(len * sizeof(double))) == NULL) {
		perror("malloc");
		exit (ERROR_EXIT_CODE);
	}

	initDoubles(d, len);

	return d;
}

/*
 *	Loads one input sample ~ array of real numbers into
 *	 C_ARRAY structure. Skips empty lines and everything
 *	 after character '#', which can be used for comments.
 */
static C_ARRAY *readLine(FILE *fin) {
	char c;
	C_ARRAY *in_arr;
	in_arr = allocCA(512);
	STRING *token = allocStr(16);

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
		// read one whole token ~ real number (double)
		while (c != ' ' && c != '\n' && c != '\t') {
			// we need to allocate more space for this number
			if (token->max - token->len < 1) {
				reallocStr(token, token->max + 8);
			}
			token->text[token->len++] = c;
			c = getc(fin);
		}
		// add new token to "in_arr"
		if (token->text != '\0') {
			double din = atof(token->text);
			// we need to allocate space for few more numbers
			if (in_arr->max - in_arr->len <= 20) {
				reallocCA(in_arr, getPow(in_arr->len + 512, 2));
			}
			in_arr->c[in_arr->len].re = din;
			in_arr->len++;
		}

		if (c == '\n') break;
	}
	freeStr(token);

	return in_arr;
}

/*
 *	Loads data from input file to structure C_ARRS.
 *	Each input sample is on its line.
 */
static void readInput(C_ARRS *inputs, char *file_name) {
	FILE *fin;

	if ((fin = fopen(file_name, "r")) == NULL) {
		perror("fopen");
		exit (ERROR_EXIT_CODE);
	}

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
		perror("fclose");
		exit (ERROR_EXIT_CODE);
	}
}

/*
 *	Writes array of complex numbers to specified file in Octave format
 */
static void writeOutput(char *file_name, C_ARRAY *ca) {
	FILE *fout;

	if ((fout = fopen(file_name, "w")) == NULL) {
		perror("fopen");
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
		perror("fclose");
		exit (ERROR_EXIT_CODE);
	}
}

static void usage(void) {
	fprintf(stderr, "Usage: %s -f file_in [-w] [-d level]\n"
									"   -f in_file: set the name of an input file to \"file_in\"\n"
									"   -w:         input file is in WAV format\n"
									"   -d level:   changes debug level to \"level\", smaller value means more info\n"
									"        (default value is 90, maximum is 100)\n", program_name);
	exit (ERROR_EXIT_CODE);
}

int main(int argc, char **argv) {
	program_name = basename(argv[0]);

	C_ARRS *ins;
	ins = allocCAS(8);

	int opt;
	int f_flag=0;	// read input from file in_file
	int w_flag=0;	// treat input as file in WAV format
	char *in_file;// stores name of input file (if f_flag==1)
	while ((opt = getopt(argc, argv, "f:wd:")) != -1) {
		switch(opt) {
			case 'f':
				// set in_file name to the value of the next argument
				f_flag = 1;
				in_file = optarg;
				break;
			case 'w':
				// read in_file as WAV sound file
				w_flag = 1;
				break;
			case 'd':
				// get debug level (integer value)
				debug = atoi(optarg);
				printf("Changing debug level to %d\n", debug);
				break;
			case '?':
				usage();
				break;
		}
	}

	// in_file is required argument
	if (f_flag == 0) {
		fprintf(stderr, "in_file required\n");
		usage();
	}

	// w_flag was not set, read in_file as raw input data ~ default
	if (w_flag == 0) {
		printf("Reading raw data from file \"%s\"...\n", in_file);
		readInput(ins, in_file);
	}
	// w_flag was set, read in_file as WAV
	else {
		printf("Reading wav input file from \"%s\"...\n", in_file);
		readWav(ins, in_file);
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
		printf("INPUT %d, length->^2 of %d:\n", i+1, ilen2);
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
			x[j] = j; y[j] = ire->c[j].re;
		}
		char *dc = allocString(32);
		char *nq = allocString(32);
		formatComplex(ire->c[0], dc);
		formatComplex(ire->c[ilen2/2], nq);
		printf("DC slot = %s, Nyquist slot = %s\n", dc, nq);
		free(dc); free(nq);
		gnuplot_plot_xy(g, x, y, ilen, "Invers");
		gnuplot_close(g);

		sprintf(fname, "invers_%d.mat", i+1);
		writeOutput(fname, ire);

		printf("\n\n");

		free(fname);
		free(x); free(y);
		freeCA(ire); freeCA(re);
	}
	freeCAS(ins);

	return (0);
}
