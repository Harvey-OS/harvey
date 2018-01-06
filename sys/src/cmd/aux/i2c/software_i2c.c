/*
 * This file is part of the coreboot project.
 *
 * Copyright (C) 2014 Google, Inc.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; version 2 of the License.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 */

#include <u.h>
#include <libc.h>
#include "dat.h"
#include "fns.h"
#include "i2c.h"

/*
 * The implementation is based on Wikipedia.
 */

#define DEBUG 0		/* Set to 1 for per-byte output */

#define DELAY_US 5	/* Default setup delay: 4us (+1 for timer inaccuracy) */
#define TIMEOUT_US 50000 /* Maximum clock stretching time we want to allow */

extern int porttrace;
#define spew(...) do { if (porttrace) fprint(2, ##__VA_ARGS__); } while (0)

struct software_i2c_ops *software_i2c[SOFTWARE_I2C_MAX_BUS];

/*
 * Waits until either timeout_us have passed or (iff for_scl is set) until SCL
 * goes high. Will report random line changes during the wait and return SCL.
 */
static int __wait(unsigned bus, int timeout_us, int for_scl)
{
	int sda = software_i2c[bus]->get_sda(bus);
	int scl = software_i2c[bus]->get_scl(bus);
	uint64_t start, end;

	start = nsec();
	end = start + timeout_us * 1000;
	
	do {
		int old_sda = sda;
		int old_scl = scl;

		if (old_sda != (sda = software_i2c[bus]->get_sda(bus)))
			spew("[SDA transitioned to %d after %dus] ", sda, (nsec()-start)/1000);
		if (old_scl != (scl = software_i2c[bus]->get_scl(bus)))
			spew("[SCL transitioned to %d after %dus] ", scl, (nsec()-start)/1000);
	} while ((nsec() < end) && (!for_scl || !scl));

	return scl;
}

/* Waits the default DELAY_US to allow line state to stabilize. */
static void i2cwait(unsigned bus)
{
	uint64_t end = nsec() + DELAY_US * 1000;
	while (nsec() < end)
		;
}

/* Waits until SCL goes high. Prints a contextual error message on timeout. */
static int wait_for_scl(unsigned bus, const char *error_context)
{
	if (!__wait(bus, TIMEOUT_US, 1)) {
		fprint(2, "software_i2c(%d): ERROR: Clock stretching "
				 "timeout %s!\n", bus, error_context);
		return -1;
	}

	return 0;
}

static int start_cond(unsigned bus)
{
	spew("software_i2c(%d): Sending start condition... ", bus);

	/* SDA might not yet be high if repeated start. */
	software_i2c[bus]->set_sda(bus, 1);
	i2cwait(bus);

	/* Might need to wait for clock stretching if repeated start. */
	software_i2c[bus]->set_scl(bus, 1);
	if (wait_for_scl(bus, "before start condition"))
		return -1;
	i2cwait(bus);	/* Repeated start setup time, minimum 4.7us */

	if (!software_i2c[bus]->get_sda(bus)) {
		fprint(2, "software_i2c(%d): Arbitration lost trying "
			"to send start condition!\n", bus);
		return -1;
	}

	/* SCL is high, transition SDA low as first part of start condition. */
	software_i2c[bus]->set_sda(bus, 0);
	i2cwait(bus);
	assert(software_i2c[bus]->get_scl(bus));

	/* Pull SCL low to finish start condition (next pulse will be data). */
	software_i2c[bus]->set_scl(bus, 0);

	spew("Start condition transmitted!\n");
	return 0;
}

static int stop_cond(unsigned bus)
{
	spew("software_i2c(%d): Sending stop condition... ", bus);

	/* SDA is unknown, set it to low. SCL must be low. */
	software_i2c[bus]->set_sda(bus, 0);
	i2cwait(bus);

	/* Clock stretching */
	assert(!software_i2c[bus]->get_scl(bus));
	software_i2c[bus]->set_scl(bus, 1);
	if (wait_for_scl(bus, "before stop condition"))
		return -1;
	i2cwait(bus);	/* Stop bit setup time, minimum 4us */

	/* SCL is high, transition SDA high to signal stop condition. */
	software_i2c[bus]->set_sda(bus, 1);
	i2cwait(bus);
	if (!software_i2c[bus]->get_sda(bus)) {
		fprint(2, "software_i2c(%d): WARNING: SDA low after "
			"stop condition... access by another master or line "
			"stuck from faulty slave?\n", bus);
		/* Could theoretically happen with multi-master, so no -1. */
	}

	spew("Stop condition transmitted\n");
	return 0;
}

static int out_bit(unsigned bus, int bit)
{
	spew("software_i2c(%d): Sending a %d bit... ", bus, bit);

	software_i2c[bus]->set_sda(bus, bit);
	i2cwait(bus);

	if (bit && !software_i2c[bus]->get_sda(bus)) {
		fprint(2,  "software_i2c(%d): ERROR: SDA wedged low "
			"by slave before clock pulse on transmit!\n", bus);
		return -1;
	}

	/* Clock stretching */
	assert(!software_i2c[bus]->get_scl(bus));
	software_i2c[bus]->set_scl(bus, 1);
	if (wait_for_scl(bus, "on transmit"))
		return -1;
	i2cwait(bus);

	if (bit && !software_i2c[bus]->get_sda(bus)) {
		fprint(2,  "software_i2c(%d): ERROR: SDA wedged low "
			"by slave after clock pulse on transmit!\n", bus);
		return -1;
	}

	assert(software_i2c[bus]->get_scl(bus));
	software_i2c[bus]->set_scl(bus, 0);

	spew("%d bit sent!\n", bit);
	return 0;
}

static int in_bit(unsigned bus)
{
	int bit;

	spew("software_i2c(%d): Receiving a bit... ", bus);

	/* Let the slave drive data */
	software_i2c[bus]->set_sda(bus, 1);
	i2cwait(bus);

	/* Clock stretching */
	assert(!software_i2c[bus]->get_scl(bus));
	software_i2c[bus]->set_scl(bus, 1);
	if (wait_for_scl(bus, "on receive"))
		return -1;

	/* SCL is high, now data is valid */
	bit = software_i2c[bus]->get_sda(bus);
	i2cwait(bus);
	assert(software_i2c[bus]->get_scl(bus));
	software_i2c[bus]->set_scl(bus, 0);

	spew("Received a %d!\n", bit);

	return bit;
}

/* Write a byte to I2C bus. Return 0 if ack by the slave. */
static int out_byte(unsigned bus, uint8_t byte)
{
	unsigned bit;
	int nack;

	for (bit = 0; bit < 8; bit++)
		if (out_bit(bus, (byte >> (7 - bit)) & 0x1) < 0)
			return -1;

	nack = in_bit(bus);

	if (porttrace && nack >= 0)
		fprint(2, "software_i2c(%d): wrote byte 0x%02x, "
			"received %s\n", bus, byte, nack ? "NAK" : "ACK");

	return nack;
}

static int in_byte(unsigned bus, int ack)
{
	uint8_t byte = 0;
	int i;
	for (i = 0; i < 8; ++i) {
		int bit = in_bit(bus);
		if (bit < 0)
			return -1;
		byte = (byte << 1) | bit;
	}

	if (out_bit(bus, !ack) < 0)
		return -1;

	if (porttrace)
		fprint(2, "software_i2c(%d): read byte 0x%02x, "
			"sent %s\n", bus, byte, ack ? "ACK" : "NAK");

	return byte;
}

int software_i2c_transfer(unsigned bus, struct i2c_seg *segments, int count)
{
	int i;
	struct i2c_seg *seg;

	for (seg = segments; seg - segments < count; seg++) {
		if (start_cond(bus) < 0)
			return -1;
		if (out_byte(bus, seg->chip << 1 | !!seg->read) < 0)
			return -1;
		for (i = 0; i < seg->len; i++) {
			int ret;
			if (seg->read) {
				ret = in_byte(bus, i < seg->len - 1);
				seg->buf[i] = (uint8_t)ret;
			} else {
				ret = out_byte(bus, seg->buf[i]);
			}
			if (ret < 0)
				return -1;
		}
	}
	if (stop_cond(bus) < 0)
		return -1;

	return 0;
}

void software_i2c_wedge_ack(unsigned bus, uint8_t chip)
{
	int i;

	/* Start a command to 'chip'... */
	start_cond(bus);

	/* Send the address bits but don't yet read the ACK. */
	chip <<= 1;
	for (i = 0; i < 8; ++i)
		out_bit(bus, (chip >> (7 - i)) & 0x1);

	/* Let the slave drive it's ACK but keep the clock high forever. */
	software_i2c[bus]->set_sda(bus, 1);
	i2cwait(bus);
	software_i2c[bus]->set_scl(bus, 1);
	wait_for_scl(bus, "on wedge_ack()");

	fprint(2, "software_i2c(%d): wedged address write on slave "
		"ACK. SDA %d, SCL %d\n", bus, software_i2c[bus]->get_sda(bus),
		software_i2c[bus]->get_scl(bus));
}

void software_i2c_wedge_read(unsigned bus, uint8_t chip, uint8_t reg, int bits)
{
	int i;

	/* Start a command to 'chip'... */
	start_cond(bus);
	out_byte(bus, chip << 1);
	/* ...for register 'reg'. */
	out_byte(bus, reg);

	/* Start a read command... */
	start_cond(bus);
	out_byte(bus, chip << 1 | 1);

	/* Read bit_count bits and stop */
	for (i = 0; i < bits; ++i)
		in_bit(bus);

	/* Let the slave drive SDA but keep the clock high forever. */
	software_i2c[bus]->set_sda(bus, 1);
	i2cwait(bus);
	software_i2c[bus]->set_scl(bus, 1);
	wait_for_scl(bus, "on wedge_read()");

	fprint(2, "software_i2c(%d): wedged data read after %d bits. "
		"SDA %d, SCL %d\n", bus, bits, software_i2c[bus]->get_sda(bus),
		software_i2c[bus]->get_scl(bus));
}

void software_i2c_wedge_write(unsigned bus, uint8_t chip, uint8_t reg, int bits)
{
	int i;

	/* Start a command to 'chip'... */
	start_cond(bus);
	out_byte(bus, chip << 1);

	/* Write bit_count register bits and stop */
	for (i = 0; i < bits; ++i)
		out_bit(bus, (reg >> (7 - i)) & 0x1);

	/* Pretend to write another 1 bit but keep the clock high forever. */
	software_i2c[bus]->set_sda(bus, 1);
	i2cwait(bus);
	software_i2c[bus]->set_scl(bus, 1);
	wait_for_scl(bus, "on wedge_write()");

	fprint(2, "software_i2c(%d): wedged data write after %d bits. "
		"SDA %d, SCL %d\n", bus, bits, software_i2c[bus]->get_sda(bus),
		software_i2c[bus]->get_scl(bus));
}
