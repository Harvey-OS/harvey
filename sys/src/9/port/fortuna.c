/*
 * fortuna.c
 *		Fortuna-like PRNG.
 *
 * Copyright (c) 2005 Marko Kreen
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *	  notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *	  notice, this list of conditions and the following disclaimer in the
 *	  documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.	IN NO EVENT SHALL THE AUTHOR OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 * contrib/pgcrypto/fortuna.c
 */


#include <u.h>
#include <rijndael.h>
#include <sha2.h>

#include "../port/lib.h"
#include "mem.h"
#include "dat.h"
#include "fns.h"




/*
 * Why Fortuna-like: There does not seem to be any definitive reference
 * on Fortuna in the net.  Instead this implementation is based on
 * following references:
 *
 * http://en.wikipedia.org/wiki/Fortuna_(PRNG)
 *	 - Wikipedia article
 * http://jlcooke.ca/random/
 *	 - Jean-Luc Cooke Fortuna-based /dev/random driver for Linux.
 */

/*
 * There is some confusion about whether and how to carry forward
 * the state of the pools.	Seems like original Fortuna does not
 * do it, resetting hash after each request.  I guess expecting
 * feeding to happen more often that requesting.   This is absolutely
 * unsuitable for pgcrypto, as nothing asynchronous happens here.
 *
 * J.L. Cooke fixed this by feeding previous hash to new re-initialized
 * hash context.
 *
 * Fortuna predecessor Yarrow requires ability to query intermediate
 * 'final result' from hash, without affecting it.
 *
 * This implementation uses the Yarrow method - asking intermediate
 * results, but continuing with old state.
 */


/*
 * Algorithm parameters
 */

/*
 * How many pools.
 *
 * Original Fortuna uses 32 pools, that means 32'th pool is
 * used not earlier than in 13th year.	This is a waste in
 * pgcrypto, as we have very low-frequancy seeding.  Here
 * is preferable to have all entropy usable in reasonable time.
 *
 * With 23 pools, 23th pool is used after 9 days which seems
 * more sane.
 *
 * In our case the minimal cycle time would be bit longer
 * than the system-randomness feeding frequency.
 */
enum{
	numPools	= 23,

	/* in microseconds */
	reseedInterval = 100000,	/* 0.1 sec */

	/* for one big request, reseed after this many bytes */
	reseedBytes	= (1024*1024),

	/*
	* Skip reseed if pool 0 has less than this many
	* bytes added since last reseed.
	*/
	pool0Fill		= (256/8),

/*
* Algorithm constants
*/

	/* Both cipher key size and hash result size */
	block			= 32,

	/* cipher block size */
	ciphBlock		= 16
};




/* for internal wrappers */
typedef SHA256Ctx mdCtx;
typedef rijndaelCtx ciphCtx;

struct FState
{
	uint8_t		counter[ciphBlock];
	uint8_t		result[ciphBlock];
	uint8_t		key[block];
	mdCtx		pool[numPools];
	ciphCtx		ciph;
	unsigned	reseedCount;
	int32_t 	lastReseedTime;
	unsigned	pool0Bytes;
	unsigned	rndPos;
	int			tricksDone;
};
typedef struct FState FState;


/*
 * Use our own wrappers here.
 * - Need to get intermediate result from digest, without affecting it.
 * - Need re-set key on a cipher context.
 * - Algorithms are guaranteed to exist.
 * - No memory allocations.
 */

static void
ciph_init(ciphCtx * ctx, const uint8_t *key, int klen)
{
	rijndael_set_key(ctx, (const uint32_t *) key, klen, 1);
}

static void
ciph_encrypt(ciphCtx * ctx, const uint8_t *in, uint8_t *out)
{
	rijndael_encrypt(ctx, (const uint32_t *) in, (uint32_t *) out);
}

static void
md_init(mdCtx * ctx)
{
	SHA256_Init(ctx);
}

static void
md_update(mdCtx * ctx, const uint8_t *data, int len)
{
	SHA256_Update(ctx, data, len);
}

static void
md_result(mdCtx * ctx, uint8_t *dst)
{
	SHA256Ctx	tmp;

	memmove(&tmp, ctx, sizeof(*ctx));
	SHA256_Final(dst, &tmp);
	memset(&tmp, 0, sizeof(tmp));
}

/*
 * initialize state
 */
static void
init_state(FState *st)
{
	int			i;

	memset(st, 0, sizeof(*st));
	for (i = 0; i < numPools; i++)
		md_init(&st->pool[i]);
}

/*
 * Endianess does not matter.
 * It just needs to change without repeating.
 */
static void
inc_counter(FState *st)
{
	uint32_t	   *val = (uint32_t *) st->counter;

	if (++val[0])
		return;
	if (++val[1])
		return;
	if (++val[2])
		return;
	++val[3];
}

/*
 * This is called 'cipher in counter mode'.
 */
static void
encrypt_counter(FState *st, uint8_t *dst)
{
	ciph_encrypt(&st->ciph, st->counter, dst);
	inc_counter(st);
}


/*
 * The time between reseed must be at least reseedInterval
 * microseconds.
 */
static int
enough_time_passed(FState *st)
{
	        int                     ok;
        int32_t now;
        int32_t last = st->lastReseedTime;

        now = seconds();

        /* check how much time has passed */
        ok = 0;
        if (now - last >= reseedInterval)
                ok = 1;

        /* reseed will happen, update lastReseedTime */
        if (ok)
                st->lastReseedTime=now;

        return ok;

}

/*
 * generate new key from all the pools
 */
static void
reseed(FState *st)
{
	unsigned	k;
	unsigned	n;
	mdCtx		key_md;
	uint8_t		buf[block];

	/* set pool as empty */
	st->pool0Bytes = 0;

	/*
	 * Both #0 and #1 reseed would use only pool 0. Just skip #0 then.
	 */
	n = ++st->reseedCount;

	/*
	 * The goal: use k-th pool only 1/(2^k) of the time.
	 */
	md_init(&key_md);
	for (k = 0; k < numPools; k++)
	{
		md_result(&st->pool[k], buf);
		md_update(&key_md, buf, block);

		if (n & 1 || !n)
			break;
		n >>= 1;
	}

	/* add old key into mix too */
	md_update(&key_md, st->key, block);

	/* now we have new key */
	md_result(&key_md, st->key);

	/* use new key */
	ciph_init(&st->ciph, st->key, block);

	memset(&key_md, 0, sizeof(key_md));
	memset(buf, 0, block);
}

/*
 * Pick a random pool.	This uses key bytes as random source.
 */
static unsigned
get_rand_pool(FState *st)
{
	unsigned	rnd;

	/*
	 * This slightly prefers lower pools - thats OK.
	 */
	rnd = st->key[st->rndPos] % numPools;

	st->rndPos++;
	if (st->rndPos >= block)
		st->rndPos = 0;

	return rnd;
}

/*
 * update pools
 */
static void
add_entropy(FState *st, const uint8_t *data, unsigned len)
{
	unsigned	pos;
	uint8_t		hash[block];
	mdCtx		md;

	/* hash given data */
	md_init(&md);
	md_update(&md, data, len);
	md_result(&md, hash);

	/*
	 * Make sure the pool 0 is initialized, then update randomly.
	 */
	if (st->reseedCount == 0)
		pos = 0;
	else
		pos = get_rand_pool(st);
	md_update(&st->pool[pos], hash, block);

	if (pos == 0)
		st->pool0Bytes += len;

	memset(hash, 0, block);
	memset(&md, 0, sizeof(md));
}

/*
 * Just take 2 next blocks as new key
 */
static void
rekey(FState *st)
{
	encrypt_counter(st, st->key);
	encrypt_counter(st, st->key + ciphBlock);
	ciph_init(&st->ciph, st->key, block);
}

/*
 * Hide public constants. (counter, pools > 0)
 *
 * This can also be viewed as spreading the startup
 * entropy over all of the components.
 */
static void
startup_tricks(FState *st)
{
	int			i;
	uint8_t		buf[block];

	/* Use next block as counter. */
	encrypt_counter(st, st->counter);

	/* Now shuffle pools, excluding #0 */
	for (i = 1; i < numPools; i++)
	{
		encrypt_counter(st, buf);
		encrypt_counter(st, buf + ciphBlock);
		md_update(&st->pool[i], buf, block);
	}
	memset(buf, 0, block);

	/* Hide the key. */
	rekey(st);

	/* This can be done only once. */
	st->tricksDone = 1;
}

static void
extract_data(FState *st, unsigned count, uint8_t *dst)
{
	unsigned	n;
	unsigned	block_nr = 0;

	/* Should we reseed? */
	if (st->pool0Bytes >= pool0Fill || st->reseedCount == 0)
		if (enough_time_passed(st))
			reseed(st);

	/* Do some randomization on first call */
	if (!st->tricksDone)
		startup_tricks(st);

	while (count > 0)
	{
		/* produce bytes */
		encrypt_counter(st, st->result);

		/* copy result */
		if (count > ciphBlock)
			n = ciphBlock;
		else
			n = count;
		memmove(dst, st->result, n);
		dst += n;
		count -= n;

		/* must not give out too many bytes with one key */
		block_nr++;
		if (block_nr > (reseedBytes / ciphBlock))
		{
			rekey(st);
			block_nr = 0;
		}
	}
	/* Set new key for next request. */
	rekey(st);
}

static FState mainState;
static int	initDone = 0;

static void init(){
	init_state(&mainState);
	initDone = 1;
}

/*
 * public interface
 */



void
fortuna_add_entropy(const uint8_t *data, unsigned len)
{
	if (!initDone)
	{
		init();
	}
	if (!data || !len)
		return;
	add_entropy(&mainState, data, len);
}

void
fortuna_get_bytes(unsigned len, uint8_t *dst)
{
	if (!initDone)
	{
		init();
	}
	if (!dst || !len)
		return;
	extract_data(&mainState, len, dst);
}
