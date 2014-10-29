#include <stdlib.h>
#include <stdio.h>
#include <errno.h>

#include "string.h"


/*
 *	FUNCTIONS FOR WORK WITH STRUCTURE STRING
 */

void initStr(STRING *s, unsigned int length, unsigned int start) {
	s->len = start;
	s->max = length-1;

	int i;
	for (i=start; i<length; i++) {
		s->text[i] = '\0';
	}
}

STRING *allocStr(unsigned int length) {
	STRING *s;

	if ((s = (STRING *) malloc(sizeof(STRING))) == NULL) {
		perror("malloc");
		return NULL;
	}

	if ((s->text = (char *) malloc(length * sizeof(char))) == NULL) {
		perror("malloc");
		return NULL;
	}

	initStr(s, length, 0);

	return s;
}

void reallocStr(STRING *s, unsigned int nlen) {
	int olen = s->max+1;

	if ((s->text = (char *) realloc(s->text, nlen * sizeof(char))) == NULL) {
		perror("malloc");
	}

	initStr(s, nlen, olen);
}

void freeStr(STRING *s) {
	free(s->text);
	s->text = NULL;
	free(s);
	s = NULL;
}

/*
 *	FUNCTIONS FOR WORK WITH SIMPLE STRING REPRESENTATION
 */
/*
 *	Initialize allocated string to zero bytes
 */
void initString(char *str, unsigned int length) {
	int i;
	for (i=0; i<length; i++) {
		str[i] = '\0';
	}
	str[length-1] = '\0';
}

char *allocString(unsigned int length) {
	char *s;

	if ((s = (char *) malloc(length * sizeof(char))) == NULL) {
		perror("malloc");
		return NULL;
	}

	initString(s, length);

	return s;
}
