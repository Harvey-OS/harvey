!!/bin/sh
!
!   This script file makes a new TeX PK font, because one wasn't
!   found.  Parameters are:
!
!   name dpi bdpi magnification [mode]
!
!   `name' is the name of the font, such as `cmr10'.  `dpi' is
!   the resolution the font is needed at.  `bdpi' is the base
!   resolution, useful for figuring out the mode to make the font
!   in.  `magnification' is a string to pass to MF as the
!   magnification.  `mode', if supplied, is the mode to use.
!
!   Note that this file must execute Metafont, and then gftopk,
!   and place the result in the correct location for the PostScript
!   driver to find it subsequently.  If this doesn't work, it will
!   be evident because MF will be invoked over and over again.
!
!   Of course, it needs to be set up for your site.
!
!
$   set mess/nofa/nose/note/noid
$   newtex
$   set mess/se/fa/id/te
$ NAME = f$sear("mf_inputs:"+p1+".mf")
$ onlyname=f$parse(name,,,"name")
$ name= f$parse(name,,,"device")+f$parse(name,,,"directory")+f$parse(name,,, -
      "name")
$ DPI  = f$integer(P2)
$ BDPI = f$integer(P3)
$ MAG  = f$edit(P4,"LOWERCASE")
$ MODE = P5
$ if MODE .EQS. "" then 
$   if BDPI .EQ. 300 
$   then
$      MODE := "imagen"
$      goto GotMode
$   endif
$   if BDPI .EQ. 400 
$   then
$      MODE := "nexthi"
$      goto GotMode
$   endif
$   if BDPI .EQ. 100 
$   then
$      MODE := "nextscreen"
$      goto GotMode
$   endif
$   if BDPI .EQ. 635 
$   then
$      MODE := "linolo"
$      goto GotMode
$   endif
$   if BDPI .EQ. 1270
$    then
$      MODE := "linohi"
$      goto GotMode
$   endif
$   if BDPI .EQ. 2540
$    then
$      MODE := "linosuper"
$      goto GotMode
$   endif
$   write sys$output "I don't know the mode for ",BDPI
$   write sys$output "Have your system admin update MakeTeXPK"
$   exit 1
$ GotMode:
$ 
$ GFNAME = f$fao("!AS.!ASgf",onlyNAME,f$string(DPI))
$ PKNAME = f$fao("!AS.!ASPK",onlyNAME,f$string(DPI))
$ 
$ pkfilename = f$fao("tex_pk:!AS",PKNAME)
$ pkfile = f$search (pkfilename)
$ if pkfile .EQS. "" 
$ then
$!    Clean up on normal or abnormal exit
$     on control_y then goto ControlY
$     on error then goto ControlY
$     on severe_error then goto ControlY
$ 
$     OLDDIR = f$environment("DEFAULT")
$     set def sys$login
$     command = f$fao  -
  ("mf ""\mode:=!AS; mag:=!AS; scrollmode ; input !AS""", -
  MODE,MAG,NAME)
$     write sys$output "Creating font file for ",onlyname
$     write sys$output "Please wait about 2 minutes"
$     write sys$output ""
$     set noon
$     command
$     set on
$
$     gffile=f$sear(onlyname+"."+f$str(dpi)+"gf")
$     if gffile .EQS. ""
$     then
$         write sys$output "Metafont failed for some reason on ",gfname
$         goto ControlY
$     endif
$  
$     gftopk  'gfname' 
$     delete/noconf/nolog 'gfname';
$     delete/noconf/nolog 'onlyname'.lis;
$     
$   if f$getsyi("nodename").eqs."VAXSNS"   
$   then
$     on error then goto goodexit
$     on severe_error then goto goodexit
$     send/file/vms 'pkname' newtex
$!     write sys$output "------- ",pkname," has been sent to NEWTEX"
$     send/file/vms 'onlyname'.tfm newtex
$!     write sys$output "------- ",onlyname,".TFM has been sent to NEWTEX"
$   endif
$
$     goto GoodExit
$ else
$     write sys$output "tex_pk:",PKNAME," already exists!"
$     exit 
$ endif
$
$
$ GoodExit:
$   set default 'olddir
$   exit 
$
$ ControlY:
$   set default 'olddir
$   exit 1
