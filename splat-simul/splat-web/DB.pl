# This is included in each of the files which connects to the database

my ($server, $sock, $host);
$host = "localhost";

# The name of the server
$server = "localhost";

# the name of the database itself
$database = "splat";

# the name of a database user who has administrative authority
# over the $database
$dbuser = "root";

# mysql password of this administrative user
$password = "rst599";

# the home of the web pages
$SPLATHOME="/var/www/html/SPLAT";

############# nothing to change below here

sub logit {
    my $event =$_[0];
    my $freq = $_[1];
# fix up the current time to form a date-time group

    my $dbh2 = DBI->connect("DBI:mysql:$database:$server","$dbuser","$password") || die "can't connect";

if (! $dbh2) {
    print h1("No database");
  exit(0);
}

    my @timearray = localtime();
    my $year = $timearray[5] + 1900;
    my $month = $timearray[4] + 1;
    
    while (length($month) < 2 ) {
	$month = "0$month";
    }
    
    while (length($timearray[3]) < 2 ) {
	$timearray[3] = "0$timearray[3]";
    }
    
    while (length($timearray[2]) < 2 ) {
	$timearray[2] = "0$timearray[2]";
    }
    while (length($timearray[1]) < 2 ) {
	$timearray[1] = "0$timearray[1]";
    }
    while (length($timearray[0]) < 2 ) {
	$timearray[0] = "0$timearray[0]";
    }
    
    
    my $dtgroup = "$year-$month-$timearray[3] $timearray[2]:$timearray[1]:$timearray[0]";
    my $q_dtgroup = $dbh2->quote($dtgroup);

    my $q_event = $dbh2->quote($event);

    my $IP = $ENV{'REMOTE_ADDR'};
    my $q_IP = $dbh2->quote($IP);

    
    my $string = "INSERT INTO log (datetime, event, frequency,  IP)  VALUES ($q_dtgroup,$q_event, $freq, $q_IP)";

    my $tabledata = $dbh2->do("$string");

    $dbh2->disconnect();

    

}



1;


