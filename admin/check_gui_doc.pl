#!/usr/bin/perl -w

use strict;
use Digest::MD5 qw(md5_hex);

my $SRCFILE = "../gemrb/plugins/GUIScript/GUIScript.cpp";
my $TGTDIR = "../gemrb/docs/en/GUIScript";

my %fn_hash = ();
my %file_hash = ();
my %desc_hash = ();

my %file_ignore = (
		   'CVS' => 1,
		   'Makefile.am' => 1,
		   'Makefile.in' => 1,
		   'Makefile' => 1,
		   );

sub parse_guiscript_cpp {
    local (*SRC);

    my $fname = '';
    my $desc = '';

    open (SRC, "< $SRCFILE") || die "Can't open $SRCFILE: $!\n";

    while (defined (my $line = <SRC>)) {
	if ($line =~ /^PyDoc_STRVAR\s*\(\s*GemRB_(.*)__doc/g) {
	    $fname = $1;
	}
	elsif ($fname && $line =~ /^\"(.*)\"\s*$/g) {
	    $desc .= $1;
	}
	elsif ($fname && $line =~ /^\"(.*)\"\s*\);\s*$/g) {
	    $desc .= " : $1";
	    my $md5 = md5_hex ($1);
	    $fn_hash{$fname} = $md5;
	    $desc_hash{$fname} = $desc;
	    $fname = '';
	    $desc = '';
	}
    }

    close (SRC);
}

sub parse_doc {
    my ($file) = @_;
    local (*SRC);

    open (SRC, "< $TGTDIR/$file") || die "Can't open $TGTDIR/$file: $!\n";

    my $md5 = '';
    while (defined (my $line = <SRC>)) {
	if ($line =~ /^MD5:\s*([0-9a-f]+)/o) {
	    $md5 = $1;
	    last;
	}
    }

    $file_hash{$file} = $md5;

    close (SRC);
}

&parse_guiscript_cpp ();

opendir (DIR, $TGTDIR) || die "Can't open dir  $TGTDIR: $!\n";
my @files = grep { -f "$TGTDIR/$_" && ! exists ($file_ignore{$_}) } grep !/^\.\.?/, readdir (DIR);
closedir (DIR);

foreach my $f (@files) {
    &parse_doc ($f);
}

foreach my $fn (sort keys %fn_hash) {
    my $md5_1 = $fn_hash{$fn};
    my $file = $fn . ".txt";

    if (exists ($file_hash{$file})) {
	my $md5_2 = $file_hash{$file};

	if ($md5_1 eq $md5_2) {
	    print "= $fn\n";
	} else {
	    print "! $fn: $md5_1 : $md5_2\n";
	}
    }
    else {
	print "+ $fn : $md5_1 : $desc_hash{$fn}\n";
    }
}

foreach my $file (sort keys %file_hash) {
    my $md5 = $file_hash{$file};
    my $fn = $file;
    $fn =~ s/\.[^\.]+$//o;

    if (! exists ($fn_hash{$fn})) {
	print "- $fn : $md5\n";
    }
}
