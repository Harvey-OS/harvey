/* This file is part of the UCB release of Plan 9. It is subject to the license
 * terms in the LICENSE file found in the top-level directory of this
 * distribution and at http://akaros.cs.berkeley.edu/files/Plan9License. No
 * part of the UCB release of Plan 9, including this file, may be copied,
 * modified, propagated, or distributed except according to the terms contained
 * in the LICENSE file. */

#include "u.h"
#include "../port/lib.h"
#include "mem.h"
#include "dat.h"
#include "fns.h"

#include "apic.h"
#include "acpi.h"

extern int mpisabusno;

int mpacpi(int ncleft)
{
	char *already;
	int np, bp;
	Apic *apic;
	Apicst *st;
	Madt *mt;

	/* If we don't have an mpisabusno yet, it's because the MP tables failed to
	 * parse.  So we'll just take the last one available.  I think we're
	 * supposed to parse the ACPI shit with the AML to figure out the buses and
	 * find a clear one, but fuck that.  Note this busno is just for our own
	 * RDT/Rbus bookkeeping. */
	if (mpisabusno == -1)
		mpisabusno = Nbus - 1;

	if (apics == nil)
		return ncleft;
	mt = apics->tbl;
	if (mt == nil)
		return ncleft;

	print("APIC lapic paddr %#.8llx, flags %#.8x\n",
		   mt->lapicpa, mt->pcat);
	np = 0;
	//print("apics->st %p\n", apics->st);
	for (int i = 0; i < apics->nchildren; i++) {
		st = apics->children[i]->tbl;
		already = "";
		switch (st->type) {
			case ASlapic:
				print("ASlapic %d\n", st->lapic.id);
				/* this table is supposed to have all of them if it exists */
				if (st->lapic.id > Napic)
					break;
				apic = xlapic + st->lapic.id;
				bp = (np++ == 0);
				if (apic->useable) {
					already = "(mp)";
				} else if (ncleft != 0) {
					ncleft--;
					apicinit(st->lapic.id, mt->lapicpa, bp);
				} else
					already = "(off)";

				print("apic proc %d/%d apicid %d %s\n", np - 1, apic->Lapic.machno,
					   st->lapic.id, already);
				break;
			case ASioapic:
				print("ASioapic %d\n", st->ioapic.id);
				if (st->ioapic.id > Napic){
					print("ASioapic: %d is > %d, ignoring\n", st->ioapic.id, Napic);
					break;
				}
				apic = xioapic + st->ioapic.id;
				if (apic->useable) {
					already = "(mp)";
					goto pr1;
				}
				ioapicinit(st->ioapic.id, st->ioapic.ibase, st->ioapic.addr);
pr1:
				apic->Ioapic.gsib = st->ioapic.ibase;
				print("ioapic %d ", st->ioapic.id);
				print("addr %p ibase %d %s\n", st->ioapic.addr, st->ioapic.ibase,
					   already);
				break;
		}
	}
	return ncleft;
}
