#include "u.h"
#include "../port/lib.h"
#include "mem.h"
#include "dat.h"
#include "fns.h"
#include "io.h"
#include "mp.h"
#include "apic.h"
#include "acpi.h"

extern	Madt	*apics;

int
mpacpi(int ncleft)
{
	char *already;
	int np, bp;
	Apic *apic;
	Apicst *st;

	if (apics == nil)
		return ncleft;

	print("APIC lapic paddr %#.8llux, flags %#.8ux\n",
		apics->lapicpa, apics->pcat);
	np = 0;
	for(st = apics->st; st != nil; st = st->next){
		already = "";
		switch(st->type){
		case ASlapic:
			/* this table is supposed to have all of them if it exists */
			if(st->lapic.id > Napic)
				break;
			apic = xlapic + st->lapic.id;
			bp = (np++ == 0);
			if(apic->useable){
				already = "(mp)";
			}
			else if(ncleft != 0){
				ncleft--;
				apicinit(st->lapic.id, apics->lapicpa, bp);
			} else
				already = "(off)";

			print("apic proc %d/%d apicid %d %s\n", np-1, apic->machno, st->lapic.id, already);
			break;
		case ASioapic:
			if(st->ioapic.id > Napic)
				break;
			apic = xioapic + st->ioapic.id;
			if(apic->useable){
				apic->ibase = st->ioapic.ibase;	/* gnarly */
				already = "(mp)";
				goto pr1;
			}
			ioapicinit(st->ioapic.id, st->ioapic.ibase, st->ioapic.addr);
		pr1:
			print("ioapic %d ", st->ioapic.id);
			print("addr %p base %d %s\n", apic->paddr, apic->ibase, already);
			break;
		}
	}
	return ncleft;
}
