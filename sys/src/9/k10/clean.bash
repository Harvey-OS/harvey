#!/bin/bash
set -x
rm -f *.o *.root.c *.out errstr.h init.h amd64^l.h $BOOTLIB ../boot/*.o boot$CONF.c
