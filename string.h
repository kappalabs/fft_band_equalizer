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
 *       Filename:  string.h
 *
 *    Description:  This module contains functions to simplify work with strings.
 *                  It defines new structure STRING containing some useful
 *                  information which string usually has.
 *
 *         Author:  Vojtech Vasek
 *
 * ==============================================================================
 */

#ifndef STRING_H_
#define STRING_H_

/*
 *  Structure for storing one string.
 */
typedef struct {
	char *text;        // text itself
	unsigned int len;  // length of the text in chars (not counting last zero byte)
	unsigned int max;  // # of allocated chars (not counting last zero byte)
} STRING;


extern void init_chars(char *, unsigned int length);
extern char *alloc_chars(unsigned int length);

extern void init_string(STRING *s, unsigned int length, unsigned int start);
extern STRING alloc_string(unsigned int length);
extern void realloc_string(STRING *s, unsigned int nlen);
extern void free_string(STRING *s);

extern void append(STRING *str, char c);

#endif
