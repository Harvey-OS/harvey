/*
 * This file is part of the coreboot project.
 *
 * Copyright (C) 2009 coresystems GmbH
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; version 2 of the License.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301 USA
 */
#include <u.h>

void outb(uint16_t port, uint8_t value)
{
	__asm__ __volatile__ ("outb %b0, %w1" : : "a" (value), "Nd" (port));
}

void outs(uint16_t port, uint16_t value)
{
	__asm__ __volatile__ ("outw %w0, %w1" : : "a" (value), "Nd" (port));
}

void outl(uint16_t port, uint32_t value)
{
	__asm__ __volatile__ ("outl %0, %w1" : : "a" (value), "Nd" (port));
}

uint8_t inb(uint16_t port)
{
	uint8_t value;
	__asm__ __volatile__ ("inb %w1, %b0" : "=a"(value) : "Nd" (port));
	return value;
}

uint16_t ins(uint16_t port)
{
	uint16_t value;
	__asm__ __volatile__ ("inw %w1, %w0" : "=a"(value) : "Nd" (port));
	return value;
}

uint32_t inl(uint16_t port)
{
	uint32_t value;
	__asm__ __volatile__ ("inl %w1, %0" : "=a"(value) : "Nd" (port));
	return value;
}

void outsb(uint16_t port, const void *addr, unsigned long count)
{
	__asm__ __volatile__ (
		"cld ; rep ; outsb "
		: "=S" (addr), "=c" (count)
		: "d"(port), "0"(addr), "1" (count)
		);
}

void outsw(uint16_t port, const void *addr, unsigned long count)
{
	__asm__ __volatile__ (
		"cld ; rep ; outsw "
		: "=S" (addr), "=c" (count)
		: "d"(port), "0"(addr), "1" (count)
		);
}

void outsl(uint16_t port, const void *addr, unsigned long count)
{
	__asm__ __volatile__ (
		"cld ; rep ; outsl "
		: "=S" (addr), "=c" (count)
		: "d"(port), "0"(addr), "1" (count)
		);
}


void insb(uint16_t port, void *addr, unsigned long count)
{
	__asm__ __volatile__ (
		"cld ; rep ; insb "
		: "=D" (addr), "=c" (count)
		: "d"(port), "0"(addr), "1" (count)
		: "memory"
		);
}

void insw(uint16_t port, void *addr, unsigned long count)
{
	__asm__ __volatile__ (
		"cld ; rep ; insw "
		: "=D" (addr), "=c" (count)
		: "d"(port), "0"(addr), "1" (count)
		: "memory"
		);
}

void insl(uint16_t port, void *addr, unsigned long count)
{
	__asm__ __volatile__ (
		"cld ; rep ; insl "
		: "=D" (addr), "=c" (count)
		: "d"(port), "0"(addr), "1" (count)
		: "memory"
		);
}

uint8_t read8(const volatile void *addr)
{
	return *((volatile uint8_t *)(addr));
}

uint16_t read16(const volatile void *addr)
{
	return *((volatile uint16_t *)(addr));
}

uint32_t read32(const volatile void *addr)
{
	return *((volatile uint32_t *)(addr));
}

void write8(volatile void *addr, uint8_t value)
{
	*((volatile uint8_t *)addr) = value;
}

void write16(volatile void *addr, uint16_t value)
{
	*((volatile uint16_t *)addr) = value;
}

void write32(volatile void *addr, uint32_t value)
{
	*((volatile uint32_t *)addr) = value;
}

uint64_t rdmsr(uint32_t index)
{
	uint64_t result;
	uint32_t lo, hi;
	__asm__ __volatile__ (
		"rdmsr"
		: "=a" (lo), "=d" (hi)
		: "c" (index)
		);
	// make sure the shift does not lose the 32 bits.
	result = hi;
	result <<= 32;
	result |= lo;
}

void wrmsr(uint32_t index, uint64_t msr)
{
	uint32_t lo, hi;
	lo = (uint32_t) msr;
	hi = (uint32_t) (msr>>32);
	__asm__ __volatile__ (
		"wrmsr"
		: /* No outputs */
		: "c" (index), "a" (lo), "d" (hi)
		);
}

