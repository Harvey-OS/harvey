/*
 * This file is part of the UCB release of Plan 9. It is subject to the license
 * terms in the LICENSE file found in the top-level directory of this
 * distribution and at http://akaros.cs.berkeley.edu/files/Plan9License. No
 * part of the UCB release of Plan 9, including this file, may be copied,
 * modified, propagated, or distributed except according to the terms contained
 * in the LICENSE file.
 */

#include <u.h>
#include <libc.h>
#include <ip.h>

static uint8_t classmask[4][16] = {
    0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0x00, 0x00, 0x00,
    0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0x00, 0x00, 0x00,
    0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0x00, 0x00,
    0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0x00,
};

static uint8_t v6loopback[IPaddrlen] = {
    0, 0, 0, 0,
    0, 0, 0, 0,
    0, 0, 0, 0,
    0, 0, 0, 0x01};

static uint8_t v6linklocal[IPaddrlen] = {
    0xfe, 0x80, 0, 0,
    0, 0, 0, 0,
    0, 0, 0, 0,
    0, 0, 0, 0};
static uint8_t v6linklocalmask[IPaddrlen] = {
    0xff, 0xff, 0xff, 0xff,
    0xff, 0xff, 0xff, 0xff,
    0, 0, 0, 0,
    0, 0, 0, 0};
static int v6llpreflen = 8; /* link-local prefix length in bytes */

static uint8_t v6multicast[IPaddrlen] = {
    0xff, 0, 0, 0,
    0, 0, 0, 0,
    0, 0, 0, 0,
    0, 0, 0, 0};
static uint8_t v6multicastmask[IPaddrlen] = {
    0xff, 0, 0, 0,
    0, 0, 0, 0,
    0, 0, 0, 0,
    0, 0, 0, 0};
static int v6mcpreflen = 1; /* multicast prefix length */

static uint8_t v6solicitednode[IPaddrlen] = {
    0xff, 0x02, 0, 0,
    0, 0, 0, 0,
    0, 0, 0, 0x01,
    0xff, 0, 0, 0};
static uint8_t v6solicitednodemask[IPaddrlen] = {
    0xff, 0xff, 0xff, 0xff,
    0xff, 0xff, 0xff, 0xff,
    0xff, 0xff, 0xff, 0xff,
    0xff, 0x0, 0x0, 0x0};
static int v6snpreflen = 13;

uint8_t *
defmask(uint8_t *ip)
{
	if(isv4(ip))
		return classmask[ip[IPv4off] >> 6];
	else {
		if(ipcmp(ip, v6loopback) == 0)
			return IPallbits;
		else if(memcmp(ip, v6linklocal, v6llpreflen) == 0)
			return v6linklocalmask;
		else if(memcmp(ip, v6solicitednode, v6snpreflen) == 0)
			return v6solicitednodemask;
		else if(memcmp(ip, v6multicast, v6mcpreflen) == 0)
			return v6multicastmask;
		return IPallbits;
	}
}

void
maskip(uint8_t *from, uint8_t *mask, uint8_t *to)
{
	int i;

	for(i = 0; i < IPaddrlen; i++)
		to[i] = from[i] & mask[i];
}
