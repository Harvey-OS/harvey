#!/bin/bash
set +x
# assumptions: 
# go 1.16 or newer?
# plan9ports/bin is LAST in your path
. buildnamespace
which mk

# we clean things out b/c it really is no time penalty
# and we want to be sure it all gets built.
mk clean

time rc $HARVEY/build/scripts/linux-build

# in case of emergency break glass
/bin/bash
