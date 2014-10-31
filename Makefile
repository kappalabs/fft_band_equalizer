.PHONY:	clean debug run

CC		= gcc
CFLAGS	= -Wall -c -g -m64 -O0
LDFLAGS	= -Wall
LDLIBS	= -lm
PROG	= main
OBJS	= main.o gnuplot_i.o fft.o complex.o string.o wave.o
DEPS	= $(OBJS:.o=.h)
GARBAGE = *.png *.mat gnuplot_tmpdatafile_*
RM		= rm -f


all:	$(PROG)

$(PROG):	$(OBJS)
	$(CC) $(LDFLAGS) -o $@ $^ $(LDLIBS)

%.o:	%.c $(DEPS)
	$(CC) $(CFLAGS) -o $@ $<

main.o:	gnuplot_i.o fft.o complex.o string.o	#? $(OBJS)


debug:	clean $(PROG)
#	valgrind 2>valgrind.log --track-origins=yes --leak-check=full --show-leak-kinds=all ./main -d 90 -f sample.in >main.log
	valgrind 2>valgrind.log --track-origins=yes --leak-check=full --show-leak-kinds=all ./main -f "dve-32.wav" -w >main.log

run:	clean $(PROG)
#	./main -d 80 -f sample.in >main.log
	./main -f "dve-32.wav" -w >main.log

clean:
	$(RM) $(GARBAGE) $(PROG) $(OBJS)
