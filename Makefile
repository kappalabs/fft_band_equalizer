.PHONY:	clean

CC	= gcc
CFLAGS	= -Wall -c -g -m64 -O0
LDFLAGS	= -Wall
LDLIBS	= -lm
PROG	= befft
OBJS	= befft.o gnuplot_i.o my_std.o equalizer.o complex.o string.o wave.o
DEPS	= $(OBJS:.o=.h)
GARBAGE = *.png *.mat gnuplot_tmpdatafile_*
RM	= rm -f


all:	$(PROG)

$(PROG):	$(OBJS)
	$(CC) $(LDFLAGS) -o $@ $^ $(LDLIBS)

%.o:	%.c $(DEPS)
	$(CC) $(CFLAGS) -o $@ $<

clean:
	$(RM) $(GARBAGE) $(PROG) $(OBJS)
