#!/usr/bin/perl -w
# GemRB - Infinity Engine Emulator
# Copyright (C) 2003 The GemRB Project
#
# This program is free software; you can redistribute it and/or
# modify it under the terms of the GNU General Public License
# as published by the Free Software Foundation; either version 2
# of the License, or (at your option) any later version.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program; if not, write to the Free Software
# Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
#
# $Id$
#

use strict;

my $TGT_DIR = "gemrb";

# TODO: use list of exceptions, i.e. pairs file, boilerplate to use
# 


my $copyright_cpp = <<EOT;
/\\* GemRB \\- Infinity Engine Emulator
 \\* Copyright \\(C\\) 2003 The GemRB Project
 \\*
 \\* This program is free software; you can redistribute it and/or
 \\* modify it under the terms of the GNU General Public License
 \\* as published by the Free Software Foundation; either version 2
 \\* of the License, or \\(at your option\\) any later version\\.

 \\* This program is distributed in the hope that it will be useful,
 \\* but WITHOUT ANY WARRANTY; without even the implied warranty of
 \\* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE\\.  See the
 \\* GNU General Public License for more details\\.

 \\* You should have received a copy of the GNU General Public License
 \\* along with this program; if not, write to the Free Software
 \\* Foundation, Inc\\., 51 Franklin Street, Fifth Floor, Boston, MA 02111\\-1301, USA\\.
 \\*
 \\* \\\$[I]d:.* \\\$
 \\*
 \\*/

/\\*\\*
 \\* \@file .*
 \\* .*
 \\* \@author The GemRB Project
 \\*/
EOT

my $copyright_py = <<EOT;
# \\-\\*\\-python\\-\\*\\-
# GemRB \\- Infinity Engine Emulator
# Copyright \\(C\\) 2003 The GemRB Project
#
# This program is free software; you can redistribute it and/or
# modify it under the terms of the GNU General Public License
# as published by the Free Software Foundation; either version 2
# of the License, or \\(at your option\\) any later version\\.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE\\.  See the
# GNU General Public License for more details\\.
#
# You should have received a copy of the GNU General Public License
# along with this program; if not, write to the Free Software
# Foundation, Inc\\., 51 Franklin Street, Fifth Floor, Boston, MA 02111\\-1301, USA\\.
#
# \\\$[I]d: .* \\\$
EOT



my @copyright_cpp = split (/\n/, $copyright_cpp);
my @copyright_py = split (/\n/, $copyright_py);

my @filetypes = (
		 [ '/\\.#', 'ignore'],
		 [ '~$', 'ignore'],
		 [ '\\.(pyc|lo)$', 'ignore'],
		 [ '\\.(h|cpp)$', \@copyright_cpp ],
		 [ '\\.py$', \@copyright_py ],
		 [ '.', 'ignore' ],
		 );


sub get_filetype {
    my ($filename) = @_;
    foreach my $rec (@filetypes) {
	my $re = $$rec[0];
	if ($filename =~ /$re/) {
	    return $$rec[1];
	}
    }
    return undef;
}

sub check_file {
    my ($filename) = @_;

    my $copyright = &get_filetype ($filename);
    if (! defined ($copyright)) {
	print "? $filename\n";
	return;
    }

    if ($copyright eq 'ignore') {
	return;
    }

    my $num_lines = scalar (@{$copyright});


    open (SRC, "< $filename") || die "Can't open file $filename: $!\n";

    my $index = 0;
    while (defined (my $line = <SRC>) && ($index < $num_lines)) {
	chomp($line);
	my $cline = $$copyright[$index];
	if ($line !~ /^$cline$/) {
	    $cline =~ tr/\\//d;
	    print "! $filename\n";
	    print "  -: $cline\n";
	    print "  +: $line\n";
	    print "\n";
	    last;
	}
	$index++;
    }

    if ($index >= $num_lines) {
	print "= $filename\n";
    }

    close (SRC);
}

sub check_dir {
    my ($dir) = @_;
    local (*DIR);

    opendir (DIR, $dir) || die "Can't open dir  $dir: $!\n";
    while (defined (my $file = readdir (DIR))) {
	my $dirfile = "$dir/$file";
	if (-f $dirfile) {
	    &check_file ($dirfile);
	} elsif (-d $dirfile && substr($file, 0, 1) ne '.') {
	    &check_dir ($dirfile);
	}
    }
    closedir (DIR);
}


if (scalar (@ARGV) != 0) {
    $TGT_DIR = $ARGV[0];
}

&check_dir ($TGT_DIR);
