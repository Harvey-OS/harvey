# Description file for the Make command
# Makefile 4.1 81/02/28

P%.o :: %.c
	-ln $%.c P$%.c
	cc -c $(PCFLAGS) P$%.c
	rm P$%.c

P = ipr
T =
O = v
FILES = Makefile ident.c defs.h main.c doname.c misc.c files.c dosys.c gram.y
OBJECTS = ident.$O main.$O doname.$O dosys.$O gram.$O misc.$O files.$O
POBJECTS = Pident.$O Pmain.$O Pdoname.$O Pdosys.$O Pgram.$O Pmisc.$O Pfiles.$O
LIBES= 
LINT = lint -ps
CFLAGS =
PCFLAGS = -p -g

GCOSFILES = defs.h ident.c main.c doname.c misc.c gram.c gcos.c

all:	make

cmp:	make
	cmp make /bin/make
	rm -f *.$O gram.c make

cp:	make
	cp make /bin/make
	rm -f *.$O gram.c make


make:&  $(OBJECTS)
	$(CC) $(CFLAGS) -o make $(OBJECTS) $(LIBES)

profmake:& $(POBJECTS)
	$(CC) $(PCFLAGS) -o profmake $(POBJECTS)

$(OBJECTS):  defs.h
$(POBJECTS): defs.h

clean:
	-rm -f *.$O P*.c gram.c make $O.out core

install:	make
	strip make
	cp make $(DESTDIR)/bin/make

printall: # Print files off line.
	-pr $(FILES) | $P
	touch print

print:  $(FILES)	# print recently changed files
	-pr $? | $P
	touch print

test: 1zap 2zap
	diff 1zap 2zap
	rm -f 1zap 2zap

1zap:
	./make -dp | grep -v TIME >1zap

2zap:
	/bin/make -dp | grep -v TIME >2zap

time:  time1 time2 ;

time1 time2 :
	time ./make $T
	time /bin/make $T

lint :  dosys.c doname.c files.c main.c misc.c ident.c gram.c
	$(LINT) dosys.c doname.c files.c main.c misc.c ident.c gram.c
	rm -f gram.c

src:
	cp $(FILES) /usr/src/cmd/make

makearch:	$(FILES)
	ar uv makearch $?

v8:
	CFLAGS := $(CFLAGS) -DVERSION8
debug:
	CFLAGS := $(CFLAGS) -g

gcos: $(GCOSFILES)
	fsend -c -u sif $?
	touch gcos

%.$O:	%.c
	$(CC) -c $(CFLAGS) $*.c
