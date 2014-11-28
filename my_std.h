
#ifndef MY_STD_H_
#define MY_STD_H_

#define MIN(a, b) (((a) < (b)) ? (a) : (b))
#define MAX(a, b) (((a) > (b)) ? (a) : (b))

#define ERROR_EXIT_CODE 2

extern int debug;

extern void log_out(int level, const char* text_form, ...);
extern int is_pow_of_2(int val);
extern int get_pow(int val, int base);

extern double *allocDoubles(unsigned int len);

#endif
