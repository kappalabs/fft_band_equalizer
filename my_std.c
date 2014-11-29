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
 *       Filename:  my_std.c
 *
 *    Description:  This module contains few useful function for general
 *                  usage, like logging out messages, safe allocation of
 *                  arrays and also two functions connected to computing
 *                  power of some number.
 *
 *         Author:  Vojtech Vasek
 *
 * ==============================================================================
 */

#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <stdarg.h>

#include "my_std.h"
#include "string.h"


/*
 *  GLOBAL VARIABLE
 *  Default value of debug level.
 */
int debug = 90;


/*
 *  Prints out to the standard output given message
 *   in printf-like format if "level" is higher than "debug".
 */
void log_out(int level, const char* text_form, ...) {
	// Print only logs above desired importance
	if (level > debug) {
		va_list vl;
		va_start(vl, text_form);
		vprintf(text_form, vl);
		va_end(vl);
	}
}

/*
 *  Returns 1 if given integer "val" is power of 2, 0 otherwise.
 */
int is_pow_of_2(int val) {
	return (val & (val-1)) == 0;
}

/*
 *  Returns nearest power of integer value "base", which is bigger
 *   or equal to given value "val".
 */
int get_pow(int val, int base) {
	// Initialization to the zero power of every integer
	int nearest = 1;
	while (nearest < val) {
		nearest *= base;
	}

	return nearest;
}

/*
 *  Allocate array of doubles with length of "len" elements.
 *  Initialize all of them to 0.
 */
double *allocDoubles(unsigned int len) {
	double *d;
	if ((d = (double *) calloc(len, sizeof(double))) == NULL) {
		perror("calloc");
		exit (ERROR_EXIT_CODE);
	}

	return d;
}
