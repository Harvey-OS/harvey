@  @
expression E;
@@
-qlock(E)
+qlock(&E->lock)

@  @
expression E;
@@
-qunlock(E)
+qunlock(&E->lock)
