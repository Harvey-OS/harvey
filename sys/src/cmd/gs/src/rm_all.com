$ Verif = F$Verify(0)
$ ! OpenVMS command file to emulate behavior of:
$ !
$ !      Define the command for deleting multiple files / patterns
$ !
$ DELETE="DELETE$$$/LOG"
$ IF F$SEARCH("''P1'") .NES. "" THEN DELETE 'P1';*
$ IF F$SEARCH("''P2'") .NES. "" THEN DELETE 'P2';*
$ IF F$SEARCH("''P3'") .NES. "" THEN DELETE 'P3';*
$ IF F$SEARCH("''P4'") .NES. "" THEN DELETE 'P4';*
$ IF F$SEARCH("''P5'") .NES. "" THEN DELETE 'P5';*
$ IF F$SEARCH("''P6'") .NES. "" THEN DELETE 'P6';*
$ IF F$SEARCH("''P7'") .NES. "" THEN DELETE 'P7';*
$ IF F$SEARCH("''P8'") .NES. "" THEN DELETE 'P8';*
$ xxx = F$Verify(Verif)
