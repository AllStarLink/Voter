#!/usr/bin/perl

# this script allows registration of a call sign, with other info, in
# postgres database

use CGI qw/:standard/;
use DBI;

use lib ("/var/www/html/SPLAT");
require "DB.pl";

$dbh = DBI->connect("DBI:mysql:$database:$server","$dbuser","$password") || die "can't connect";

print header();
print start_html();

if (! $dbh) {
    print h1("No database");
  exit(0);
}

my $tabledata = $dbh->prepare("select * from calls order by callsign asc");

$tabledata->execute();

# Number of rows returned?
$numRows = $tabledata->rows;

printf("<center>\n");

printf("<h1>Registered Stations</h1>\n");
printf("<table border=5 cellpadding=2 padding=2>\n");
printf("<tr><th>Call</th><th>Latitude</th><th>Longitude</th><th>Antenna Height</th><th>ERP (Watts)</th><th>State</th><th>Date and Time</th><th>Region</th><th>IP</th></tr>\n");
printf("<tr>\n");

while (@fields = $tabledata->fetchrow_array) {
  $i = 0;
  while ($i < @fields) {
    printf("<td align='center'>$fields[$i]</td> ");
    $i++;
    
  }
  printf("</tr>\n");

}

printf("</table>\n");

printf("</center>\n");


$tabledata->finish();
$dbh->disconnect();


print end_html();

