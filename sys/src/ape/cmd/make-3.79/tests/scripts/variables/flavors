#                                                                    -*-perl-*-

$description = "Test various flavors of make variable setting.";

$details = "";

open(MAKEFILE, "> $makefile");

# The Contents of the MAKEFILE ...

print MAKEFILE <<'EOF';
foo = $(bar)
bar = ${ugh}
ugh = Hello

all: multi ; @echo $(foo)

multi: ; $(multi)

x := foo
y := $(x) bar
x := later

nullstring :=
space := $(nullstring) $(nullstring)

next: ; @echo $x$(space)$y

define multi
@echo hi
@echo there
endef

ifdef BOGUS
define
@echo error
endef
endif

EOF

# END of Contents of MAKEFILE

close(MAKEFILE);

# TEST #1
# -------

&run_make_with_options($makefile, "", &get_logfile);
$answer = "hi\nthere\nHello\n";
&compare_output($answer, &get_logfile(1));

# TEST #2
# -------

&run_make_with_options($makefile, "next", &get_logfile);
$answer = "later foo bar\n";
&compare_output($answer, &get_logfile(1));

# TEST #3
# -------

&run_make_with_options($makefile, "BOGUS=true", &get_logfile, 512);
$answer = "$makefile:23: *** empty variable name.  Stop.\n";
&compare_output($answer, &get_logfile(1));


1;
