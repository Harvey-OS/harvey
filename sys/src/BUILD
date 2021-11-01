#!/bin/bash
set +x
. buildnamespace
which mk
export FORCERCFORMK=1
mk clean
mk libs
# yes, plan 9 really needs this!
PATH=$PATH:.
/usr/bin/strace -o /tmp/shit -f -s 1024 -v mk all

