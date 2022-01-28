#!/bin/bash
set +x
. buildnamespace
which mk
export FORCERCFORMK=1
mk clean
mk libs
# yes, plan 9 really needs this!
PATH=$PATH:.
mk all

time rc $HARVEY/build/scripts/linux-build

# in case of emergency break glass
/bin/bash
