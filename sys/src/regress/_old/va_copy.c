#include <u.h>
#include <libc.h>
#include <stdio.h>

void
main(){
	char *correct="0.987650\n";
	static char result[128];

	sprintf(result, "%f\n", 0.98765f);

	if(!strcmp(result, correct)){
		print("PASS\n");
		exits("PASS");
	}
	else{
		print("FAIL\n");
		exits("FAIL");
	}
}
