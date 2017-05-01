/*
 * This file is part of the UCB release of Plan 9. It is subject to the license
 * terms in the LICENSE file found in the top-level directory of this
 * distribution and at http://akaros.cs.berkeley.edu/files/Plan9License. No
 * part of the UCB release of Plan 9, including this file, may be copied,
 * modified, propagated, or distributed except according to the terms contained
 * in the LICENSE file.
 */

#include "stdinc.h"
#include "dat.h"
#include "fns.h"

int			syncwrites = 0;
int			queuewrites = 0;
int			writestodevnull = 0;
int			verifywrites = 0;

static Packet		*readilump(Lump *u, IAddr *ia, uint8_t *score);

/*
 * Some of this logic is duplicated in hdisk.c
 */
Packet*
readlump(uint8_t *score, int type, uint32_t size, int *cached)
{
	Lump *u;
	Packet *p;
	IAddr ia;
	uint32_t n;

	trace(TraceLump, "readlump enter");
/*
	qlock(&stats.lock);
	stats.lumpreads++;
	qunlock(&stats.lock);
*/
	if(scorecmp(score, zeroscore) == 0)
		return packetalloc();
	u = lookuplump(score, type);
	if(u->data != nil){
		trace(TraceLump, "readlump lookuplump hit");
		if(cached)
			*cached = 1;
		n = packetsize(u->data);
		if(n > size){
			seterr(EOk, "read too small: asked for %d need at least %d", size, n);
			putlump(u);

			return nil;
		}
		p = packetdup(u->data, 0, n);
		putlump(u);
		return p;
	}

	if(cached)
		*cached = 0;

	if(lookupscore(score, type, &ia) < 0){
		/* ZZZ place to check for someone trying to guess scores */
		seterr(EOk, "no block with score %V/%d exists", score, type);

		putlump(u);
		return nil;
	}
	if(ia.size > size){
		seterr(EOk, "read too small 1: asked for %d need at least %d", size, ia.size);

		putlump(u);
		return nil;
	}

	trace(TraceLump, "readlump readilump");
	p = readilump(u, &ia, score);
	putlump(u);

	trace(TraceLump, "readlump exit");
	return p;
}

/*
 * save away a lump, and return it's score.
 * doesn't store duplicates, but checks that the data is really the same.
 */
int
writelump(Packet *p, uint8_t *score, int type, uint32_t creator, uint ms)
{
	Lump *u;
	int ok;

/*
	qlock(&stats.lock);
	stats.lumpwrites++;
	qunlock(&stats.lock);
*/

	packetsha1(p, score);
	if(packetsize(p) == 0 || writestodevnull==1){
		packetfree(p);
		return 0;
	}

	u = lookuplump(score, type);
	if(u->data != nil){
		ok = 0;
		if(packetcmp(p, u->data) != 0){
			uint8_t nscore[VtScoreSize];

			packetsha1(u->data, nscore);
			if(scorecmp(u->score, score) != 0)
				seterr(EStrange, "lookuplump returned bad score %V not %V", u->score, score);
			else if(scorecmp(u->score, nscore) != 0)
				seterr(EStrange, "lookuplump returned bad data %V not %V", nscore, u->score);
			else
				seterr(EStrange, "score collision %V", score);
			ok = -1;
		}
		packetfree(p);
		putlump(u);
		return ok;
	}

	if(writestodevnull==2){
		packetfree(p);
		return 0;
	}

	if(queuewrites)
		return queuewrite(u, p, creator, ms);

	ok = writeqlump(u, p, creator, ms);

	putlump(u);
	return ok;
}

int
writeqlump(Lump *u, Packet *p, int creator, uint ms)
{
	ZBlock *flat;
	Packet *old;
	IAddr ia;
	int ok;

	if(lookupscore(u->score, u->type, &ia) == 0){
		if(verifywrites == 0){
			/* assume the data is here! */
			packetfree(p);
			ms = msec() - ms;
			addstat2(StatRpcWriteOld, 1, StatRpcWriteOldTime, ms);
			return 0;
		}

		/*
		 * if the read fails,
		 * assume it was corrupted data and store the block again
		 */
		old = readilump(u, &ia, u->score);
		if(old != nil){
			ok = 0;
			if(packetcmp(p, old) != 0){
				uint8_t nscore[VtScoreSize];

				packetsha1(old, nscore);
				if(scorecmp(u->score, nscore) != 0)
					seterr(EStrange, "readilump returned bad data %V not %V", nscore, u->score);
				else
					seterr(EStrange, "score collision %V", u->score);
				ok = -1;
			}
			packetfree(p);
			packetfree(old);

			ms = msec() - ms;
			addstat2(StatRpcWriteOld, 1, StatRpcWriteOldTime, ms);
			return ok;
		}
		logerr(EAdmin, "writelump: read %V failed, rewriting: %r\n", u->score);
	}

	flat = packet2zblock(p, packetsize(p));
	ok = storeclump(mainindex, flat, u->score, u->type, creator, &ia);
	freezblock(flat);
	if(ok == 0)
		insertlump(u, p);
	else
		packetfree(p);

	if(syncwrites){
		flushdcache();
		flushicache();
		flushdcache();
	}

	ms = msec() - ms;
	addstat2(StatRpcWriteNew, 1, StatRpcWriteNewTime, ms);
	return ok;
}

static Packet*
readilump(Lump *u, IAddr *ia, uint8_t *score)
{
	Arena *arena;
	ZBlock *zb;
	Packet *p, *pp;
	Clump cl;
	uint64_t aa;
	uint8_t sc[VtScoreSize];

	trace(TraceLump, "readilump enter");
	arena = amapitoa(mainindex, ia->addr, &aa);
	if(arena == nil){
		trace(TraceLump, "readilump amapitoa failed");
		return nil;
	}

	trace(TraceLump, "readilump loadclump");
	zb = loadclump(arena, aa, ia->blocks, &cl, sc, paranoid);
	if(zb == nil){
		trace(TraceLump, "readilump loadclump failed");
		return nil;
	}

	if(ia->size != cl.info.uncsize){
		seterr(EInconsist, "index and clump size mismatch");
		freezblock(zb);
		return nil;
	}
	if(ia->type != cl.info.type){
		seterr(EInconsist, "index and clump type mismatch");
		freezblock(zb);
		return nil;
	}
	if(scorecmp(score, sc) != 0){
		seterr(ECrash, "score mismatch");
		freezblock(zb);
		return nil;
	}

	trace(TraceLump, "readilump success");
	p = zblock2packet(zb, cl.info.uncsize);
	freezblock(zb);
	pp = packetdup(p, 0, packetsize(p));
	trace(TraceLump, "readilump insertlump");
	insertlump(u, pp);
	trace(TraceLump, "readilump exit");
	return p;
}
