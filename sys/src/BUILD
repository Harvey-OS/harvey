#!/bin/bash
set +x
# assumptions: 
# go 1.16 or newer?
# plan9ports/bin is LAST in your path
. buildnamespace
which mk
export FORCERCFORMK=1
export CGO_ENABLED=0
PATH=`pwd`/../../linux_amd64/bin:$PATH
export objtype=amd64
export cputype=unix
HARVEY=`pwd`/../..
# yes, plan 9 really needs this!
PATH=$PATH:.
# we clean things out b/c it really is no time penalty
# and we want to be sure it all gets built.
mk clean

time mk all
time mk install

# in case of emergency break glass
# /bin/bash
