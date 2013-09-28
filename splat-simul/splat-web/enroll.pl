#!/usr/bin/perl

# this script inserts registration of a call sign into postgres database 'splat'


use CGI qw/:standard/;
use DBI;

$callsign=param("callsign");
$callsign =~ tr /[a-z]/[A-Z]/;
$callsign =~ s/ //g; # no spaces

$longitude=param("longitude");

$latitude=param("latitude");

$antenna=param("antenna");
$antenna =~ s/-//;

$state = param("state");
$inserttype=param("inserttype");

$geography = param("geography");

if (length($geography) <= 0 ) {
    $geography = "North_America";
}

$IP = $ENV{'REMOTE_ADDR'};

require "/var/www/html/SPLAT/DB.pl";


print header();

print start_html();

$dbh = DBI->connect("DBI:mysql:$database:$server","$dbuser","$password") || die "can't connect";

if (! $dbh) {
    print h1("No database");
  exit(0);
}

# quote the inputs
$q_callsign = $dbh->quote($callsign);
$q_latitude = $dbh->quote($latitude);
$q_longitude = $dbh->quote($longitude);
$q_antenna = $dbh->quote($antenna);
$q_geography = $dbh->quote($geography);
$q_IP = $dbh->quote($IP);

@timearray = localtime();
$year = $timearray[5] + 1900;
$month = $timearray[4] + 1;

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


$dtgroup = "$year-$month-$timearray[3] $timearray[2]:$timearray[1]:$timearray[0]";
$q_dtgroup = $dbh->quote($dtgroup);




if ($inserttype =~ m/original/ic) {
    $string = "INSERT INTO calls  (callsign, latitude, longitude, height, state, geography, lastmodified, IP) VALUES ($q_callsign,$q_latitude,$q_longitude,$q_antenna,\'$state\', \'$geography\', $q_dtgroup, $q_IP)";
    logit("enroll $callsign","");
} else {
   $string = "update calls set latitude = $q_latitude, longitude = $q_longitude, height = $q_antenna, state=\'$state\', geography= \'$geography\', lastmodified=$q_dtgroup, IP=$q_IP  where callsign = $q_callsign";
   logit("update $callsign","");
}

#printf ("<h1>$string</h1>\n");
$tabledata = $dbh->do("$string");
#$dbh->commit();


#printf("test: $callsign, $latitude, $longitude, $antenna<br>\n");
printf("<h1>This data is entered into the database:</h1>\n");
printf("<table border=5 cellpadding=2 padding=2>\n");
printf("<tr><th>Call</th><th>Latitude</th><th>Longitude</th><th>Antenna Height</th><th>State</th><th>Region</th><th>Date and Time</th><th>IP</th></tr>\n");
printf("<tr><td>$callsign</td><td>$latitude</td><td>$longitude</td><td>$antenna</td><td>$state</td><td>$geography</td><td>$dtgroup</td><td>$IP</td></tr>\n");
printf("<tr>\n");


printf("</table>\n");





## make the call_locations.txt file

open(LOCATIONS, ">" . "call_locations.txt") || die "can't write call_locations.txt";
my $tabledata1 = $dbh->prepare("select callsign, latitude, longitude from calls order by callsign asc");

$tabledata1->execute();

while (@fields = $tabledata1->fetchrow_array()) {
    $call = $fields[0];
    $latitude = $fields[1];
    $longitude = $fields[2];
    printf(LOCATIONS "$call, $latitude, $longitude\n");
}

close(LOCATIONS);

$tabledata1->finish();


$dbh->disconnect();


print h1("Thanks");
printf("<a href='index.html'>Now return to the main page</a>\n");

print end_html();

EOT
