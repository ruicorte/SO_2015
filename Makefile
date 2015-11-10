#####################################################################################################
# sistemas operativos - 2015/2016 - simulacao de discoteca
#
# Docente: Eduardo Marques
#
# Alunos:
# David Ricardo Cândido nº 2032703
# Luís Bouzas Prego 	nº 2081815
#
# Ficheiro: Makefile
#####################################################################################################

all: sim mon

mon: mon.o
	gcc -Wall -g mon.o -o mon -lpthread
mon.o: mon.c util.h unix.h
	gcc -c mon.c
sim: sim.o
	gcc -Wall -g sim.o -o sim -lpthread
sim.o: sim.c util.h unix.h
	gcc -c sim.c
clean:
	rm *.o
