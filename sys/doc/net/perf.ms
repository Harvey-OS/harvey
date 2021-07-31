.NH
Performance
.PP
We've measured both latency and throughput between
to for a number of different paths.
Measurements were made on a two and four
CPU SGI Power Series processor.
The CPU's are 25 MHZ MIPS 3000's.
The latency is measured as the round trip time
for a byte sent from one process to another and
back again.
Throughput is measured using 16k writes froom
one process to another.
.TS
box, tab(:);
c s s
c | c | c
l | n | n.
Table 1 - Pipes
_
test:throughput:latency
:MBytes/sec:millisec
_
pipes:8.15:.255
_
IL/ether:1.02:1.42
_
URP/datakit:0.22:1.75
_
Cyclone::
.TE
