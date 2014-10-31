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

/*
 *	standard WAVE format header
 */
struct element header[13] = {
				{1, 0, 4, "ChunkID", 0},			// 0
				{0, 4, 4, "ChunkSize", 0},		
				{1, 8, 4, "Format", 0},	
				{1, 12, 4, "Subchunk1ID", 0},	// 3
				{0, 16, 4, "Subchunk1Size", 0},
				{0, 20, 2, "AudioFormat", 0}, // pokud neni 1=PCM, jde o kompresi
				{0, 22, 2, "NumChannels", 0}, // 1 - mono, 2 - stereo
				{0, 24, 4, "SampleRate", 0},
				{0, 28, 4, "ByteRate", 0},
				{0, 32, 2, "BlockAlign", 0}, // # of bytes  one sample
				{0, 34, 2, "BitsPerSample", 0},
				{1, 36, 4, "Subchunk2ID", 0},
				{0, 40, 4, "Subchunk2Size", 0}
};



//void initBuf(unsigned int *buf, int len) {
//	int i;
//	for (i=0; i<len; i++) {
//		buf[i] = '\0';
//	}
//}

/*
 *	Returns converted input int array in specified endian format
 *	 to simple integer value
 */
int toInt(short int *data, int size, short int endian) {
	int ret = 0;
	int i;
	for (i=0; i<size; i++) {
		// big endian
		if (endian == 1) {
		  ret += (unsigned short int) data[i];
			ret <<= 8;
		}
		// little endian
	 	else {
			//printf("(%d += %d << %d)\n", ret, e.data[i], i*8);
			ret += (unsigned short int) (data[i] << (i*8));
		}
	}

	return ret;
}

int getElementIntValue(struct element h) {
	return toInt(h.data, h.size, h.endian);
}

/*
 *	Conversion between array of short integers and double.
 *	Double is made by normalization into [-1; 1] interval.
 *
 *	short int *data  - input array with data to convert
 *	int size         - length of array *data
 *	short int endian - 1~big endian, 0~little endian
 *	short int b_len  - length in bytes of this sound sample
 */
double toDouble(short int *data, int size, short int endian, short int b_len) {
	int ret = 0;
	int i;
	for (i=0; i<size; i++) {
		// big endian
		if (endian == 1) {
		  ret += (short int) data[i];
			ret <<= 8;
		}
		// little endian
	 	else {
			ret += ((short int) data[i]) << (i*8);
		}
	}

	return ret/pow(2, b_len*8);
}

int initHeader(int fd) {
	short int *buf;
	int i;
	for (i=0; i<13; i++) {
		int size = header[i].size;
 		if ((buf = (short int *) calloc(size, sizeof(short int))) == NULL) {
			perror("calloc");
			return -1;
		}

		// move to specific offset where data should be stored
		lseek(fd, header[i].offset, SEEK_SET);
		if (read(fd, buf, size) != size) {
			printf("Spatne nastavene hlavickove parametry, koncim");
			exit (2);
		}
		
		header[i].data = buf;
		printf("%s = ", header[i].name);
		//TODO: if 4 prvni byty RIFX namisto RIFF, uzivej pouze big-endian
		if (header[i].endian == 1) {
			printf("%s\n", (char *)header[i].data);
		} else {
			printf("%d\n", getElementIntValue(header[i]));
		}
	}
	//TODO: overeni, zdali jde o realnou hlavicku WAV souboru

	return 0;
}

void freeHeader(struct element *h) {
	int i;
	for (i=0; i<13; i++) {
		free(h[i].data);
		h[i].data = NULL;
	}
}

C_ARRAY *getChannel(int fd, short int ch_id) {
	// array for input samples
	C_ARRAY *ca;
 	ca = allocCA(512);
	// # of readed bytes
	int r=0;
	// # of bits per sample
	int bps = getElementIntValue(header[10]);
	// # of bytes per sample
	int B_SIZE = bps/8;
	short int *buf = (short int *) calloc(B_SIZE, sizeof(short int));
	// jump to offset where the data starts
	lseek(fd, 44+B_SIZE*ch_id, SEEK_SET);
	// # of channels
	int nch = getElementIntValue(header[6]);
	// # of total samples
	int tns = getElementIntValue(header[12]);

	int r_in; // # of bytes readed to buffer
	while ((r_in = read(fd, buf, B_SIZE)) > 0 && r < tns) {
		// we need more memory for next samples/values
		if (ca->max - ca->len == 0) {
			reallocCA(ca, getPow(ca->len + 64, 2));
		}
		r++;

		//TODO: endianita podle RIF(F|X)?
		ca->c[ca->len++].re = toDouble(buf, r_in, 0, B_SIZE);
		if (r<=15) {
			printf("%d-th sample: %.5f\n", r, ca->c[ca->len-1].re);
		}
		lseek(fd, B_SIZE*(nch-1), SEEK_CUR);
		//initBuf(buf, B_SIZE);
		//memset(buf, 0, B_SIZE);
	}

	free(buf);

	return ca;
}

void readWav(C_ARRS *cas, char *path) {
	int fd;
	//C_ARRS *cas;

	if ((fd = open(path, O_RDONLY)) < 0) {
		perror("open");
		return;
	}

	if (initHeader(fd) != 0) {
		perror("header");
		close(fd);
		return;
	}

	int nch = getElementIntValue(header[6]);
	// allocate to the # of in channels
	if (cas->max - cas->len < nch) {
		reallocCAS(cas, cas->max + nch);
	}

	int i;
	for (i=0; i<nch; i++) {
		printf("Channel %d:\n", i+1);
		cas->carrs[cas->len++] = getChannel(fd, i);
	}

	freeHeader(header);

	close(fd);
}
