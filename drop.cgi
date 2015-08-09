#! /usr/bin/perl

use strict;
use warnings;

sub browser_message {
	my ($msg) = @_;

	print "Content-type: text/plain\r\n\r\n";
	print "STATUS: 0x0002\r\n";
	
	exit;
}

sub cgi_new {
if (($ENV{REQUEST_METHOD}||"") eq "POST") {
        my $post = $ENV{REQUEST_BODY_1};
        if (!defined $post) { $post = ""; }
        my $len = $ENV{CONTENT_LENGTH};
        if (defined $len && ($len -= length($post)) >= 0) {
                read(STDIN, $post, $len, length($post));
        } else {
                $post .= join "", <STDIN>;
        }
        return CGI->new($post);
} else {
        return CGI->new();
}
}

use CGI qw/:standard/;

my $query = cgi_new();

my $username = $query->param("username");
my $password = $query->param("password");
 
my $filename = $query->param("filename");

my $fh = $query->upload("filename");
my $data =""; 

open (FH, "> example.txt") or die "$!";
binmode FH;

print FH "$username and $password\n";
if (!defined $fh) {
	browser_message("nooo");	
}	

while (<$fh>) {
	$data .= $_;
}

print FH $data;
close FH;

print "Content-type: text/plain\r\n\r\n";
print "STATUS: 0x0001\r\n";

exit 0;
