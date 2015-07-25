@  @
expression E;
@@
-incref(E)
+incref(&E->ref)

@  @
expression E;
@@
-decref(E)
+decref(&E->ref)
