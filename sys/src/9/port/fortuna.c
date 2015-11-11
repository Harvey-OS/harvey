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

#include "../port/lib.h"
#include "mem.h"
#include "dat.h"
#include "fns.h"

#include "rijndael.h"
#include "sha2.h"


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
	num_pools	= 23,

	/* in microseconds */
	reseed_interval = 100000,	/* 0.1 sec */

	/* for one big request, reseed after this many bytes */
	reseed_bytes	= (1024*1024),

	/*
	* Skip reseed if pool 0 has less than this many
	* bytes added since last reseed.
	*/
	pool0_fill		= (256/8),

/*
* Algorithm constants
*/

	/* Both cipher key size and hash result size */
	block			= 32,

	/* cipher block size */
	ciph_block		= 16
};




/* for internal wrappers */
typedef SHA256_CTX MD_CTX;
typedef rijndael_ctx CIPH_CTX;

struct fortuna_state
{
	uint8_t		counter[ciph_block];
	uint8_t		result[ciph_block];
	uint8_t		key[block];
	MD_CTX		pool[num_pools];
	CIPH_CTX	ciph;
	unsigned	reseed_count;
	int32_t 	last_reseed_time;
	unsigned	pool0_bytes;
	unsigned	rnd_pos;
	int			tricks_done;
};
typedef struct fortuna_state FState;


/*
 * Use our own wrappers here.
 * - Need to get intermediate result from digest, without affecting it.
 * - Need re-set key on a cipher context.
 * - Algorithms are guaranteed to exist.
 * - No memory allocations.
 */

void
ciph_init(CIPH_CTX * ctx, const uint8_t *key, int klen)
{
	rijndael_set_key(ctx, (const uint32_t *) key, klen, 1);
}

void
ciph_encrypt(CIPH_CTX * ctx, const uint8_t *in, uint8_t *out)
{
	rijndael_encrypt(ctx, (const uint32_t *) in, (uint32_t *) out);
}

void
md_init(MD_CTX * ctx)
{
	SHA256_Init(ctx);
}

void
md_update(MD_CTX * ctx, const uint8_t *data, int len)
{
	SHA256_Update(ctx, data, len);
}

void
md_result(MD_CTX * ctx, uint8_t *dst)
{
	SHA256_CTX	tmp;

	memmove(&tmp, ctx, sizeof(*ctx));
	SHA256_Final(dst, &tmp);
	memset(&tmp, 0, sizeof(tmp));
}

/*
 * initialize state
 */
void
init_state(FState *st)
{
	int			i;

	memset(st, 0, sizeof(*st));
	for (i = 0; i < num_pools; i++)
		md_init(&st->pool[i]);
}

/*
 * Endianess does not matter.
 * It just needs to change without repeating.
 */
void
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
void
encrypt_counter(FState *st, uint8_t *dst)
{
	ciph_encrypt(&st->ciph, st->counter, dst);
	inc_counter(st);
}


/*
 * The time between reseed must be at least reseed_interval
 * microseconds.
 */
int
enough_time_passed(FState *st)
{
	        int                     ok;
        int32_t now;
        int32_t last = st->last_reseed_time;

        now = seconds();

        /* check how much time has passed */
        ok = 0;
        if (now - last >= reseed_interval)
                ok = 1;

        /* reseed will happen, update last_reseed_time */
        if (ok)
                st->last_reseed_time=now;

        return ok;

}

/*
 * generate new key from all the pools
 */
void
reseed(FState *st)
{
	unsigned	k;
	unsigned	n;
	MD_CTX		key_md;
	uint8_t		buf[block];

	/* set pool as empty */
	st->pool0_bytes = 0;

	/*
	 * Both #0 and #1 reseed would use only pool 0. Just skip #0 then.
	 */
	n = ++st->reseed_count;

	/*
	 * The goal: use k-th pool only 1/(2^k) of the time.
	 */
	md_init(&key_md);
	for (k = 0; k < num_pools; k++)
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
unsigned
get_rand_pool(FState *st)
{
	unsigned	rnd;

	/*
	 * This slightly prefers lower pools - thats OK.
	 */
	rnd = st->key[st->rnd_pos] % num_pools;

	st->rnd_pos++;
	if (st->rnd_pos >= block)
		st->rnd_pos = 0;

	return rnd;
}

/*
 * update pools
 */
void
add_entropy(FState *st, const uint8_t *data, unsigned len)
{
	unsigned	pos;
	uint8_t		hash[block];
	MD_CTX		md;

	/* hash given data */
	md_init(&md);
	md_update(&md, data, len);
	md_result(&md, hash);

	/*
	 * Make sure the pool 0 is initialized, then update randomly.
	 */
	if (st->reseed_count == 0)
		pos = 0;
	else
		pos = get_rand_pool(st);
	md_update(&st->pool[pos], hash, block);

	if (pos == 0)
		st->pool0_bytes += len;

	memset(hash, 0, block);
	memset(&md, 0, sizeof(md));
}

/*
 * Just take 2 next blocks as new key
 */
void
rekey(FState *st)
{
	encrypt_counter(st, st->key);
	encrypt_counter(st, st->key + ciph_block);
	ciph_init(&st->ciph, st->key, block);
}

/*
 * Hide public constants. (counter, pools > 0)
 *
 * This can also be viewed as spreading the startup
 * entropy over all of the components.
 */
void
startup_tricks(FState *st)
{
	int			i;
	uint8_t		buf[block];

	/* Use next block as counter. */
	encrypt_counter(st, st->counter);

	/* Now shuffle pools, excluding #0 */
	for (i = 1; i < num_pools; i++)
	{
		encrypt_counter(st, buf);
		encrypt_counter(st, buf + ciph_block);
		md_update(&st->pool[i], buf, block);
	}
	memset(buf, 0, block);

	/* Hide the key. */
	rekey(st);

	/* This can be done only once. */
	st->tricks_done = 1;
}

void
extract_data(FState *st, unsigned count, uint8_t *dst)
{
	unsigned	n;
	unsigned	block_nr = 0;

	/* Should we reseed? */
	if (st->pool0_bytes >= pool0_fill || st->reseed_count == 0)
		if (enough_time_passed(st))
			reseed(st);

	/* Do some randomization on first call */
	if (!st->tricks_done)
		startup_tricks(st);

	while (count > 0)
	{
		/* produce bytes */
		encrypt_counter(st, st->result);

		/* copy result */
		if (count > ciph_block)
			n = ciph_block;
		else
			n = count;
		memmove(dst, st->result, n);
		dst += n;
		count -= n;

		/* must not give out too many bytes with one key */
		block_nr++;
		if (block_nr > (reseed_bytes / ciph_block))
		{
			rekey(st);
			block_nr = 0;
		}
	}
	/* Set new key for next request. */
	rekey(st);
}

/*
 * public interface
 */

FState main_state;
int	init_done = 0;

void
fortuna_add_entropy(const uint8_t *data, unsigned len)
{
	if (!init_done)
	{
		init_state(&main_state);
		init_done = 1;
	}
	if (!data || !len)
		return;
	add_entropy(&main_state, data, len);
}

void
fortuna_get_bytes(unsigned len, uint8_t *dst)
{
	if (!init_done)
	{
		init_state(&main_state);
		init_done = 1;
	}
	if (!dst || !len)
		return;
	extract_data(&main_state, len, dst);
}
