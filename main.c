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
#include "equalizer.h"
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

static void usage(void) {
	fprintf(stderr, "Usage: %s -f in_file [-w] [-r denom] [-k list] [-d level]\n"
									"   -f in_file: set the name of an input file to \"in_file\"\n\n"
									"   -w:         input file is in WAV format\n\n"
									"   -r denom:   set Octave bands control to [1/denom] (must be in range [1; 24] by standard ISO)\n"
									"        (default value is 1)\n\n"
									"   -k list:    list defines configuration of virtual knobs separated by commas, every knob has 3 properties:\n"
									"        band ID:  integer representing specific band, interval depends on choosen Octave fraction (-r option)\n"
									"        function: must be one of \"f\" for flat, \"p\" for peak, or \"n\" for next\n"
									"        gain:     integer value from range [-24; 24] (in dB) with, or without its sign\n"
									"        EXAMPLE:  -k \"1f+20,7n-24,42p+21\" (use flat function applied to the first band with gain 20dB, etc.)\n\n"
									"   -d level:   changes debug level to \"level\", smaller value means more info\n"
									"        (default value is 90, used range is [1; 100])\n", program_name);
	exit (ERROR_EXIT_CODE);
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


/*
 *	Structure used to save all modification that will
 *	 be applied in linked list - LL
 */
struct b_modif {
	// C_ARRAY *sample, int srate, double gain [-24,+24]
	void (*modif_f)(C_ARRAY *, struct band *, int, double);
	// which band will be modified
	int band_id;
	double gain;
	// pointer to allocated Octave structure
	struct octave *oct;
	// next operation in LL - linked list
	struct b_modif *next;
};

/*
 *	Adds new modification to the linked list structure
 */
void addModif(struct b_modif *head, char func, int band_id, double gain, struct octave *oct) {
	struct b_modif *nbm;
	if ((nbm = (struct b_modif *) malloc(sizeof(struct b_modif))) == NULL) {
		perror("malloc");
	}
	nbm->band_id = band_id;
	nbm->gain = gain;
	nbm->oct = oct;
	nbm->next = NULL;

	switch (func) {
		case 'p':
						nbm->modif_f = peakBand;
						break;
		case 'f':
						nbm->modif_f = flatBand;
						break;
		case 'n':
						nbm->modif_f = nextBand;
						break;
		default:
						fprintf(stderr, "Unknown modification function");
						usage();
						break;
	}

	if (head == NULL) {
		head = nbm;
	} else {
		struct b_modif *akt;
		akt = head;
		do {
			akt = akt->next;
		} while (akt->next != NULL);
		akt->next = nbm;
	}
}

/*
 *	Parse option inputs and appropriately initialize
 *	 b_modif as LL of these modifications.
 */
initModifs(struct b_modif *head, STRING bands_in) {
//TODO: format bude -t1f+24,5m-18,6p20,9n-18
	int i;
	while (i < bands_in.len) {
		char akt;
		while ((akt = bands_in.text[i]) != ',') {
			//while ((akt = bands_in[++i]) != '=') {
				struct b_oper *nb;
				//addModif(*head, func_char, (int)akt, gain, *octave);
				
				//TODO: cteni operace, optiony operace
			//}
		}
	}
}

/*
 *	Execute all of the modifications in the b_modif LL
 */
void processModifs(struct b_modif *head, C_ARRAY *ca, int srate) {
	struct b_modif *actb;
	actb = head;
	// go throught all of them and make the modification
	while (actb != NULL) {
		struct band *bnd = getBand(actb->oct, actb->band_id);
		actb->modif_f(ca, bnd, srate, actb->gain);
		actb = actb->next;
	}
}

int main(int argc, char **argv) {
	program_name = basename(argv[0]);

	C_ARRS *ins;
	ins = allocCAS(8);

	int opt;
	int f_flag=0;	// read input from file in_file
	int w_flag=0;	// treat input as file in WAV format
	int o_flag=0; // write output to file out_file
	int r_flag=0; // set Octave fraction, default is Octave [1/1]
	int r_value=1;
	char *in_file;  // stores name of input file (if f_flag==1)
	char *out_file; // stores name of output file (if o_flag==1)
	while ((opt = getopt(argc, argv, "f:wd:o:r:")) != -1) {
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
			case 'o':
				// write output to out_file in WAV format
				o_flag = 1;
				out_file = optarg;
			case 'd':
				// get debug level (integer value)
				debug = atoi(optarg);
				printf("Changing debug level to %d\n", debug);
				break;
			case 'r':
				// set Octave fraction denominator
				r_flag = 1;
				r_value = atoi(optarg);
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

	struct element *header;

	// w_flag was not set, read in_file as raw input data ~ default
	if (w_flag == 0) {
		printf("Reading raw data from file \"%s\"...\n", in_file);
		readInput(ins, in_file);
	}
	// w_flag was set, read in_file as WAV
	else {
		printf("Reading wav input file from \"%s\"...\n", in_file);
		header = readWav(ins, in_file);
	}

	struct octave *oct;

	// check if r_value is in correct range
	if (r_flag != 0 && r_value > 0 && r_value < 25) {
		printf("Changing to Octave [1/%d] bands\n", r_flag);
	} else if (r_flag != 0) {
		fprintf(stderr, "Ignoring r flag.\n");
		r_value = 1;
	} 
	oct = initOctave(1000, r_value);


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
//		ilen = 4096;
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
//TODO: EXPERIMENTAL
		int srate = getSampleRate(header);
		int pom = re->max;
		re->max = ilen2;
//modulateFreq(re, 0, srate/2, 0.0, 0.0, srate);
//flatFreq(re, 3000, 4000/*, srate/2*/, srate);
		for (j=0; j < oct->len; j++) {
			modulateBand(re, oct, j, 0.001, 0.0, srate);
		}
		re->max = pom;

		for (j=0; j < ilen2; j++) {
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

//TODO: EXPERIMENTAL: zapis vystupu do WAV souboru
		if (o_flag == 1) {
			C_ARRS *caso;
			caso = allocCAS(1);
			caso->carrs[caso->len++] = ire;
			writeWav(header, caso, out_file);

			free(caso->carrs);
			free(caso);
		}

		sprintf(fname, "invers_%d.mat", i+1);
		writeOutput(fname, ire);

		printf("\n\n");

		free(fname);
		free(x); free(y);
		freeCA(ire); freeCA(re);
	}
	freeHeader(header);
	freeOctave(oct);
	freeCAS(ins);

	return (0);
}
