# by David A. Wheeler. Last changed 12/29/89.
# modified by Terry L Anderson last change 91-Mar-21
#
#joins lines ending in a back slash or back slash space
:join
/\\$/N
s/\\\n//g
/\\ $/N
s/\\ \n//g
tjoin
# /^\.H /s/"//g
# /^\.FG /s/"//g
# /^\.TB /s/"//g
# /^\.UL /s/"//g
s/%/\\%/g
s/\\/\\backslash/g
s/\_/\\_/g
s/\$/\\$/g
s/#/\\#/g
s/&/\\&/g
s/{/$\\{$/g
s/}/$\\}$/g
#s/\\backslashfI /{\\it\\ /g
#s/\\backslashfI/{\\it /g
#s/\\backslashf(CW /{\\tt\\ /g
#s/\\backslashf(CW/{\\tt /g
#s/\\backslashfR /{\\rm\\ /g
#s/\\backslashfR/{\\rm /g
#s/\\backslashfB /{\\bf\\ /g
#s/\\backslashfB/{\\bf /g
#s/\\backslashfP /\ }/g
#s/\\backslashfP/}/g
# s/	/\\t/g #can't do tabs!!
# handle special symbols
s/\\backslash(sc/\\S /g
s/\\backslash(em/---/g
s/\\backslash(bu/\\bullet /g
s/\\backslash(dg/\\dag /g
s/\\backslash(dd/\\ddag /g
s/\\backslash(rg/\\copy\\regbox /g
s/\\backslash(co/\\copyright /g
s/\\backslash(tm/\\copy\\tmbox /g
s/\\backslash\*(Tm/\\copy\\tmbox /g
s/\\backslash(ct/\\cents /g
s/\\backslash(de/$^\circ$/g
s/\\backslash(fm/'/g
# MM strings
s/\\backslash(EM/---/g
s/\\backslash(BU/\\bullet /g
s/\\backslash(DT/\\today /
#
# backslashes left to print should be in math mode
s/\\backslash/$\\backslash$/g
# remove quotes from command lines and replace spaces with $.$
# this allows the awk script to see them as one arg 
# another sed script will change them back after awk
:remsp
/^\./s/\(.*".*\)\( \)\(.*".*\)/\1$\.$\3/g
/^\./s/"\([^ ]*\)"/\1/
tremsp

