$ Verif = F$Verify(0)
$ ! OpenVMS command file to emulate behavior of:
$ !
$ !      Appending a string to the end of a new or existing file
$ !
$ OPEN="OPEN"
$ WRITE="WRITE"
$ CLOSE="CLOSE"
$ IF F$SEARCH("''P1'") .EQS. "" THEN OPEN/WRITE TEXT_FILE 'P1'
$ IF F$SEARCH("''P1'") .NES. "" THEN OPEN/APPEND TEXT_FILE 'P1'
$ IF P2 .NES. "" THEN WRITE TEXT_FILE "''P2'"
$ IF P3 .NES. "" THEN WRITE TEXT_FILE "''P3'"
$ IF P4 .NES. "" THEN WRITE TEXT_FILE "''P4'"
$ IF P5 .NES. "" THEN WRITE TEXT_FILE "''P5'"
$ IF P6 .NES. "" THEN WRITE TEXT_FILE "''P6'"
$ IF P7 .NES. "" THEN WRITE TEXT_FILE "''P7'"
$ IF P8 .NES. "" THEN WRITE TEXT_FILE "''P8'"
$ CLOSE TEXT_FILE
$ xxx = F$Verify(Verif)
