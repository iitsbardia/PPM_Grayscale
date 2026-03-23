.PHONY: all clean

all: ppm_grayscale

ppm_grayscale: main.o worker.o ppm.o
	gcc -Wall -g -o $@ main.o worker.o ppm.o

%.o: %.c
	gcc -Wall -g -c -o $@ $<

clean:
	rm -f *.o ppm_grayscale out.ppm