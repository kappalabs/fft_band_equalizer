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
 *       Filename:  string.c
 *
 *    Description:  This module contains functions to simplify work with strings.
 *                  It defines new structure STRING containing some useful
 *                  information which string usually has.
 *
 *         Author:  Vojtech Vasek
 *
 * ==============================================================================
 */

#include <stdlib.h>
#include <stdio.h>
#include <errno.h>
#include <string.h>

#include "string.h"


/*
 *  FUNCTIONS FOR WORK WITH SIMPLE STRING REPRESENTATION
 */

/*
 *  Initialize allocated array of characters to zero bytes.
 */
void init_chars(char *chars, unsigned int length) {
	memset(chars, '\0', length);
}

/*
 *  Allocate array of exactly "length" number of characters and
 *   return pointer to it if allocation was succesfull, NULL
 *   otherwise.
 */
char *alloc_chars(unsigned int length) {
	char *s;

	if ((s = (char *) calloc(length, sizeof(char))) == NULL) {
		perror("calloc");
		return NULL;
	}

	return s;
}


/*
 *  FUNCTIONS FOR WORK WITH STRUCTURE STRING REPRESENTATION
 */

/*
 *  Initialize given STRING with zero byte from position "start" to
 *  byte on position "length".
 *  Also set length and maximum appropriately.
 */
void init_string(STRING *s, unsigned int length, unsigned int start) {
	s->len = start;
	s->max = length;

	/* Null out including the last "unvisible" byte */
	memset(s->text, '\0', s->max+1);
}

/*
 *  Allocate and initialize new STRING structure of the given length.
 *  Return prepared STRING if allocation was succesfull.
 */
STRING alloc_string(unsigned int length) {
	STRING s;

	/* Allocate length bytes + 1 for last null byte */
	if ((s.text = (char *) malloc(length+1)) == NULL) {
		perror("malloc");
	} else {
		init_string(&s, length, 0);
	}

	return s;
}

/*
 *  Reallocate given STRING to the new given length.
 *  Also initialize new bytes to zero.
 */
void realloc_string(STRING *s, unsigned int new_len) {
	int olen = s->max;

	if ((s->text = (char *) realloc(s->text, new_len + 1)) == NULL) {
		perror("realloc");
	} else {
		init_string(s, new_len, olen);
	}
}

/*
 *  Release allocated memory for given STRING structure and set pointers to NULL.
 */
void free_string(STRING *s) {
	free(s->text);
	s->text = NULL;
}


/*
 *  FUNCTIONS FOR USEFUL STRING OPERATIONS
 */

/*
 *  Appends one char to the end of given string,
 *   allocate more memory if needed.
 */
void append(STRING *str, char c) {
	/* Check if we need to allocate more memory */
	if (str->max - str->len < 1) {
		realloc_string(str, str->max+1);
	}

	str->text[str->len] = c;
	str->text[++str->len] = '\0';
}
