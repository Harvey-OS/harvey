#!/bin/mksh
exec_prefix="NONE"
prefix="LALA"
for ac_var in exec_prefix prefix bindir sbindir libexecdir datarootdir \
 datadir sysconfdir sharedstatedir localstatedir includedir \
  oldincludedir docdir infodir htmldir dvidir pdfdir \
   libdir localedir mandir
do
  eval ac_val=\$$ac_var
  echo $ac_val
  case $ac_val in                                                                                                                                                                       
    [\\/$]* | ?:[\\/]* )  continue;;
    NONE | '' )
              case $ac_var in *prefix ) 
                    echo $ac_var;
                    continue;;
              esac;;
  esac
done
