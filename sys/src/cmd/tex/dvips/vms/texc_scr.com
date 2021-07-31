$!----------------------------------------------------------------------
$!
$!                            TEXC_SCR.COM
$!
$!  Convert TEX.LPRO to TEXC.LPRO
$!  This command file replaces the TexC.script file, intended for Unix OS,
$!  with its TPU equivalent.
$!
$!  Tony McGrath    5-OCT-1990
$!  Dept. of Physics, Monash University, Victoria, Australia 3168
$!
$!----------------------------------------------------------------------
$!
$ Edit/TPU/NoSection/NoDisplay/Command=SYS$Input/Output=TEXC.LPRO TEX.LPRO
!
PROCEDURE texc$script

LOCAL string_1, string_2, string_3, a_range, a_line, two_chars;

!-----------------------------------------------------------------------
! Won't bother with CREATE_ARRAY, try to keep the TPU as basic as possible
! so it works on older versions of VMS.
!-----------------------------------------------------------------------
string_1 := "% begin code for uncompressed fonts only";
string_2 := "% end code for uncompressed fonts only";
string_3 := "% end of code for unpacking compressed fonts";
!-----------------------------------------------------------------------
! Search for the first of the 3 special strings.
! Exit if we can't find it.
!-----------------------------------------------------------------------
a_range  := search( string_1, forward);
if( a_range = 0)
then
  message( "TEXC-F-NoString, Couldn't locate first string, Aborting");
  return(0);
endif;
!-----------------------------------------------------------------------
! Go to the start of the first string.
!-----------------------------------------------------------------------
position( beginning_of( a_range));
!-----------------------------------------------------------------------
! Search for the second of the 3 special strings.
! Exit if we can't find it.
!-----------------------------------------------------------------------
a_range  := search( string_2, forward);
if( a_range = 0)
then
  message( "TEXC-F-NoString, Couldn't locate second string, Aborting");
  return(0);
endif;
!-----------------------------------------------------------------------
! Then start deleting lines until the second special string is found.
!-----------------------------------------------------------------------
loop
  a_line := erase_line;
  exitif a_line = string_2;
endloop;
!-----------------------------------------------------------------------
! Search for the third of the 3 special strings.
! Exit if we can't find it.
!-----------------------------------------------------------------------
a_range  := search( string_3, forward);
if( a_range = 0)
then
  message( "TEXC-F-NoString, Couldn't locate third string, Aborting");
  return(0);
endif;
!-----------------------------------------------------------------------
! Again start looping, deleting the first 2 characters from each line
! until the 3rd special string is found, making sure that the first two
! characters are "% "
!-----------------------------------------------------------------------
loop
  two_chars := erase_character(2);
  if two_chars <> "% "
  then
    message( "TEXC-F-NoComment, First 2 chars not correct, Aborting");
    return(0);
  endif;
  exitif current_line = string_3;
  move_vertical(1);
endloop;
!-----------------------------------------------------------------------
! Assume all is well, return TRUE.
!-----------------------------------------------------------------------
return(1);

ENDPROCEDURE
!-----------------------------------------------------------------------
! Initialize the main buffer.
!-----------------------------------------------------------------------
f:=Get_Info(Command_Line,"File_Name");
b:=Create_Buffer("",f);
o:=Get_Info(Command_Line,"Output_File");
Set (Output_File,b,o);
Position (Beginning_of(b));
!
if texc$script
then
  Exit;
else
  message( "TEXC-W-NoSave, current buffer not saved, errors were encountered");
  Quit;
endif;
