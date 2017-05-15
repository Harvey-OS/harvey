/* hashtable tests. These come from Akaros.
 * 
 * The Akaros kernel is licensed under the GNU General Public License,
 * version 2.  Our kernel is made up of code from a number of other
 * systems.  Anything written for the Akaros kernel is licensed "GPLv2
 * or later".  However, other code, such as from Linux and Plan 9, are
 * licensed GPLv2, without the "or later" clause.  There is also code
 * from BSD, Xen, JOS, and Plan 9 derivatives.  As a whole, the kernel
 * is licensed GPLv2.
 */

#include <u.h>
#include <libc.h>
#include <hashtable.h>

#define KT_ASSERT_M(message, test)                                               \
	do {                                                                     \
		if (!(test)) {                                                   \
			char fmt[] = "Assertion failure in %s() at %s:%d: %s";   \
			print(fmt, __FUNCTION__, __FILE__, __LINE__, message);   \
			return 0;                                                \
		}                                                                \
	} while (0)

static size_t test_hash_fn_col(void *k)
{
	return (size_t)k % 2; // collisions in slots 0 and 1
}

static int test_hashtable(void)
{
	struct test {int x; int y;};
	struct test tstruct[10];

	struct hashtable *h;
	uintptr_t k = 5;
	struct test *v = &tstruct[0];

	h = create_hashtable(32, __generic_hash, __generic_eq);

	// test inserting one item, then finding it again
	KT_ASSERT_M("It should be possible to insert items to a hashtable",
	            hashtable_insert(h, (void*)k, v));
	v = nil;
	KT_ASSERT_M("It should be possible to find inserted stuff in a hashtable",
	            (v = hashtable_search(h, (void*)k)));

	KT_ASSERT_M("The extracted element should be the same we inserted",
	            (v == &tstruct[0]));

	v = nil;

	KT_ASSERT_M("It should be possible to remove an existing element",
	            (v = hashtable_remove(h, (void*)k)));

	KT_ASSERT_M("An element should not remain in a hashtable after deletion",
	            !(v = hashtable_search(h, (void*)k)));

	/* Testing a bunch of items, insert, search, and removal */
	for (int i = 0; i < 10; i++) {
		k = i; // vary the key, we don't do KEY collisions
		KT_ASSERT_M("It should be possible to insert elements to a hashtable",
		            (hashtable_insert(h, (void*)k, &tstruct[i])));
	}
	// read out the 10 items
	for (int i = 0; i < 10; i++) {
		k = i;
		KT_ASSERT_M("It should be possible to find inserted stuff in a hashtable",
		            (v = hashtable_search(h, (void*)k)));
		KT_ASSERT_M("The extracted element should be the same we inserted",
		            (v == &tstruct[i]));
	}

	KT_ASSERT_M("The total count of number of elements should be 10",
	            (10 == hashtable_count(h)));

	// remove the 10 items
	for (int i = 0; i < 10; i++) {
		k = i;
		KT_ASSERT_M("It should be possible to remove an existing element",
		            (v = hashtable_remove(h, (void*)k)));

	}
	// make sure they are all gone
	for (int i = 0; i < 10; i++) {
		k = i;
		KT_ASSERT_M("An element should not remain in a hashtable after deletion",
		            !(v = hashtable_search(h, (void*)k)));
	}

	KT_ASSERT_M("The hashtable should be empty",
	            (0 == hashtable_count(h)));

	hashtable_destroy(h);

	// same test of a bunch of items, but with collisions.
	/* Testing a bunch of items with collisions, etc. */
	h = create_hashtable(32, test_hash_fn_col, __generic_eq);
	// insert 10 items
	for (int i = 0; i < 10; i++) {
		k = i; // vary the key, we don't do KEY collisions

		KT_ASSERT_M("It should be possible to insert elements to a hashtable",
		            (hashtable_insert(h, (void*)k, &tstruct[i])));
	}
	// read out the 10 items
	for (int i = 0; i < 10; i++) {
		k = i;
		KT_ASSERT_M("It should be possible to find inserted stuff in a hashtable",
		            (v = hashtable_search(h, (void*)k)));
		KT_ASSERT_M("The extracted element should be the same we inserted",
		            (v == &tstruct[i]));
	}

	KT_ASSERT_M("The total count of number of elements should be 10",
	            (10 == hashtable_count(h)));

	// remove the 10 items
	for (int i = 0; i < 10; i++) {
		k = i;
		KT_ASSERT_M("It should be possible to remove an existing element",
		            (v = hashtable_remove(h, (void*)k)));
	}
	// make sure they are all gone
	for (int i = 0; i < 10; i++) {
		k = i;

		KT_ASSERT_M("An element should not remain in a hashtable after deletion",
		            !(v = hashtable_search(h, (void*)k)));
	}

	KT_ASSERT_M("The hashtable should be empty",
	            (0 == hashtable_count(h)));

	hashtable_destroy(h);

	return 1;
}

void
main(int argc, char *argv[])
{
	if (test_hashtable() != 1) {
		exits("FAIL");
	}

	print("PASS\n");
	exits("PASS");

}
