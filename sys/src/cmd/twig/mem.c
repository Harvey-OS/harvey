#include "common.h"
#include "mem.h"

#define BLKF    100

static struct _mem *mlist;

void
mem_init(struct _mem *mp, int size)
{
        int i, s;
        assert(size>=4);
        mp->size = s = size;
        s *= BLKF;
        for (i=1; i < s; i <<= 1);
        mp->bsize = i;
        mp->blkf = mp->bsize/size;
        mp->cnt = 0;
        mp->freelist = NULL;
        mp->totelem = 0;
        mp->freecnt = 0;
        mp->next = mlist;
        mlist = mp;
};

void *
mem_get(struct _mem *mp)
{
        char *cp;
        if (mp->freelist!=NULL) {
                cp = mp->freelist;
                mp->freelist = *(char **) cp;
                mp->freecnt--;
                memset(cp, 0, mp->size);
                return(cp);
        } else if (mp->cnt==0) {
                mp->block = (char *)Malloc(mp->bsize);
                mp->cnt = mp->blkf;
                mp->totelem += mp->blkf;
        }
        mp->cnt--;
        cp = mp->block;
        mp->block += mp->size;
        return(cp);
}

void
mem_free(struct _mem *mp, char *cp)
{
        *(char **)cp = mp->freelist;
        mp->freelist = cp;
        mp->freecnt++;
}

int
mem_outstanding(struct _mem *mp)
{
        /* returns elements that are outstanding..i.e. asked for
         * but haven't yet been returned
         */
        return(mp->totelem - (mp->cnt+mp->freecnt));
}

void
mem_epilogue(void)
{
        struct _mem *mp;
        /*
         * if the following assertion fails then one of the following
         * has happened:
         * 1) somebody forgot to free something or
         * 2) there is leakage.
         */
        for(mp=mlist; mp!=NULL; mp = mp->next) {
                assert(mem_outstanding(mp)==0);
        }
}
