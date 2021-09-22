.EQ
delim $#
.EN
.NH 3
Invariants
.LP
Reclamation is tricky enough to warrant explicit statement
of the invariants that are needed and the reasons they are true.
This section will use the notation
$b.e#
and
$b.e sub 1#
to denote the allocation and
closing epochs of block
$b#.
The invariants are:
.IP (i)
If $b# points at $bb#, then $bb.e <= b.e < bb.e sub 1#.
.IP (ii)
If $b# points at $bb#, then no other block $b'# with $b'.e = b.e# points at $bb#.
.IP (iii)
If $b# is not marked
.CW BsCopied
and points at $bb# such that $b.e = bb.e#, then no other block $b'# points at $bb#.
.IP (iv)
If $b# is in the active file system and points at $bb# then no other block $b'# in the
active file system points at $bb#.
.IP (v)
If $b'# is a (possibly indirect) copy of $b#, then only one of $b# and $b'# is in the active file system.
.LP
Invariant (i) lets us reclaim blocks using the file system low epoch.
Invariant (iii) lets us reclaim some blocks immediately once they are unlinked.
Invariants (ii), (iv), and (v) are helpful in proving (i) and (iii); collectively they
say that taking snapshots doesn't break the active file system.
.PP
Freshly allocated blocks start filled with nil pointers,
and thus satisfy all the invariants.  We need to check that
copying a block, zeroing a pointer, and setting a pointer
preserve the invariants.
.LP
$"BlockCopy" (b)#
allocates a new block
$b'# and copies the active and open block $b# into $b'#.
.IP (i)
Since $b# is open, all the blocks $bb# it points to are also
active, and thus they have $bb.e sub 1# set to positive infinity
(well,
.CW ~0 ).
Thus (i) is satisfied.
.IP (ii)
Since $b'.e# will be set to the current epoch, and $b.e# is less
than the current epoch (it's copy-on-write), $b.e < b'.e# so (ii)
is vacuously satisfied.
.IP (iii)
Since $b.e < b'.e#, all the pointers in $b# are to blocks with epochs less than $b'.e#.
Thus (iii) is vacuously satisfied for both $b'#.
Since $"blockCopy"# sets the
.CW BsCopied
flag, (iii) is vacuously satisfied for $b#.
.IP (iv),(v)
Since no pointers to $b# or $b'# were modified,
(iv) and (v) are unchanged.
.LP
$"BlockRemoveLink" (b -> bb)# removes from block $b# the pointer to $bb#
.IP
Zeroing a pointer only restricts the preconditions on the 
invariants, so it's always okay.
By (iii), if $b# is not
.CW BsCopied
and $b.e = bb.e#, then no other $b'# anywhere
points at $bb#, so $bb# can be freed.
.LP
$"BlockSetLink" (b->bb sub 0 , bb sub 1)# changes the pointer in block $b# from $bb sub 0# to $bb sub 1#.
We derive sufficient conditions on $bb sub 1#, and then
examine the possible values of $bb sub 0# and $bb sub 1#.
.IP (i)
Since we're changing $b#, $b.e# is the current epoch.
If $bb sub 1# is open, then (i) is satisfied.
.IP (ii)
If either $b.e != bb sub 1 .e# or $bb sub 1# is an orphan, then (ii) is satisfied.
.IP (iii)
If either $b.e != bb sub 1 .e# or $b# is marked
.CW BsCopied
or $bb sub 1# is an orphan, then (iii) is satisfied.
.IP (iv)
If $bb sub 1# is not currently active or $bb sub 1# is an orphan, then (iv) is satisfied.
.IP (v)
If $bb sub 1# is a copy of $bb sub 0# or $bb sub 1# is empty, then (v) is satisfied.
.LP
$"BlockSetLink" (b -> bb sub 0 , "blockAlloc" ())# allocates a new block and points $b# at it.
.IP
Since $bb sub 1# in this case is newly allocated, it is open, an orphan, and empty, and thus
the invariants are satisfied.
.LP
$"BlockSetLink" (b -> bb sub 0 , "blockCopy" (bb sub 0 ))# copies $bb sub 0# and points
$b# at the copy.
.IP
Since $bb sub 1# is newly allocated, it is open and an orphan.  Thus (i)-(iv) are satisfied.
Since $bb sub 1# is a copy of $bb sub 0#, (v) is satisfied.
.LP
$"BlockSetLink" (b -> "nil" , "oldRoot" )# changes a nil pointer to point
at a snapshot root.
.IP (i)
Invariant (i) is broken, but the 
.CW snap
field in the entry will be used to make sure
we don't access the snapshot after it has been reclaimed.
.IP (ii)
Since the epoch of  $"oldRoot"# is less than the current epoch but $b.e# is equal
to the current epoch, (ii) is vacuously true.
.IP (iii)
XXX
.IP (iv)
XXX
.IP (v)
XXX
.PP
Ta da!
xxx
yyyy
zzz
