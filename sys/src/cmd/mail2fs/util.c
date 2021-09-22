#include <u.h>
#include <libc.h>
#include <ctype.h>

char*
mailnamenb(char *name)
{
	Rune	dummy;

	if(isdigit(name[0]) == 0){
		name += chartorune(&dummy, name);
		if(name[0] == '.')
			name++;
	}
	return name;
}

char*
cleanpath(char* file, char* dir)
{
	char*	s;
	char*	t;
	char	cwd[512];

	assert(file && file[0]);
	if(file[1])
		file = strdup(file);
	else {
		s = file;
		file = malloc(3);
		file[0] = s[0];
		file[1] = 0;
	}
	s = cleanname(file);
	if(s[0] != '/' && access(s, AEXIST) == 0){
		getwd(cwd, sizeof(cwd));
		t = smprint("%s/%s", cwd, s);
		free(s);
		return t;
	}
	if(s[0] != '/' && dir != nil && dir[0] != 0){
		t = smprint("%s/%s", dir, s);
		free(s);
		s = cleanname(t);
	}
	return s;
}

