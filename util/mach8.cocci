@mr exists@
typedef Mach; // only needed once per semantic patch
idexpression Mach *m;
function f;
position p;
identifier d;
@@
f@p(...){
<+...
m->d
...+>
}

@@
function mr.f;
position mr.p;
@@

f@p(...) {
++ Mach *m = machp();
...
}

@r exists@
function f;
position p;
@@
f@p(...){
<+...
 if (waserror()) {...}
...+>
}

@@
function r.f;
position r.p;
@@

f@p(...) {
++ Mach *m = machp();
...
}

@nr exists@
function f;
position p;
@@
f@p(...){
<+...
 if (!waserror()) {...}
...+>
}

@@
function nr.f;
position nr.p;
@@

f@p(...) {
++ Mach *m = machp();
...
}

