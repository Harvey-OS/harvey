@err@
function f;
position p;
@@
f@p(...){
<+...
 if (waserror()) {...}
...+>
}

@@
function err.f;
position err.p;
@@

f@p(...) {
++ Proc *up = externup();
...
}
