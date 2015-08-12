CC ?= gcc

CFLAGS = -Wall -Wextra -std=c99 -pedantic \
	-D_FORTIFY_SOURCE=2 -fstack-protector --param=ssp-buffer-size=4 \
	-O3 -pipe -s

DEBUG_CFLAGS = -Wall -Wextra -std=c99 -pedantic -msse4.2 \
	-Og -g -rdynamic -pg \
	-Wformat=2 -D_FORTIFY_SOURCE=2 --param=ssp-buffer-size=4 \
	-fstack-protector-all -Wstack-protector \
	-fsanitize=address -ftrapv \
	-fstrict-overflow -Wstrict-overflow=5 \
	-fstrict-aliasing -Wstrict-aliasing

LDFLAGS =

NAME = bombman

HDR = *.h
DST = $(NAME)
DST_DEBUG = $(NAME)_debug
OBJ =


SRC = $(NAME).c

all: $(DST)

parson.o: parson/parson.c parson/parson.h
	$(CC) -c $< $(CFLAGS) $(LDFLAGS)

nobrain: nobrain.c $(HDR)
	$(CC) -o $@ $< $(OBJ) $(CFLAGS) $(LDFLAGS)

$(DST): $(SRC) $(HDR) $(OBJ)
	$(CC) -o $@ $< $(OBJ) $(CFLAGS) $(LDFLAGS)

debug: $(SRC) $(HDR) $(OBJ)
	$(CC) -o $(DST_DEBUG) $< $(OBJ) $(DEBUG_CFLAGS) $(LDFLAGS)

clean:
	rm -f $(DST) $(DST_DEBUG) $(OBJ) nobrain
