#!/bin/bash
# Kernel linking script
#
# This isn't the worlds most elegant link command.  link-kernel takes the obj
# output name, then the linker script, then everything else you'd dump on the
# ld command line, including linker options and objects to link together.
#
# We link extra times to resolve the reflected symbol table addresses.  After
# the first link, we can generate the reflected symbol table of the appropriate
# size.  Once we link it into the kernel, some addresses may have shifted, so
# we repeat the creation of the ksyms and relink.  For more info, check out
# Linux's kallsyms, their build script, and
# http://stackoverflow.com/questions/11254891/can-a-running-c-program-access-its-own-symbol-table

NM=nm
CC=gcc
LD=ld
OBJDUMP=objdump
OBJDIR=.
LDFLAGS="-z max-page-size=0x1000 -nostdlib -g"

gen_symtab_obj()
{
	$NM -n $KERNEL_OBJECT > $KSYM_MAP
	#awk 'BEGIN{ print "//#include <kdebug.h>";
	            #print "struct symtab_entry gbl_symtab[]={" }
	     #{ if(NF==3){print "{\"" $3 "\", 0x" $1 "},"}}
	     #END{print "{0,0} };"}' $KSYM_MAP > $KSYM_C
	echo "FIX THIS SOMEDAY"
	#$CC $NOSTDINC_FLAGS $AKAROSINCLUDE $CFLAGS_KERNEL -o $KSYM_O -c $KSYM_C
}

KERNEL_OBJECT=9k
LINKER_SCRIPT=kernel.ld
REMAINING_ARGS=$@

KSYM_MAP=$OBJDIR/ksyms.map
KSYM_C=$OBJDIR/ksyms-refl.c
KSYM_O=$OBJDIR/ksyms-refl.o

# Use "make V=1" to debug this script (from Linux)
case "${KBUILD_VERBOSE}" in
*1*)
	set -x
	;;
esac

# Generates the first version of $KERNEL_OBJECT
echo $LD $LDFLAGS -T $LINKER_SCRIPT -o $KERNEL_OBJECT $REMAINING_ARGS 
$LD $LDFLAGS -T $LINKER_SCRIPT -o $KERNEL_OBJECT $REMAINING_ARGS 

# Generates a C and obj file with a table of the correct size, with relocs
echo FIX ME gen_symtab_obj

# Links the syms with the kernel and inserts the glb_symtab in the kernel.
echo $LD $LDFLAGS -T $LINKER_SCRIPT -o $KERNEL_OBJECT $REMAINING_ARGS # $KSYM_O 
$LD $LDFLAGS -T $LINKER_SCRIPT -o $KERNEL_OBJECT $REMAINING_ARGS # $KSYM_O 

# Need to recheck/compute the symbols (table size won't change)
# gen_symtab_obj

# Final link
# $LD -T $LINKER_SCRIPT -o $KERNEL_OBJECT $REMAINING_ARGS # $KSYM_O

# And objdump for debugging
$OBJDUMP -S $KERNEL_OBJECT > $KERNEL_OBJECT.asm


objcopy -I elf64-x86-64 -O elf32-i386 9k 9k.32bit
