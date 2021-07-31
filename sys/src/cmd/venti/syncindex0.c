#include "stdinc.h"
#include "dat.h"
#include "fns.h"

enum
{
	ClumpChunks	= 32*1024
};

/*
 * shell sort is plenty good enough
 * because we're going to do a bunch of disk i/o's
 */
static void
sortClumpInfo(ClumpInfo *ci, int *s, int n)
{
	int i, j, m, t;

	for(m = (n + 3) / 5; m > 0; m = (m + 1) / 3){
		for(i = n - m; i-- > 0;){
			for(j = i + m; j < n; j += m){
				if(memcmp(ci[s[j - m]].score, ci[s[j]].score, VtScoreSize) <= 0)
					break;
				t = s[j];
				s[j] = s[j - m];
				s[j - m] = t;
			}
		}
	}
}

int
syncArenaIndex(Index *ix, Arena *arena, u32int clump, u64int a, int fix)
{
	Packet *pack;
	IEntry ie;
	IAddr ia;
	ClumpInfo *ci, *cis;
	u32int now;
	u64int *addrs;
	int i, n, ok, *s;

	now = time(nil);
	cis = MKN(ClumpInfo, ClumpChunks);
	addrs = MKN(u64int, ClumpChunks);
	s = MKN(int, ClumpChunks);
	ok = 1;
	for(; clump < arena->clumps; clump += n){
		n = ClumpChunks;
		if(n > arena->clumps - clump)
			n = arena->clumps - clump;
		n = readClumpInfos(arena, clump, cis, n);
		if(n <= 0){
			fprint(2, "arena directory read failed\n");
			ok = 0;
			break;
		}

		for(i = 0; i < n; i++){
			addrs[i] = a;
			a += cis[i].size + ClumpSize;
			s[i] = i;
		}

		sortClumpInfo(cis, s, n);

		for(i = 0; i < n; i++){
			ci = &cis[s[i]];
			ia.type = ci->type;
			ia.size = ci->uncsize;
			ia.addr = addrs[s[i]];
			ia.blocks = (ci->size + ClumpSize + (1 << ABlockLog) - 1) >> ABlockLog;

			if(!loadIEntry(ix, ci->score, ci->type, &ie))
				fprint(2, "missing block type=%d score=%V\n", ci->type, ci->score);
			else if(!iAddrEq(&ia, &ie.ia)){
				fprint(2, "\nmismatched index entry and clump at %d\n", clump + i);
				fprint(2, "\tclump: type=%d size=%d blocks=%d addr=%lld\n", ia.type, ia.size, ia.blocks, ia.addr);
				fprint(2, "\tindex: type=%d size=%d block=%d addr=%lld\n", ie.ia.type, ie.ia.size, ie.ia.blocks, ie.ia.addr);
				pack = readLump(ie.score, ie.ia.type, ie.ia.size);
				packetFree(pack);
				if(pack != nil){
					fprint(2, "duplicated lump\n");
					continue;
				}
			}else
				continue;
			if(!fix){
				ok = 0;
				continue;
			}
			ie.ia = ia;
			scoreCp(ie.score, ci->score);
			ie.train = 0;
			ie.wtime = now;
			if(!storeIEntry(ix, &ie)){
				fprint(2, "can't fix index: %R");
				ok = 0;
			}
		}

		if(0 && clump / 1000 != (clump + n) / 1000)
			fprint(2, ".");
	}
	free(cis);
	free(addrs);
	free(s);
	return ok;
}

int
syncIndex(Index *ix, int fix)
{
	Arena *arena;
	u64int a;
	u32int clump;
	int i, e, e1, ok, ok1;

	ok = 1;
	for(i = 0; i < ix->narenas; i++){
		arena = ix->arenas[i];
		clump = arena->clumps;
		a = arena->used;
		e = syncArena(arena, TWID32, 1, fix);
		e1 = e;
		if(fix)
			e1 &= ~(SyncHeader|SyncCIZero);
		if(e1)
			ok = 0;
		else{
			ok1 = syncArenaIndex(ix, arena, clump, a + ix->amap[i].start, fix);
			if(fix && ok1 && (e & SyncHeader) && !wbArena(arena))
				fprint(2, "arena=%s header write failed: %R\n", arena->name);
			ok &= ok1;
		}
	}
	if(fix && !wbIndex(ix)){
		fprint(2, "can't write back index header for %s: %R\n", ix->name);
		return 0;
	}
	return ok;
}
