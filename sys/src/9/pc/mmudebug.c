#include	"u.h"
#include	"../port/lib.h"
#include	"mem.h"
#include	"dat.h"
#include	"fns.h"
#include	"io.h"

void
countpagerefs(ulong *ref, int print)
{
	int i, n;
	Mach *mm;
	Page *pg;
	Proc *p;

	n = 0;
	for(i=0; i<conf.nproc; i++){
		p = proctab(i);
		if(p->mmupdb){
			if(print){
				if(ref[pagenumber(p->mmupdb)])
					iprint("page %#.8lux is proc %d (pid %lud) pdb\n",
						p->mmupdb->pa, i, p->pid);
				continue;
			}
			if(ref[pagenumber(p->mmupdb)]++ == 0)
				n++;
			else
				iprint("page %#.8lux is proc %d (pid %lud) pdb but has other refs!\n",
					p->mmupdb->pa, i, p->pid);
		}
		if(p->kmaptable){
			if(print){
				if(ref[pagenumber(p->kmaptable)])
					iprint("page %#.8lux is proc %d (pid %lud) kmaptable\n",
						p->kmaptable->pa, i, p->pid);
				continue;
			}
			if(ref[pagenumber(p->kmaptable)]++ == 0)
				n++;
			else
				iprint("page %#.8lux is proc %d (pid %lud) kmaptable but has other refs!\n",
					p->kmaptable->pa, i, p->pid);
		}
		for(pg=p->mmuused; pg; pg=pg->next){
			if(print){
				if(ref[pagenumber(pg)])
					iprint("page %#.8lux is on proc %d (pid %lud) mmuused\n",
						pg->pa, i, p->pid);
				continue;
			}
			if(ref[pagenumber(pg)]++ == 0)
				n++;
			else
				iprint("page %#.8lux is on proc %d (pid %lud) mmuused but has other refs!\n",
					pg->pa, i, p->pid);
		}
		for(pg=p->mmufree; pg; pg=pg->next){
			if(print){
				if(ref[pagenumber(pg)])
					iprint("page %#.8lux is on proc %d (pid %lud) mmufree\n",
						pg->pa, i, p->pid);
				continue;
			}
			if(ref[pagenumber(pg)]++ == 0)
				n++;
			else
				iprint("page %#.8lux is on proc %d (pid %lud) mmufree but has other refs!\n",
					pg->pa, i, p->pid);
		}
	}
	if(!print)
		iprint("%d pages in proc mmu\n", n);
	n = 0;
	for(i=0; i<conf.nmach; i++){
		mm = MACHP(i);
		for(pg=mm->pdbpool; pg; pg=pg->next){
			if(print){
				if(ref[pagenumber(pg)])
					iprint("page %#.8lux is in cpu%d pdbpool\n",
						pg->pa, i);
				continue;
			}
			if(ref[pagenumber(pg)]++ == 0)
				n++;
			else
				iprint("page %#.8lux is in cpu%d pdbpool but has other refs!\n",
					pg->pa, i);
		}
	}
	if(!print){
		iprint("%d pages in mach pdbpools\n", n);
		for(i=0; i<conf.nmach; i++)
			iprint("cpu%d: %d pdballoc, %d pdbfree\n",
				i, MACHP(i)->pdballoc, MACHP(i)->pdbfree);
	}
}
