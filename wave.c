#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <errno.h>
#include <fcntl.h>
#include <string.h>
#include <math.h>

#include "main.h"
#include "wave.h"
#include "complex.h"

#define HEADER_SIZE 13
/*
 *	Standard WAVE format header
 */
struct element header[HEADER_SIZE] = {
				{1, 0, 4, "ChunkID", 0},       // 0; RIFF or RIFX, RIFX means that default is big endian
				{0, 4, 4, "ChunkSize", 0},     // 1;
				{1, 8, 4, "Format", 0},        // 2; should be text "WAVE" = 57 41 56 45
				{1, 12, 4, "Subchunk1ID", 0},  // 3; should be text "fmt" = 66 6D 74 20
				{0, 16, 4, "Subchunk1Size", 0},// 4; 16 if PCM is used
				{0, 20, 2, "AudioFormat", 0},  // 5; 1 ~ PCM, otherwise, there is some kind of compression in data chunk
				{0, 22, 2, "NumChannels", 0},  // 6; 1 ~ mono, 2 ~ stereo
				{0, 24, 4, "SampleRate", 0},   // 7;
				{0, 28, 4, "ByteRate", 0},     // 8;
				{0, 32, 2, "BlockAlign", 0},   // 9; # of bytes for one sample
				{0, 34, 2, "BitsPerSample", 0},// 10;
				{1, 36, 4, "Subchunk2ID", 0},  // 11; should be text "data"
				{0, 40, 4, "Subchunk2Size", 0} // 12; # of bytes in the rest of the file
};

/*
 *	Returns converted input int array in specified endian format
 *	 into simple integer value
 */
int toInt(char *data, int size, short int endian) {
	int ret = 0;
	int i;
	for (i=0; i<size; i++) {
		// big endian
		if (endian == 1) {
		  ret |= data[i] & 0x00FF;
			if (i != size-1) {
				ret <<= 8;
			}
		}
		// little endian
	 	else {
			ret |= ((data[i] & 0x00FF) << (i*8));
		}
	}

	return ret;
}

int elementToInt(struct element h) {
	return toInt(h.data, h.size, h.endian);
}

/*
 *	Conversion between array of short integers and double.
 *	Double is made by normalization into [-1; 1] interval.
 *
 *	short int *data  - input array with data to convert
 *	int size         - length of array *data
 *	short int endian - 1~big endian, 0~little endian
 */
double toDouble(char *data, int size, short endian) {
	int ret = 0;
	int i;
	for (i=0; i<size; i++) {
		// big endian
		if (endian == 1) {
		  ret |= (unsigned char) data[i];
			if (i != size-1) {
				ret <<= 8;
			}
		}
		// little endian
	 	else {
			ret |= ((unsigned char) data[i]) << (i*8);
		}
	}

	return ret/pow(2, size*8 - 1) - 1.0;
}

/*
 *	Returns number of channels as it is in given header
 */
int getNumChannels(struct element *h) {
	return elementToInt(h[6]);
}

/*
 *	Returns rate of samples in Hz as it is in given header
 */
int getSampleRate(struct element *h) {
	return elementToInt(h[7]);
}

/*
 *	Returns number of bytes in the data block
 */
int getSubchunk2Size(struct element *h) {
	return elementToInt(h[12]);
}

/*
 *	Compares data in element.data with given integer value
 *	 and returns 0 if values are equal, -1 otherwise.
 */
int elementComp(struct element el, unsigned int val) {
	if (elementToInt(el) == val) {
		return 0;
	}
	return 1;
}

/*
 *	Retrieve data from WAV file and save them in element structure header
 */
int initHeader(int fd) {
	char *buf;
	int i;
	for (i=0; i<HEADER_SIZE; i++) {
		int size = header[i].size;
 		if ((buf = (char *) calloc((size+1), sizeof(char))) == NULL) {
			perror("calloc");
			return -1;
		}

		// move to specific offset where data should be stored
		lseek(fd, header[i].offset, SEEK_SET);
		if (read(fd, buf, size) != size) {
			fprintf(stderr, "Header was set incorrectly.");
			exit (2);
		}
		
		header[i].data = buf;
		printf("%s = ", header[i].name);
		//TODO: if 4 prvni byty RIFX namisto RIFF, uzivej pouze big-endian
		if (header[i].endian == 1) {
			printf("%s\n", header[i].data);
		} else {
			printf("%d\n", elementToInt(header[i]));
		}
	}

	// simple check, if file has WAVE header
	if (elementComp(header[2], 0x57415645) != 0) {
		return -1;
	}
	// compression is not supported
	if (elementToInt(header[5]) != 1) {
		return -1;
	}

	return 0;
}

void freeHeader(struct element *h) {
	int i;
	for (i=0; i<HEADER_SIZE; i++) {
		free(h[i].data);
		h[i].data = NULL;
	}
}

C_ARRAY *getChannel(int fd, short ch_id) {
	// array for input samples
	C_ARRAY *ca;
 	ca = allocCA(512);
	// # of readed bytes
	int r=0;
	// # of bits per sample
	int bps = elementToInt(header[10]);
	// # of bytes per sample
	int B_SIZE = bps/8;
	char *buf = (char *) calloc(B_SIZE, sizeof(char));
	// jump to offset where the data starts
	lseek(fd, 44+B_SIZE*ch_id, SEEK_SET);
	// # of channels
	int nch = elementToInt(header[6]);
	// # of total samples
	int tns = elementToInt(header[12]);

	while (read(fd, buf, B_SIZE) > 0 && r < tns) {
		// we need more memory for next samples/values
		if (ca->max - ca->len == 0) {
			reallocCA(ca, getPow(ca->len + 64, 2));
		}
		r++;

		//TODO: endianita podle RIF(F|X)?
		ca->c[ca->len++].re = toDouble(buf, B_SIZE, 0);
		if (r<=15) {
			printf("%d-th sample: %.5f\n", r, ca->c[ca->len-1].re);
		}
		lseek(fd, B_SIZE*(nch-1), SEEK_CUR);
	}

	free(buf);

	return ca;
}

struct element *readWav(C_ARRS *cas, char *fpath) {
	int fd;

	if ((fd = open(fpath, O_RDONLY)) < 0) {
		perror("open");
		return NULL;
	}

	if (initHeader(fd) != 0) {
		fprintf(stderr, "Unacceptable WAVE header\n");
		close(fd);
		return NULL;
	}

	int nch = elementToInt(header[6]);
	// allocate to the # of in channels
	if (cas->max - cas->len < nch) {
		reallocCAS(cas, cas->max + nch);
	}

	int i;
	for (i=0; i<nch; i++) {
		printf("Channel %d:\n", i+1);
		cas->carrs[cas->len++] = getChannel(fd, i);
	}

	close(fd);
	return header;
}

void writeHeader(int fd, struct element *h) {
	// header starts on the first bit of the file
	lseek(fd, 0, SEEK_SET);

	int i;
	for (i=0; i<HEADER_SIZE; i++) {
		if (write(fd, h[i].data, h[i].size) <= 0) {
			perror("write");
			return;
		}
	}
}

void denormalize(char *buf, double d, int size, int endian) {
	long long int pom = 1; // stores denormalized integer value
	long long int mask = 0xff; // mask for lowest one byte
	d += 1.0;	// return d from range [-1; 1] to non-negative value
	pom <<= size*8 - 1;
	pom *= d;
	int i;
	for (i=0; i<size; i++) {
		// little endian format
		buf[i] = (pom >> i*8) & mask;
	}
}

int writeChannel(struct element *h, C_ARRAY *ca, int fd, int ch_id) {
	int nch = elementToInt(h[6]);
	int bps = elementToInt(h[10]);
	int tot = elementToInt(h[12]);
	int B_SIZE = bps/8;	// # of bytes per sample
	int ctw = tot/(B_SIZE*nch);	// # of chunks (of size B_SIZE) to be written out

	// check if header was written out,
	//  otherwise there would be missing chunk in this file
	if (lseek(fd, 0, SEEK_CUR) < 44) {
		fprintf(stderr, "Header file was not written yet, can't write data\n");
		return -1;
	}
	char *buf; // buffer for the output bytes
	if ((buf = (char *) calloc(B_SIZE, sizeof(char))) == NULL) {
		perror("calloc");
		return -1;
	}

	// jump to the start of the data section
	lseek(fd, 44+B_SIZE*ch_id, SEEK_SET);
	int i;
	for (i=0; i<ctw; i++) {
		denormalize(buf, ca->c[i].re, B_SIZE, 0);
		if (write(fd, buf, B_SIZE) <= 0) {
			perror("write");
			return -1;
		}
		lseek(fd, B_SIZE*(nch-1), SEEK_CUR);
	}

	free(buf);

	return 0;
}

/*
 *	Writes array *cas into file using WAV format.
 *	Struct element *h must be already prepared.
 */
void writeWav(struct element *h, C_ARRS *cas, char *fpath) {
	int fd;
	int nch = getNumChannels(h);

	if (nch != cas->len) {
		fprintf(stderr, "Header file improperly set.\n");
		return;
	}

	if ((fd = open(fpath, O_WRONLY | O_CREAT | O_TRUNC, 0755)) < 0) {
		perror("open");
		return;
	}

	writeHeader(fd, h);

	int i;
	for (i=0; i<nch; i++) {
		printf("Writing out %d channel...\n", i+1);
		writeChannel(h, cas->carrs[i], fd, i);
	}

	close(fd);
}
