.FP lucidasans
.HTML "Venti Backups to Blu-Ray Discs"
.EQ
delim $$
.EN
.
.TL
Venti Backups to Blu-Ray Discs
.AU
Geoff Collyer
.AI
.MH
.SH
Overview
.PP
As a precaution against multiple disks in our
Plan 9
.I venti
server's RAID array failing at about the same time,
or other catastrophic failure, we record
.I venti
arenas,
after they are sealed,
onto dual-layer Blu-Ray discs (BDs).
One could use other large optical discs instead.
.PP
The advertised capacity of a dual-layer BD is 50GB,
but those aren't even disk-manufacturer's (decimal) gigabytes,
which would give a capacity of
$50 times 10 sup 9$ bytes,
which is roughly equivalent to 46.6 gigabytes,
as the term is used by everyone but disk manufacturers.
In the case of BDs,
even that is an exaggeration, with the actual capacity being
closer to $48.44 times 10 sup 9$ bytes,
so the claimed capacity should be read as `50 VAX-gigabytes',
where a
.I VAX-gigabyte
is 968,800,338 bytes.
The default
.I venti
arena size is 512MB, but for some reason our server is configured
with 1GB arenas, so we could fit 46 of them on a BD.
To leave a little extra room for lead-in, lead-out, inter-track gaps,
lossless-linking and the like,
we record 45 arenas per BD.
.PP
The scripts used and records kept are in
.CW /sys/lib/backup
and pertain to the file server on which they are stored.
You will probably want to edit
.CW funcs
to set default file server and Blu-ray device, at minimum.
.SH
Creating and Updating Backups
.PP
We start
.I cdfs
after inserting a disc (virgin or partially-written),
then use
.I venti/rdarena
to copy the next sealed but not backed-up arena
to the start of the unwritten portion of the BD by writing to
.CW /mnt/cd/wd/x .
We don't fixate data BDs, as that seems to tickle a bug,
perhaps in our Sony Blu-Ray burners, that only records the first track
in the disc index upon fixation.
.PP
One can find out which arenas are sealed and which are open by viewing
.CW http://\fIventi-server\fP/index .
We also periodically print the most recent
.I fossil
dump scores and save the paper with the discs.
.PP
All of this is packaged up as scripts in
.CW /sys/lib/backup ,
notably
.CW backup .
A typical invocation would be just
.DS
.ft CW
cd /sys/lib/backup
backup
.ft
.DE
which will guide its invoker as to which discs to insert into a Blu-ray
burner and when.
This works for a first full backup and for subsequent incremental backups
of just the newly-sealed arenas.
There is provision for burning multiple backup sets, the default being
.CW set1 .
To burn a second set, we would
.DS
.ft CW
mkdir /sys/lib/backup/set2
backup -s set2
.ft
.DE
.CW backup
and related scripts keep track of which BD is current and how many
tracks are recorded, and which arenas have been dumped to BD.
.PP
We have recorded two sets of BD backups, one of which will go to
Antwerp to seed their
.I venti
server and also serve as off-site backup for Murray Hill.
.SH
Restoring from Backups
.PP
After a disaster, or when setting up a new
.I venti
server from BD backups,
the first step is to get Plan 9 running on the new
.I venti
server.
This could be done by installing into a smallish (2GB)
.I fossil
partition from a Plan 9 installation CD, if necessary.
One would then initialise the new disk partitions per
.I venti-fmt (8)
and read all the arenas on all the BDs into the new
.I venti
by running
.I venti/wrarena
once per BD track (arena).
.br
.ne 3
.PP
To add the contents of a backup BD to a (possibly fresh)
.I venti
store,
shut down the
.I venti
server,
format the arenas partition, then
run
.DS
.ft CW
restore \fIfirst-arena-number\fP
.ft
.DE
after inserting a BD into the Blu-ray drive.
Repeat this for each BD in the backup set.
When all the arenas have been restored,
it will be necessary to build a new
.I venti
index,
the usual steps being to run
.I checkarenas ,
.I fmtisect ,
.I fmtbloom ,
.I fmtindex ,
and
.I "buildindex -b" ,
all from
.I venti-fmt (8).
Then the
.I venti
server may be restarted.
.PP
Once the
.I venti
storage has been restored,
a new
.I fossil
partition (perhaps the existing one or another one)
can be initialised from the last
.I fossil
dump score corresponding to the last arena on BD
(see
.I fossil/flfmt
in
.I fossil (4)):
.DS
.ft CW
fossil/flfmt -v c388...32b5 /dev/sdC0/fossil
.ft
.DE
