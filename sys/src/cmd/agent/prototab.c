#include "agent.h"

extern Proto
	apop,
	netkey,
	plan9,
	p9sk1,
	p9sk2,
	raw,
	sshrsa;

Proto *prototab[] = {
	&apop,
	&netkey,
	&plan9,
	&p9sk1,
	&p9sk2,
	&raw,
	&sshrsa,
	0
};
