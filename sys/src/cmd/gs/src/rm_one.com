$ Verif = F$Verify(0)
$ ! OpenVMS command file to emulate behavior of:
$ !
$ !     Define the command for deleting (a) file(s) (including wild cards)
$ !
$ DELETE="DELETE$$$/LOG"
$ FILE = F$SEARCH("''P1'")
$ IF "''FILE'" .NES. "" THEN DELETE 'FILE'
$ xxx = F$Verify(Verif)
