/* This program fixes the C source to get around Sun cpp bug */
%%
"\n"[ \t]*"."		output('.');
","[ \t]*"\n"		output(',');
