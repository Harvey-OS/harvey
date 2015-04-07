// prints entry/exit when entering/leaving a function

// for example, spatch with:
// $ for i in kern/src/ns/ kern/src/net kern/drivers/; do \
//       spatch --sp-file scripts/spatch/poor-ftrace.cocci  --in-place $i; done 
// if you have functions you want to ignore, add them to the blacklist in
// kern/src/kdebug.c
@@
type t;
function f;
@@
t f(...) {
+print_func_entry();
<...
+print_func_exit();
return ...;
...>
}
