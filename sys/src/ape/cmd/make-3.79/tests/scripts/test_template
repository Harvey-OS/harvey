$description = "The following test creates a makefile to ...
                     <FILL IN DESCRIPTION HERE> ";

$details = "<FILL IN DETAILS OF HOW YOU TEST WHAT YOU SAY YOU ARE TESTING>";

# IF YOU NEED >1 MAKEFILE FOR THIS TEST, USE &get_tmpfile; TO GET
# THE NAME OF THE MAKEFILE.  THIS INSURES CONSISTENCY AND KEEPS TRACK OF
# HOW MANY MAKEFILES EXIST FOR EASY DELETION AT THE END.
# EXAMPLE: $makefile2 = &get_tmpfile;

open(MAKEFILE,"> $makefile");

# The Contents of the MAKEFILE ...

print MAKEFILE " <FILL IN THE CONTENTS OF THE MAKEFILE HERE>  \n";

# END of Contents of MAKEFILE

close(MAKEFILE);


# Run make.  You may specify a makefile, but if you don't want to, just
# insert "" where $make_filename is now.  You may also specify specific
# options to run make with, but you also don't have to. (Insert "" where it 
# says <FILL IN OPTIONS HERE>), The last field in this subroutine call
# is the code which is returned from make.  If you think that make should
# execute with no errors, you may OPTIONALLY put 0; Otherwise put the 
# error code that you expect back from make for this test.

# Every time you run make, you just need to say &get_logfile and that
# subroutine will get a new logfile name for you in incrementing order
# according to how many times you call it within ONE test.  It is 
# reset to 0 at the beginning of every new test script.

&run_make_with_options($makefile,
                       "<FILL IN OPTIONS HERE>",
                       &get_logfile,
                       0);


# THE REST OF THIS FILE DEPENDS HIGHLY ON WHAT KIND OF TEST YOU ARE
# CREATING, SO IT WILL VARY.  BASICALLY, YOU MAY INSERT ANYTHING YOU 
# WISH AT THIS POINT TO SEE IF THE TEST WORKED OK.  IF THERE ARE 
# ADDITIONAL TESTS BESIDES &compare_output, AND IT FAILES, YOU
# MUST *** SET $test_passed = 0 !!! ***

# Create the answer to what should be produced by this Makefile
$answer = "<INSERT ANSWER HERE>";

# COMPARE RESULTS

# In this call to compare output, you should use the call &get_logfile(1)
# to send the name of the last logfile created.  You may also use
# the special call &get_logfile(1) which returns the same as &get_logfile(1).

&compare_output($answer,&get_logfile(1));

# If you wish to &error ("abort
") if the compare fails, then add a "|| &error ("abort
")" to the
# end of the previous line.

# This tells the test driver that the perl test script executed properly.
1;






