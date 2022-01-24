#!/usr/bin/perl

# this script allows registration of a call sign, with other info, in
# postgres database


use CGI qw/:standard/;
use DBI;



require "/var/www/html/SPLAT/DB.pl";

$dbh = DBI->connect("DBI:mysql:$database:$server","$dbuser","$password") || die "can't connect";

print header();
print start_html();

if (! $dbh) {
    print h1("No database");
  exit(0);
}

my $tabledata = $dbh->prepare("select * from log order by datetime desc");

$tabledata->execute();

printf("<center>\n");

printf("<h1>Log Book</h1>\n");
printf("<table border=5 cellpadding=2 padding=2>\n");
printf("<tr><th>Event</th><th>Frequency</th><th>Requesting IP</th><th>Date-Time</th></tr>\n");
printf("<tr>\n");

while (@fields = $tabledata->fetchrow_array() ) {
  $i = 1; # skip serial number
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

