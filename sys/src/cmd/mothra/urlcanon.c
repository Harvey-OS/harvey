#include <u.h>
#include <libc.h>
void *emalloc(int n){
	void *p;
	p=malloc(n);
	if(p==0){
		fprint(2, "can't malloc\n");
		exits("no mem");
	}
	return p;
}
void urlcanon(char *name){
	char *s, *t;
	char **comp, **p, **q;
	int rooted;
	rooted=name[0]=='/';
	/*
	 * Break the name into a list of components
	 */
	comp=emalloc(strlen(name)*sizeof(char *));
	p=comp;
	*p++=name;
	for(s=name;;s++){
		if(*s=='/'){
			*p++=s+1;
			*s='\0';
		}
		else if(*s=='\0')
			break;
	}
	*p=0;
	/*
	 * go through the component list, deleting components that are empty (except
	 * the last component) or ., and any .. and its non-.. predecessor.
	 */
	p=q=comp;
	while(*p){
		if(strcmp(*p, "")==0 && p[1]!=0
		|| strcmp(*p, ".")==0)
			p++;
		else if(strcmp(*p, "..")==0 && q!=comp && strcmp(q[-1], "..")!=0){
			--q;
			p++;
		}
		else
			*q++=*p++;
	}
	*q=0;
	/*
	 * rebuild the path name
	 */
	s=name;
	if(rooted) *s++='/';
	for(p=comp;*p;p++){
		t=*p;
		while(*t) *s++=*t++;
		if(p[1]!=0) *s++='/';
	}
	*s='\0';
	free(comp);
}
void main(int argc, char *argv[]){
	int i;
	for(i=1;i!=argc;i++){
		print("%s: ", argv[i]);
		urlcanon(argv[i]);
		print("%s\n", argv[i]);
	}
	exits(0);
}
