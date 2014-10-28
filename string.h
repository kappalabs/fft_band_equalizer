
#ifndef STRING_H_
#define STRING_H_

typedef struct {
	char *text;
	unsigned int len;
	unsigned int max;
} STRING;


extern void initStr(STRING *s, unsigned int length, unsigned int start);
extern STRING *allocStr(unsigned int length);
extern void reallocStr(STRING *s, unsigned int nlen);
extern void freeStr(STRING *s);

extern void initString(char *str, unsigned int length);
extern char *allocString(unsigned int length);

#endif
