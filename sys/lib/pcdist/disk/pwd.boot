#!/bin/rc

syscall -o fd2path 0 buf 1024 < . >[2]/dev/null
