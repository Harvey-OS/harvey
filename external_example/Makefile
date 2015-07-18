CFLAGS=-nostdinc \
	-mcmodel=small \
	-O0 \
	-fplan9-extensions \
	-mno-red-zone \
	-ffreestanding \
	-fno-builtin \
	-trigraphs \
	-Wall \
	-Wno-missing-braces \
	-Wno-parentheses \
	-Wno-unknown-pragmas \
	-Wuninitialized \
	-Wmaybe-uninitialized \
	-I $(HARVEY)/$(ARCH)/include \
	-I$(HARVEY)/sys/include

LDFLAGS=-nostdlib \
	-e _main \
	$(HARVEY)/$(ARCH)/lib/libc.a

hi: hi.o
	ld -o hi hi.o $(LDFLAGS)
hi.o: hi.c
	cc $(CFLAGS) -c hi.c

