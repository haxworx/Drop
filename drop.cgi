#! /usr/bin/perl

use strict;
use warnings;

sub browser_message {
	my ($msg) = @_;

	print "Content-type: text/plain\r\n\r\n";
	print "STATUS: 0x0002\r\n";
	
	exit;
}

sub get_file {
	if ($ENV{REQUEST_METHOD} eq "POST") {
		my $len = $ENV{CONTENT_LENGTH};

		if (! defined $len) {
			browser_message("nopey");
		}

		my $data = "";

		read(STDIN, $data, $len);

		open(FH, "> example.txt") or die "$!";
		print FH $data;
		close FH;			


		print "Content-type: text/plain\r\n\r\n";
		print "STATUS: 0x0001\r\n";
	} else {
		browser_message("nope");
	}
}

get_file();

exit 0;
