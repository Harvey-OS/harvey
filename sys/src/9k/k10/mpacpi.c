#include "u.h"
#include "../port/lib.h"
#include "mem.h"
#include "dat.h"
#include "fns.h"

#include "io.h"
#include "mp.h"
#include "apic.h"
#include "acpi.h"

extern	Madt*	apics;

void
mpacpi(void)
{
	char *already;
	int nprocid, bp;
	Apic *apic;
	Apicst *st;

	DBG("APIC mpacpi lapic addr %#p, flags %#ux\n",
		apics->lapicpa, apics->pcat);

	nprocid = 0;
	for(st = apics->st; st != nil; st = st->next){
		already = "";
		switch(st->type){
		case ASlapic:
			if(st->lapic.id > Napic)
				break;
			apic = &xapic[st->lapic.id];
			bp = (nprocid++ == 0);
			if(apic->useable)
				already = "(on)";
			else
				apicinit(st->lapic.id, apics->lapicpa, bp);
			USED(already);
			DBG("apic proc %d/%d apicid %d %s\n", nprocid-1,
				apic->machno, st->lapic.id, already);
			break;
		case ASioapic:
			if(st->ioapic.id > Napic)
				break;
			apic = &ioapic[st->ioapic.id];
			if(apic->useable){
				apic->gsib = st->ioapic.ibase;
				already = "(on)";
			}else
				ioapicinit(st->ioapic.id, st->ioapic.addr);
			USED(already);
			DBG("ioapic %d addr %p gsib %d %s\n",
				st->ioapic.id, apic->addr, apic->gsib, already);
			break;
		}
	}
}
