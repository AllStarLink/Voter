#!/usr/local/bin/perl

use CGI qw/:standard/;
use DBI;

require "DB.pl";


$dbh = DBI->connect("DBI:mysql:$database:$server","$dbuser","$password") || die "can't connect";

if (! $dbh) {
    print h1("No database");
  exit(0);
}

printf("Content-type: text/html\n\n");
printf("<html><head>\n");

printf("</head><body>\n");

print "<script language=\"javascript\">
function suggestName( level )
{
if ( isNaN( level ) ) { level = 1 }

var f = document.theSource;
var listbox = f.callsign;
var textbox = f.measure_name;

var soFar = textbox.value.toString();
var soFarLeft = soFar.substring(0,level).toUpperCase();
var matched = false;
var suggestion = '';
for ( var m = 0; m < listbox.length; m++ ) {
suggestion = listbox.options[m].text.toString();
suggestion = suggestion.substring(0,level).toUpperCase();
if ( soFarLeft == suggestion ) {
listbox.options[m].selected = true;
matched = true;
break;
}
}
if ( matched && level < soFar.length ) { level++; suggestName(level) }
}
</script>
";

print "<script language=\"javascript\">
function suggestName1( level )
{
if ( isNaN( level ) ) { level = 1 }

var f = document.theSource;
var listbox = f.callsign1;
var textbox = f.measure_name1;

var soFar = textbox.value.toString();
var soFarLeft = soFar.substring(0,level).toUpperCase();
var matched = false;
var suggestion = '';
for ( var m = 0; m < listbox.length; m++ ) {
suggestion = listbox.options[m].text.toString();
suggestion = suggestion.substring(0,level).toUpperCase();
if ( soFarLeft == suggestion ) {
listbox.options[m].selected = true;
matched = true;
break;
}
}
if ( matched && level < soFar.length ) { level++; suggestName1(level) }
}
</script>
";

printf("<h1>Profile Plots</h1>\n");

printf("Profile  plots involve two stations, one at each end of a circuit.  Return to the <a href='index.html'>main page</a>
for other types of plots.<br>\n");
printf("<hr>\n");


printf("One station is considered the transmitter, and the other station is the receiver.\n");


    
printf("Here is a list of stations in our database - pick the ones you are interested 
in.\n");

printf("You can search incrementally for a station by typing  partial calls in the empty text boxes.\n");

printf("If a station is not in this list, you must return to the <a href='index.html'>
main page</a> and follow the 'Registration Link'<br>");


printf ("<form method='post' action='profiles.pl' name='theSource'>\n");

my $tabledata = $dbh->prepare("select callsign from calls order by callsign asc");

$tabledata->execute();

while (@fields = $tabledata->fetchrow_array) {
    push (@calls,"$fields[0]");
}
$tabledata->finish();
$dbh->disconnect();


printf("<center>\n");
printf("<table border=5 cellpadding=3 padding=3 bgcolor='yellow'><tr><th>Transmitting Station</th><th>Receiving Station</th><th>Frequency of Interest</th></tr>\n");
printf("<tr>\n");
# first column
printf("<td align='center'>\n");
printf("<input type='text' name='measure_name' onKeyUp='suggestName();' >\n");
printf("<select name='callsign' size=3
onClick='javascript:theSource.measure_name.value=this.options[options.selectedIndex].text;'>\n");
$i = 0;
while ($i < @calls) {
    printf("<option value='$calls[$i]'> $calls[$i]</option>\n");
    $i++;
}
printf("</select></td>\n");

# second column
printf("<td align='center'>\n");
printf("<input type='text' name='measure_name1' onKeyUp='suggestName1();' >\n");
printf("<select name='callsign1' size=3
onClick='javascript:theSource.measure_name1.value=this.options[options.selectedIndex].text;'>\n");
$i = 0;
while ($i < @calls) {
    printf("<option value='$calls[$i]'> $calls[$i]</option>\n");
    $i++;
}
printf("</select></td>\n");
printf("<td>(20.0 mhz to 20000 mhz)  : <input type='text' width=6 name='frequency' value='146.0'></td>\n");
printf("<td>\n");
printf("Antenna Polarization<br> <input type='radio' checked='checked' name='polarization' value='vertical'> Vertical<br>\n");
printf("<input type='radio'  name='polarization' value='horizontal'>Horizontal<br>\n");
printf("</td>\n");


printf("</tr>\n");
printf("<tr>\n");
printf("<td colspan=4>Do you want a topo map ? <input type='radio' checked='checked' name='domap' value='no'> No thanks!  or \n");
printf("<input type='radio' name='domap' value='yes'> Yes, please!</td></tr>\n");
printf("</table>\n");

printf("</center>\n");


printf("\n<br><hr><br>\n");

printf("<center>  <input type=\"reset\" value=\"Start Over\">
  <input type=\"submit\" value=\"Create Plots\"><br></center>
</form>
    ");

print end_html();

