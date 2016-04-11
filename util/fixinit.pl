#!/usr/bin/perl -w

use strict;
use Getopt::Long;

my $file_list;
my $dry_run = 0;

GetOptions("list-file|l=s"		=> \$file_list,
		   "dry-run|d"			=> \$dry_run)
	or die("Error in command line arguments\n");

my @files = @ARGV;

if (defined($file_list)) {
	die("Unable to open file: " . $file_list) if (!open(FLIST, $file_list));

	while (<FLIST>) {
		chomp;
		push(@files, $_);
	}
	close(FLIST);
}

my $matched = 0;

foreach my $ifile (@files) {
	my $ofile = $ifile . "._tmp_";
	my $result = process_file($ifile, $ofile);

	if ($result > 0) {
		print "File match: " . $ifile . "\n";
		$matched++;

		rename($ofile, $ifile) if (!$dry_run);
	} else {
		print "File mismatch: " . $ifile . "\n";
	}
}
print "Matched " . $matched . " file(s) out of " . scalar(@files) . "\n";

exit(0);


sub process_file {
	my ($ifile, $ofile) = @_;

	if (!open(ISF, $ifile)) {
		print STDERR "Unable to open file: " . $ifile . "\n";
		return -1;
	}

	my @lines = <ISF>;

	close(ISF);

	my $num_lines = scalar(@lines);

	while ($num_lines > 0) {
		last if ($lines[$num_lines - 1] !~ /^\s*[\r\n]*$/);
		$num_lines--;
	}

	my $matched = 0;
	my $indev = -1;
	my @labels = (
	        '.dc',
		'.name',
		'.reset',
		'.init',
		'.shutdown',
		'.attach',
		'.walk',
		'.stat',
		'.open',
		'.create',
		'.close',
		'.read',
		'.bread',
		'.write',
		'.bwrite',
		'.remove',
		'.wstat',
		'.power',
		'.config',
		'.zread',
		'.zwrite',
		);

	for (my $i = 0; $i < $num_lines; $i++) {
		my $ln = $lines[$i];

		next if ($ln =~ /^\s*[\r\n]*$/);
		if ($ln =~ /^\s*Dev\s+.*tab.*\{/) {
			$indev = 0;
		} elsif ($indev >= 0) {
			if ($ln =~ /^\s*\};/ || $ln =~/^\s*\./) {
				$indev = -1;
			} elsif ($ln =~ /^\s*([^\s].*)$/){
				$lines[$i] = "\t" . $labels[$indev] . " = " . $1 . "\n";
				$indev++;
				$matched++;
			}
		}
	}
	if (!$dry_run) {
		if ($matched) {
			die("Unable to create file: " . $ofile) if (!open(OSF, ">" . $ofile));

			for (my $i = 0; $i < $num_lines; $i++) {
				print OSF $lines[$i];
			}

			close(OSF);
		}
	}

	return $matched;
}


