#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <stdarg.h>

#include "my_std.h"
#include "string.h"


/*
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
