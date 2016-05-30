#!/usr/bin/perl

use strict;
use warnings;

package Content;

use Template;
use CGI qw/:standard/;
use Digest::MD5 qw/md5_hex/;
use CGI::Cookie;
use DBI;

our $DATABASE_NAME = "hive";
our $DATABASE_USER = "hive";
our $DATABASE_PASS = "hive";
our $SLASH = '/';

sub new {
	my ($class) = shift;
	my $self = {
		dbh => dbh_connect(),
		cgi => CGI->new(),
		sql_data => undef,
		message => undef,
		template_name => "default",
		fields => {
			username => undef,
			password => undef,
			action => undef, 
		},
		credentials => {
			authenticated => 0,
			session_id => 0,
		},
	};

	return bless $self, $class;
}

sub credentials {
	my ($self) = @_;
	return $self->{credentials};
}
sub fields {
	my ($self) = @_;
	return $self->{fields};
}

sub cgi {
	my ($self) = @_;
	return $self->{cgi};
}

sub dbh {
	my ($self) = @_;
	return $self->{dbh};
}

sub dbh_connect {
	my ($self) = shift;
        my $dsn = "DBI:mysql:database=$DATABASE_NAME:host=localhost";
        my $dbh = DBI->connect($dsn, $DATABASE_USER, $DATABASE_PASS) or die "DBH ERROR";

        return $dbh;
}

sub fail {
	my ($self, $mesg) = @_;
	print "Content-type: text/plain\r\n\r\n";
	print "Error: $mesg\r\n";
	exit(1);
}

sub fields_get {
	my $self = shift;
	if (! defined $self->fields) { return 0; }
	foreach (%{$self->fields}) {
		if (! defined $_ ) { next; }
		$self->fields->{$_} = $self->cgi->param($_); 
	}	
	return 1;
}

sub sql_update {
	my $self = shift;
	my $SQL = "SELECT * FROM users";

	my $sth = $self->dbh->prepare($SQL);
	$sth->execute();	

	my @rows = ();

	while (my $row = $sth->fetchrow_hashref()) {
		push @rows, $row;
	}

	$self->{'sql_data'} = \@rows;

	return \@rows;
}

sub output {
	my ($self, $type) = @_;
	use Template;

	my $template = Template->new();

	my $template_path = "templates" . $SLASH . $self->{template_name};
	if (! -e $template_path) {
		$self->fail("process template path! $template_path");
	}

	my $vars = {
		SQL_DATA => $self->{'sql_data'},
		MESSAGE => $self->{'message'},
	};

	print "Content-type: $type\r\n\r\n";
	$template->process($template_path, $vars) or
		die "template process!";

	$self->dbh->disconnect();
}


our $SECRET = "JesusIsLord";

sub create_cookie {
        my ($self, $username, $password) = @_;

        my $secret = "$username:$SECRET:$password";

        my $cookie_value = md5_hex($secret);

        my $time = time();

        my $expires = $time + 60 * 60;

        my $cookie = $self->cgi->cookie( -name  => 'auth',
					 -value => $cookie_value);

        my $last_cookie = $self->cgi->cookie( -name  => 'username',
					      -value => $username);

        print "Set-Cookie: $cookie\n";
        print "Set-Cookie: $last_cookie\n";
}

sub check_cookie {
        my ($self, $cookie, $username) = @_;

	my $SQL = "SELECT * FROM admin WHERE username = ?";
	my $sth = $self->dbh->prepare($SQL);
	$sth->execute($username);
	my $user = $sth->fetchrow_hashref();
	if (! defined $user) { return 0; };
	my $password = $user->{'password'};

        my $secret = "$username:$SECRET:$password";

        $secret = md5_hex($secret);

        if ($cookie eq $secret) {
                return 1;
        } else {
                return 0;
        }
}

sub authenticated {
	my $self = shift;
	return $self->credentials->{'authenticated'};
}

sub authenticate {
	my ($self) = shift;

	my $cookie = $self->cgi->cookie("auth");
	my $username = $self->cgi->cookie("username");
	
	if (! defined($cookie)) {
		my $user_guess = $self->fields->{'username'};
		my $pass_guess = $self->fields->{'password'};

		my $SQL = "SELECT * from admin WHERE username = ?";
		my $sth = $self->dbh->prepare($SQL);
		$sth->execute($user_guess);

		my $admin = $sth->fetchrow_hashref();
		if (! defined($admin)) {
			$self->credentials->{'authenticated'} = 0;
			$self->{template_name} = "login_page";
			return;
		}
		if ($user_guess eq $admin->{'username'} && $pass_guess eq $admin->{'password'}) {
			$self->credentials->{'authenticated'} = 1;
			$self->create_cookie($user_guess, $pass_guess);	
		}
	} elsif ($self->check_cookie($cookie, $username)) {
		$self->credentials->{'authenticated'} = 1;
	}

	if (! $self->credentials->{'authenticated'}) {
		$self->{template_name} = "login_page";
	}

	return $self->credentials->{'authenticated'};
}

sub user_exists {
	my ($self) = shift;
	my $SQL = "SELECT * from users WHERE username = ?";
	my $sth = $self->dbh->prepare($SQL);
	$sth->execute($self->fields->{'username'});
	
	return $sth->fetchrow_hashref() || undef;	
} 

sub user_add {
	my ($self) = shift;

	if ($self->fields->{'username'} eq "" || $self->fields->{'password'} eq "") {
		$self->{'message'} = "empty user or pass";
		return;
	}

	if ($self->fields->{'username'} =~ /\s/) {
		$self->{'message'} = "invalid characters in username";
		return;
	}
	if ($self->fields->{'username'} !~ /[A-Za-z0-9]+/) {
		$self->{'message'} = "invalid characters in username";
		return;
	}

	if ($self->user_exists) {
		$self->{'message'} = "username exists!";
		return;
	}

	$self->{'message'} = "username added!";

	my $SQL = "INSERT into users (username, password, active) VALUES (?, ?, 1)";
	my $sth = $self->dbh->prepare($SQL);
	return $sth->execute($self->fields->{'username'}, $self->fields->{'password'});
}

sub admin_password {
	my ($self) = shift;

	if ($self->fields->{'password'} eq "") {
		$self->{'message'} = "missing fields";
		return 0;
	}

	my $SQL = "UPDATE admin SET password = ? WHERE username = ?";
	my $sth = $self->dbh->prepare($SQL);

	$self->{'message'} = "admin password updated";

	return $sth->execute($self->fields->{'password'}, "admin");
}

sub user_del {
	my $self = shift;

	if (! defined $self->fields->{'username'}) {
		$self->{'message'} = "no user selected";
		return undef;
	}

	my $SQL = "DELETE FROM users WHERE username = ?";
	my $sth = $self->dbh->prepare($SQL);

	$self->{'message'} = "username deleted!";

	return $sth->execute($self->fields->{'username'});
}


sub logout {
	my ($self) = shift;
        my $cookie = $self->{cgi}->cookie( -name => 'auth',
				   -value => '');

	$self->{'message'} = "you logged out!";

        print "Set-Cookie: $cookie\n";
	print $self->cgi->redirect( -uri => "/");

	return 0;
}

package main;

use Switch 'Perl6';

sub main {
	my $result = 0;

	my $content = Content->new();

	$content->fields_get();

	$content->authenticate();

	if ($content->authenticated) {

		given($content->fields->{'action'}) {
			when /^add$/ {  
				$result = $content->user_add(); 
			}
			when /^del$/ {  
				$result = $content->user_del(); 
			}
			when /^pwd$/ { 
				$result = $content->admin_password(); 
			}
			when /^exit$/ {
				$result = $content->logout(); 
			}
		}

		$content->sql_update();
	} 

	$content->output("text/html");	

	return $result;
}

exit(main());

