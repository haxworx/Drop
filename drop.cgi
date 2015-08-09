#! /usr/bin/perl

use strict;
use warnings;
use CGI;

sub Error {
	my ($error) = @_;

	print "Content-type: text/plain\r\n\r\n";
	print "STATUS: $error\r\n";

	exit 1 << $error;
}

sub browser_message {
	my ($msg) = @_;

	print "Content-type: text/plain\r\n\r\n";
	print "STATUS: 0x0002\r\n";
	
	exit;
}

sub get_file {
	my $data = "";

        if ($ENV{'REQUEST_METHOD'} eq "POST") {
                my $len = $ENV{CONTENT_LENGTH};

                if (! defined $len) {
                        browser_message("nopey");
                }

                read(STDIN, $data, $len);


	my $cgi = CGI->new();
	my $username = $cgi->http('Username');
	my $password = $cgi->http('Password');
	my $filename = $cgi->http('Filename');
	my $action = $cgi->http('Action');

	if (!defined $action || ! defined $username || ! defined $password || ! defined $filename)
	{
		Error(0x0003);
	}
                if (! -e $username && ! -d $username) {
                        mkdir($username, 0755);
                }
                our $SLASH = '/';

                my $path = $username . $SLASH . $filename;
		if ($action eq "ADD") {
		open (FH, "> $path") or die "$!";
		print FH $data;
		close FH;
		} elsif ($action eq "DEL") {
			unlink($path);
		}
		print "Content-type: text/plain\r\n\r\n";
		print "STATUS: 0x0001\r\n";
	} else {
		browser_message("nope");
	}
}

get_file();

exit 0;
