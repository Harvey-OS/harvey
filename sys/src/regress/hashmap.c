#include <u.h>
#include <libc.h>
#include <hashmap.h>

enum {
	verbose = 1,
};

int
main(void)
{
	Hashmap map;
	uint64_t *keys;
	size_t stats[40];
	size_t i, j, nkeys, maxkeys;

	hmapinit(&map);

	maxkeys = 1000000;
	nkeys = maxkeys;
	keys = malloc(nkeys * sizeof keys[0]);

	for(j = 0; j < 100; j++){
		nkeys = maxkeys / (100-j);
		uintptr_t rnd = (uintptr_t)rand() << 32;
		rnd |= rand();
		rnd &= ~1023ull;
		if(j == 99)
			rnd = 0;
		for(i = 0; i < nkeys; i++)
			keys[i] = rnd + ((uintptr_t)i<<12);

		for(i = 0; i < nkeys; i++){
			int err;
			if((err = hmapput(&map, keys[i], (uint64_t)i)) != 0){
				fprint(2, "hmapput: error: %d\n", err);
				exits(smprint("hmapput: error: %d\n", err));
			}
		}
		for(i = 0; i < nkeys; i++){
			uint64_t val;
			int err;
			if((err = hmapget(&map, keys[i], &val)) != 0){
				fprint(2, "hmapget: error: %d\n", err);
				exits(smprint("hmapget: error: %d\n", err));
			}
			if(val != (uint64_t)i){
				fprint(2, "got the wrong val.\n");
				exits(smprint("got the wrong val.\n"));
			}
		}
		if(verbose){
			size_t sum = 0;
			hmapstats(&map, stats, nelem(stats));
			for(i = 0; i < nelem(stats); i++){
				if(stats[i] > 0){
					sum += stats[i];
					print("size %u: %u %u\n", i, sum, stats[i]);
				}
			}
		}
		for(i = 0; i < nkeys; i++){
			uint64_t val;
			int err;
			if((err = hmapdel(&map, keys[i], &val)) != 0){
				fprint(2, "hmapget: error: %d\n", err);
				exits(smprint("hmapget: error: %d\n", err));
			}
			if(val != (uint64_t)i){
				fprint(2, "got the wrong val.\n");
				exits("got the wrong val.\n");
			}
		}
	}
	if(map.cur->len != 0){
		fprint(2, "map isn't empty at the end.\n");
		exits("map isn't empty at the end.\n");
	}
	print("map cap %u at the end\n", map.cur->cap);

	fprint(2, "random put/del sequencing\n");

	for(j = 0; j < 100*maxkeys; j++){
		i = rand()%maxkeys;
		uintptr_t key = (uintptr_t)keys[i];
		if((key&1) == 0){
			int err;
			if((err = hmapput(&map, key, (uint64_t)i)) != 0){
				fprint(2, "hmapput: error: %d\n", err);
				exits(smprint("hmapput: error: %d\n", err));
			}
			key |= 1;
			keys[i] = key;
		} else {
			uint64_t val;
			int err;
			key &= ~1;
			if((err = hmapdel(&map, key, &val)) != 0){
				fprint(2, "hmapget: error: %d\n", err);
				exits(smprint("hmapget: error: %d\n", err));
			}
			if(val != (uint64_t)i){
				fprint(2, "got the wrong val.\n");
				exits(smprint("got the wrong val.\n"));
			}
			keys[i] = key;
		}
	}
	for(i = 0; i < maxkeys; i++){
		uintptr_t key = (uintptr_t)keys[i];
		if((key&1) == 0)
			continue;
		uint64_t val;
		int err;
		key &= ~1;
		if((err = hmapdel(&map, key, &val)) != 0){
			fprint(2, "hmapget: error: %d\n", err);
			exits(smprint("hmapget: error: %d\n", err));
		}
		if(val != (uint64_t)i){
			fprint(2, "got the wrong val.\n");
			exits(smprint("got the wrong val.\n"));
		}
	}
	if(map.cur->len != 0){
		fprint(2, "map isn't empty at the end.\n");
		exits(smprint("map isn't empty at the end.\n"));
	}
	print("map cap %u at the end\n", map.cur->cap);
	free(keys);
	hmapfree(&map);
	return 0;
}
