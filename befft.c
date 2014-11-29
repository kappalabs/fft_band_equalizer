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
 *       Filename:  befft.c
 *
 *    Description:  This program takes input file either with raw input data
 *                  where each input sample should be on separate line and
 *                  every value of this sample is separated by white-space.
 *                  If line starts with # character, then it's ignored, as well
 *                  as if it doesn't contains any non-white characters.
 *                  Input file can also be in WAV format, then you have to
 *                  write option -w.
 *
 *                  Band modification is defined by other option -k, format of
 *                  its value is [b(number)][f(char)][s(char)][g(number)],
 *                  where 'b' is selected band, 'f' is function for equalization,
 *                  's' is sign for 'g' which is gain in dB units from range
 *                  [-24; 24].
 *
 *                  For further details about options see usage, e.g. by running
 *                  ./befft without options.
 *
 *                  Output is either interpreted in graph, or as WAV sound file,
 *                  in case input was also in WAV file format.
 *
 *                  Every transformation is interpretted in graph, therefore
 *                  gnuplot program is required for this functionality.
 *
 *         Author:  Vojtech Vasek
 *
 * ==============================================================================
 */

#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <string.h>
#include <unistd.h>
#include <libgen.h>
#include <fcntl.h>
#include <errno.h>
#include <ctype.h>

/* Interpret data in chart */
#include "gnuplot_i.h"
/* My own modules */
#include "my_std.h"
#include "equalizer.h"
#include "complex.h"
#include "string.h"
#include "wave.h"

/* Size of one window (# of samples to transform in one step) */
#define WLEN (4096*1)


/* Stores the name of this program */
char const *program_name;

/*
 *  Print out to the standard output information about usage of this program.
 */
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
		"        EXAMPLE:  -k 1f+20,7n-24,42p+21 (use flat function applied to the first band with gain 20dB, etc.)\n\n"
		"   -d level:   changes debug level to \"level\", smaller value means more info\n"
		"        (default value is 90, used range is [1; 100])\n", program_name);
	exit (ERROR_EXIT_CODE);
}

/*
 *  Loads one input sample ~ array of real numbers into
 *   C_ARRAY structure. Skips empty lines and everything
 *   after character '#', which can be used for comments.
 */
static C_ARRAY *readLine(FILE *fin) {
	char c;
	C_ARRAY *in_arr;
	in_arr = allocCA(512);
	STRING token = alloc_string(16);

	/* Skip comments and empty lines */
	c = getc(fin);
	while ((c == '#' || c == ' ' || c == '\t' || c == '\n') && c != EOF) {
		if (c == '#') {
			while ((c = getc(fin)) != '\n' && c != EOF);
		}
		c = getc(fin);
	}

	/* Read one line of input file */
	while (c != '\n' && c != EOF && c != '#') {
		init_string(&token, token.max, 0);
		/* Skip empty characters at the begining */
		while (c == ' ' || c == '\t') {
			c = getc(fin);
		}
		/* Read one whole token, i.e. real number (double) */
		while (c != ' ' && c != '\n' && c != '\t') {
			//token->text[token->len++] = c;
			append(&token, c);
			c = getc(fin);
		}
		/* Add new token to "in_arr" */
		if (token.len != 0) {
			double din = atof(token.text);
			/* We need to allocate space for few more numbers */
			if (in_arr->max - in_arr->len <= 20) {
				reallocCA(in_arr, get_pow(in_arr->len + 512, 2));
			}
			in_arr->c[in_arr->len].re = din;
			in_arr->len++;
		}

		if (c == '\n') break;
	}
	free_string(&token);

	return in_arr;
}

/*
 *  Read data from input file and store them into structure C_ARRS.
 *   Each input sample is on its line.
 */
static void readInput(C_ARRS *inputs, char *file_name) {
	FILE *fin;

	if ((fin = fopen(file_name, "r")) == NULL) {
		perror("fopen");
		exit (ERROR_EXIT_CODE);
	}

	do {
		/* Check if we need to allocate more space for input values */
		if (inputs->max - inputs->len <= 1) {
			reallocCAS(inputs, inputs->max + 8);
		}
		inputs->carrs[inputs->len] = readLine(fin);
	} while(inputs->carrs[inputs->len++]->len > 0);
	/* We do not need the last one, it's empty */
	freeCA(inputs->carrs[--inputs->len]);

	if (fclose(fin) == EOF) {
		perror("fclose");
		exit (ERROR_EXIT_CODE);
	}
}

/*
 *  Writes array of complex numbers to specified file in Octave/Matlab format.
 */
static void writeOutput(char *file_name, C_ARRAY *ca) {
	FILE *fout;

	if ((fout = fopen(file_name, "w")) == NULL) {
		perror("fopen");
		exit (ERROR_EXIT_CODE);
	}

	/* Write Octave/Matlab file head */
	fprintf(fout, "# Created by %s, @timestamp <kappa@kappa>\n", program_name);
	fprintf(fout, "# name:\tcompl\n");
	fprintf(fout, "# type:\tcomplex matrix\n");
	fprintf(fout, "# rows:\t1\n");
	fprintf(fout, "# columns:\t%d\n", ca->len);

	/* Write out the data */
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
 *  Structure used to save all modification that will
 *   be applied in linked list.
 */
struct b_modif {
	/* C_ARRAY *sample, int srate, double gain [-24,+24] */
	void (*modif_f)(C_ARRAY *, struct band *, int, double);
	/* Which band will be modified */
	int band_id;
	/* What will be the gain passed into the modification function */
	double gain;
	/* Next operation in this linked list */
	struct b_modif *next;
};

/*
 *  Add new modification to the linked list structure b_modif.
 */
struct b_modif *addModif(struct b_modif *head, struct octave *oct, char func, int band_id, double gain) {
	printf("New modifier: func=%c band_id=%d gain=%.2fdB\n", func, band_id, gain);
	struct b_modif *nbm;
	if ((nbm = (struct b_modif *) malloc(sizeof(struct b_modif))) == NULL) {
		perror("malloc");
	}

	/* Check, if given band_id is really part of given Octave */
	if (band_id < 1 || band_id > oct->len) {
		fprintf(stderr, "Band ID is out of range [1; %d]\n", oct->len);
		usage();
	}
	nbm->band_id = band_id;
	/* Check range of given gain, allowed are only values from range [-24; 24] */
	if (gain < -24.0 || gain > 24.0) {
		fprintf(stderr, "Gain is out of range [-24; 24]dB\n");
		usage();
	}
	nbm->gain = gain;
	nbm->next = head;

	/* Decide which function will be used to modificate this band */
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
			fprintf(stderr, "Unknown modification function\n");
			usage();
			break;
	}

	return nbm;
}

/*
 *  Parse option inputs and appropriately initialize b_modif as a linked list
 *   of these modifications.
 *   
 *  We assume the input is in correct format, therefore only the last two parameters
 *   are being checked, bad function is recognized when we are adding new 
 *   modification to the linked list.
 *
 *  Element of the string "bands_in" should have following format:
 *   [band(number)][function(character)][sign(character)][gain(number)]
 *   every element should be separated by comma.
 */
struct b_modif *initModifs(struct b_modif *head, struct octave *oct, char *bands_in) {
	int  i=0;     /* Defines position in bands_in string */
	char akt;     /* Currently proccesed character */
	int  band_id; /* "band_id" value, that was readed from current element */
	char func;    /* Function defining character, readed from current element */
	int  gain;    /* Gain value from current element */
	STRING token = alloc_string(20);
	while ((akt = bands_in[i++]) != '\0' && i < strlen(bands_in)) {
		/* First we read band_id, which is integer */
		init_string(&token, 20, 0);
		while (akt >= '0' && akt <= '9') {
			append(&token, akt);
			akt = bands_in[i++];
		}
		band_id = atoi(token.text);

		/* Now read character defining the function to be used */
		func = akt;
		akt = bands_in[i++];

		/* 
		 * Now read the gain (integer number with sign + or -)
		 * No sign is also correct, its meaning is + sign.
		 */
		init_string(&token, 20, 0);
		if (akt == '+' || akt == '-' || (akt >= '0' && akt <= '9')) {
			append(&token, akt);
			akt = bands_in[i++];
		} else {
			/* The only thing we check for (might be unclear for users) */
			fprintf(stderr, "Incorrect sign before gain number\n");
			usage();
		}
		/* Read the rest of the gain number (if exists) */
		while (akt >= '0' && akt <= '9') {
			append(&token, akt);
			akt = bands_in[i++];
		}
		gain = atoi(token.text);

		head = addModif(head, oct, func, band_id, gain);
	}
	free_string(&token);

	return head;
}

/*
 *  Execute all of the modifications in the b_modif linked list on "ca" array.
 */
void processModifs(struct b_modif *head, C_ARRAY *ca, struct octave *oct, int srate) {
	struct b_modif *actb;
	actb = head;
	/* Go through the linked list and apply each modification */
	while (actb != NULL) {
		struct band *bnd = getBand(oct, actb->band_id);
		log_out(71, "Processing modification of %d. band with gain %.2f\n", actb->band_id, actb->gain);
		actb->modif_f(ca, bnd, srate, actb->gain);
		actb = actb->next;
	}
	log_out(71, "\n");
}

/*
 *  Free allocated space on heap by b_modif structure
 */
void freeModifs(struct b_modif *head) {
	struct b_modif *prev, *pom;
	pom = head;
	while (pom != NULL) {
		prev = pom;
		pom = pom->next;
		free(prev);
	}
}

int main(int argc, char **argv) {
	program_name = basename(argv[0]);

	C_ARRS *ins;
	ins = allocCAS(8);

	int opt;
	int f_flag=0;	/* Read input from file in_file */
	int w_flag=0;	/* Treat input as file in WAV format */
	int o_flag=0;   /* Write output to file out_file */
	int r_flag=0;   /* Set Octave fraction, default is Octave [1/1] */
	int k_flag=0;   /* Settings of virtual knots */
	int r_value=1;  /* Fraction denominator value, default is 1 */
	char *k_value;  /* Settings of virtual knots */
	char *in_file;  /* Name of input file (if f_flag==1) */
	char *out_file; /* Name of output file (if o_flag==1) */

	/* Read and process all options given to this program */
	while ((opt = getopt(argc, argv, "f:wd:o:r:k:")) != -1) {
		switch(opt) {
			case 'f':
				if (f_flag != 0) {
					fprintf(stderr, "Only one input file is required\n");
					usage();
				}
				/* Set "in_file" name to the value of the next argument */
				f_flag = 1;
				in_file = optarg;
				break;
			case 'w':
				/* Read "in_file" as WAV sound file */
				w_flag = 1;
				break;
			case 'o':
				if (o_flag != 0) {
					fprintf(stderr, "Only one output file is required\n");
					usage();
				}
				/* Write output to out_file in WAV format */
				o_flag = 1;
				out_file = optarg;
				break;
			case 'd':
				/* Get debug level (integer value) */
				debug = atoi(optarg);
				printf("Changing debug level to %d\n", debug);
				break;
			case 'r':
				/* Set Octave fraction denominator */
				r_flag = 1;
				r_value = atoi(optarg);
				break;
			case 'k':
				if (k_flag != 0) {
					fprintf(stderr, "Only one set of virtual knots configuration is allowed\n");
					usage();
				}
				/* Prepare modification functions */
				k_flag = 1;
				k_value = optarg;
				break;
			case '?':
				usage();
				break;
		}
	}

	/* "in_file" is required argument */
	if (f_flag == 0) {
		fprintf(stderr, "Argument in_file is required\n");
		usage();
	}

	/*
	 *  Stores all information from given WAV file header
	 */
	ELEMENT *header;

	/* "w_flag" was not set, read "in_file" as raw input data (default) */
	if (w_flag == 0) {
		printf("Reading raw data from file \"%s\"...\n", in_file);
		readInput(ins, in_file);
	}
	/* "w_flag" was set, read in_file as WAV */
	else {
		printf("Reading wav input file from \"%s\"...\n", in_file);
		header = readWav(ins, in_file);
	}

	/*
	 * Stores information about selected Octave, which includes
	 *  number of all bands, selected fraction and bands itself
	 *   as Bands elements in linked list.
	 */
	struct octave *oct;

	/* Check if "r_value" is in correct range [1; 24] */
	if (r_flag != 0 && r_value > 0 && r_value < 25) {
		printf("Changing to Octave [1/%d] bands\n", r_flag);
	} else if (r_flag != 0) {
		fprintf(stderr, "Ignoring r flag.\n");
		r_value = 1;
	} 
	oct = initOctave(1000, r_value);

	/*
	 *  Stores all modifications given by virtual knobs parsed
	 *   from option "-k".
	 */
	struct b_modif *modifs_head;
	modifs_head = NULL;

	/* Parse input virtual knots configuration */
	if (k_flag != 0) {
		modifs_head = initModifs(modifs_head, oct, k_value);
	}

	
	printf("Got %d input samples\n", ins->len);

	gnuplot_ctrl * g;
	C_ARRAY *re, *ire;  /* For temporary storing FFT and IFFT results */
	C_ARRAY *win;       /* Window for WLEN samples, works as kind of buffer */
	win = allocCA(WLEN);
	C_ARRAY *wav_out;


	/*
	 *  MAIN LOOP
	 *  ---------
	 *  For every channel of given WAV file or every row of data
	 *   from raw data file
	 *   i)   separate file into windows,
	 *   ii)  apply FFT on each window,
	 *   iii) apply all modification selected by user,
	 *   iv)  transfer through IFFT each window back,
	 *   v)   connect all windows together into the result
	 */
	int i; /* Current sound track id */
	for (i=0; i < ins->len; i++) {
		int ilen = ins->carrs[i]->len;
		int ilen2 = get_pow(ilen, 2);
		int imax = ins->carrs[i]->max;
		int j; /* For iteration through all points in graph */
		double *x = allocDoubles(imax);
		double *y = allocDoubles(imax);

		printf("INPUT %d, #samples: %d length->^2: %d:\n", i+1, ilen, ilen2);

		g = gnuplot_init();
		gnuplot_cmd(g, "set terminal png");
		gnuplot_setstyle(g, "lines");
		gnuplot_cmd(g, "set output \"input_%d.png\"", i+1);
		memset(x, 0, imax*sizeof(double));
		memset(y, 0, imax*sizeof(double));
		for (j=0; j<ilen; j++) {
			x[j] = j;
			y[j] = ins->carrs[i]->c[j].re;
		}
		gnuplot_plot_xy(g, x, y, ilen, "Input");
		gnuplot_close(g);

		STRING fname = alloc_string(20);
		sprintf(fname.text, "input_%d.mat", i+1);
		writeOutput(fname.text, ins->carrs[i]);
		free_string(&fname);


		/*
		 *  Divide input samples into windows of specific length
		 */
		wav_out = allocCA(ilen);
		int win_num = (int) ceil(ilen/WLEN);
		log_out(45, "Total number of windows is %d\n", win_num);
		log_out(71, "\n");
		int w_i;
		for (w_i=0; w_i < win_num; w_i++) {
			log_out(55, "Processing %d. window:\n", w_i+1);
			copyCA(ins->carrs[i], w_i*WLEN, win, 0, WLEN);
			win->max = WLEN;
			win->len = MIN(WLEN, ilen - w_i*WLEN);

			/* Not actually used, window functions needs to handle overlapping */
			/* 
			 * hammingWindow(win, 0.53836, 0.46164);
			 * planckWindow(win, 0.1);
			 * tukeyWindow(win, 0.1);
			 */

			/* Transform sound to frequency domain */
			re = fft(win);

			/* Plot graph of decibel values of each frequency */
			g = gnuplot_init();
			gnuplot_cmd(g, "set terminal png");
			gnuplot_setstyle(g, "lines");
			gnuplot_cmd(g, "set output \"fft_window_%d.png\"", w_i+1);
			gnuplot_set_ylabel(g, "dBPS");
			gnuplot_set_xlabel(g, "frequency (Hz)");
			for (j=0; j < re->len; j++) {
				x[j] = j;
				y[j] = decibel(re->c[j]); 
			}
			gnuplot_plot_xy(g, x, y, re->len/2, "FT");
			gnuplot_close(g);

			/* Apply modifications */
			processModifs(modifs_head, re, oct, getSampleRate(header));
			/* Transform back to time domain */
			ire = ifft(re);
			copyCA(ire, 0, wav_out, w_i*WLEN, MIN(WLEN, ilen - w_i*WLEN));

			freeCA(ire); freeCA(re);
		}

		/* Write the result into WAV file */
		if (o_flag == 1) {
			g = gnuplot_init();
			gnuplot_cmd(g, "set terminal png");
			gnuplot_setstyle(g, "lines");
			gnuplot_cmd(g, "set output \"wav_out.png\"");
			for (j=0; j < wav_out->len; j++) {
				x[j] = j; y[j] = wav_out->c[j].re;
			}
			gnuplot_plot_xy(g, x, y, wav_out->len, "Invers");
			gnuplot_close(g);

			log_out(55, "Writing result to WAV file\n");
			C_ARRS *caso;
			caso = allocCAS(1);
			caso->carrs[caso->len++] = wav_out;
			writeWav(header, caso, out_file);

			free(caso->carrs);
			free(caso);
		}
		printf("\n\n");

		free(x); free(y);
		freeCA(wav_out);
	}
	freeModifs(modifs_head);
	if (w_flag != 0) {
		freeHeader(header);
	}
	freeOctave(oct);
	freeCA(win);
	freeCAS(ins);

	return (0);
}
