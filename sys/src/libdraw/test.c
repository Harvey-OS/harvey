#include <u.h>
#include <libc.h>
#include <draw.h>


void
main(int argc, char **argv)
{
		print("%dn", wordsperline(Rect(atoi(argv[1]), atoi(argv[2]), atoi(argv[3]), atoi(argv[4])), atoi(argv[5])));
}
