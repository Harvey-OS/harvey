# we declare a working gzprintf() in hacks.c so remove the collision by renaming the current
s/gzprintf/gzprintf_/g

# remove the unneeded cast in gzwrite() to avoid the warning
#harvey/gzwrite.c: In function ‘gzwrite’:
#harvey/gzwrite.c:221:17: error: pointer targets in assignment differ in signedness [-Werror=pointer-sign]
#             buf = (const char *)buf + copy;
#                 ^
s/ buf = (const char \*)buf / buf = buf  /g

# add a cast in gzputs() to avoid the the warning
#harvey/gzwrite.c: In function ‘gzputs’:
#harvey/gzwrite.c:302:25: error: pointer targets in passing argument 2 of ‘gzwrite’ differ in signedness [-Werror=pointer-sign]
#     ret = gzwrite(file, str, len);
#                         ^
#harvey/gzwrite.c:165:13: note: expected ‘voidpc’ but argument is of type ‘const char *’
# int ZEXPORT gzwrite(file, buf, len)
#             ^
s/ret = gzwrite(file, str, len);/ret = gzwrite(file, (voidpc)str, len);/g
