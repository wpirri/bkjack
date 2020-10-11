#!/usr/bin/make -f

INCLUDE=
LIB=-lncurses
C++C=cc
OBJECTS=bkbanca.o bkjugador.o mesa.o sincro.o display.o carta.o
OUTPUT=bkjack
COMP_OPTIONS=-D N_DEBUG
LINK_OPTIONS=

.C.o:
	$(C++C) $(INCLUDE) $(COMP_OPTIONS) -c $*.C

all: blackjack

blackjack: $(OBJECTS) bkjack.o
	$(C++C) $(LIB) $(LINK_OPTIONS) -O bkjack.o $(OBJECTS) -o $(OUTPUT)

clean:
	rm -f $(OBJECTS) bkjack.o

