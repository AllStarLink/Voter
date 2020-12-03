#!/usr/bin/perl

use CGI qw/:standard/;
use DBI;

use lib ("/var/www/html/SPLAT");
require "DB.pl";

print header();

$dbh = DBI->connect("DBI:mysql:$database:$server","$dbuser","$password") || die "can't connect";
#$dbh->{RaiseError} = 1;
if (! $dbh) {
    print h1("No database");
  exit(0);
}

my $tabledata = $dbh->prepare("SELECT callsign FROM calls ORDER BY callsign asc") or die "Couldn't select " . $dbh->errstr ."<br >\n";

$tabledata->execute() or die "Execute failed";

# Number of rows returned?
$numRows = $tabledata->rows;

printf("<html><head>\n");


printf("</head><body>\n");

    
printf("Here is a list of stations in our database - pick the one you are interested 
in.\n");

printf("You can search incrementally for a station by typing a partial call into the empty text box.\n");

printf("If a station is not in this list, you must return to the <a href='index.html'>
main page</a> and follow the 'Registration Link'<br>");

printf("We are assuming
that you are talking to mobile stations, and therefore you are using a vertically
polarized antenna, with your target station's antenna at 5.0 feet above the ground,
and a frequency of 146.0 mhz.  You can change these below if you like.<br>\n");


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



printf ("<form method='post' action='coverage.pl' name='theSource'>\n");


printf("<center>\n");
printf("\n\n<hr><table cellspacing = 10 cellpadding=5 bgcolor='yellow' border=3><tr><td>Select A Callsign:<br> ");

printf("<input type='text' name='measure_name' onKeyUp='suggestName();' ><br>\n");

printf("<select name='callsign' size=3
onClick='javascript:theSource.measure_name.value=this.options[options.selectedIndex].text;'>\n");
while (@fields = $tabledata->fetchrow_array()) {
 print join(", ", @fields);
    printf("<option value='$fields[0]'> $fields[0]</option> ");
}
printf("</select></td>\n");
printf("<td>Frequency of Operation (20.0 mhz to 20000 mhz)  : <input type='text' width=4 name='frequency' value='146.0'></td>\n");
printf("</tr>\n");
printf("<tr><td align='center' >Target's antenna height (feet AGL) : <input type='text' width=4 name='targetheight' value='5.0'></td>\n");
printf("<td>\n");
printf("Antenna Polarization<br> <input type='radio' checked='checked' name='polarization' value='vertical'> Vertical<br>\n");
printf("<input type='radio'  name='polarization' value='horizontal'>Horizontal<br>\n");
printf("</td>\n");
printf("</tr>\n");
printf("</table>\n");
printf("</center>\n");

$tabledata->finish();
$dbh->disconnect();


printf("\n<br><hr><br>\n");

print "What type of plot do you want ?<br>
<center>
<table border=3 bgcolor='cyan'>
  <tr><td><input type=\"radio\" name=\"plot\" value=\"-c \"></td><td> Line of Sight Coverage</td></tr>
  <tr><td><input type=\"radio\" checked=\"checked\" name=\"plot\" value=\"-L \"></td><td> Longley-Rice Path Loss Analysis</td></tr>
</table>
</center>
<hr>


<center>  <input type=\"reset\" value=\"Start Over\">
  <input type=\"submit\" value=\"Create a Plot\"><br>
</center>
</form>
    ";

print end_html();

