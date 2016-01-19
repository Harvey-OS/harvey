#include <u.h>
#include <libc.h>

void check(int, char*);
void checkDir(int, char *);
void rmdir(char *);


void
main(){
	check(chdir("/usr/harvey"), "/usr/harvey");
	checkDir(create("A", OREAD, DMDIR | 0777), "A");
	check(chdir("A"), "A");
	checkDir(create("B", OREAD, DMDIR | 0777), "B");
	checkDir(create("C", OREAD, DMDIR | 0777), "C");
	check(chdir("B"), "B");
	check(chdir(".."), "..");
	remove("B");
	check(chdir("C"), "C");
	check(chdir(".."), "..");
	remove("C");
	check(chdir(".."), "..");
	remove("A");
	print("PASS\n");
	exits("PASS");
}

void check(int r, char *d){
	if(r<0){
		print("chdir(\"%s\") Err: '%r'\n", d);
		print("FAIL\n");
		exits("FAIL");
	}

}

void checkDir(int r, char *d){
	if(r<0){
		print("Error creating dir '%s'\n", d);
		print("FAIL\n");
		exits("FAIL");
	}
}
