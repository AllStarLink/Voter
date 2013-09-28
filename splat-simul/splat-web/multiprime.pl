#!/usr/bin/perl

use CGI qw/:standard/;
use DBI;

require "/var/www/html/SPLAT/DB.pl";



printf("Content-type: text/html\n\n");
printf("<html><head>\n");

printf("</head><body>\n");

print "<script language=\"javascript\">
function suggestName4( level )
{
if ( isNaN( level ) ) { level = 1 }

var f = document.theSource;
var listbox = f.callsign4;
var textbox = f.measure_name4;

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
if ( matched && level < soFar.length ) { level++; suggestName4(level) }
}
</script>
";

print "<script language=\"javascript\">
function suggestName3( level )
{
if ( isNaN( level ) ) { level = 1 }

var f = document.theSource;
var listbox = f.callsign3;
var textbox = f.measure_name3;

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
if ( matched && level < soFar.length ) { level++; suggestName3(level) }
}
</script>
";

print "<script language=\"javascript\">
function suggestName2( level )
{
if ( isNaN( level ) ) { level = 1 }

var f = document.theSource;
var listbox = f.callsign2;
var textbox = f.measure_name2;

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
if ( matched && level < soFar.length ) { level++; suggestName2(level) }
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
print h1("Multiple Transmitter Plots");

$dbh = DBI->connect("DBI:mysql:$database:$server","$dbuser","$password") || die "can't connect";

if (! $dbh) {
    print h1("No database");
  exit(0);
}



    
printf("Here is a list of stations in our database - pick up to four you are interested 
in.\n");

printf("If a station is not in this list, you must return to the <a href='index.html'>
main page</a> and follow the 'Registration Link'<br>");
printf("You may use the empty textboxes to incrementally search for callsigns.\n");

printf("These plots can take several minutes to complete.\n");

printf ("<form method='post' action='multi.pl' name='theSource'>\n");

my $tabledata = $dbh->prepare("select callsign from calls order by callsign asc");

$i = 0;
$tabledata->execute();
while (@fields = $tabledata->fetchrow_array()) {
    $CALL[$i] = $fields[0];
    $i++;
}


printf("<center>\n");
printf("\n\n<hr><table cellspacing=10 bgcolor='yellow' cellpadding=5 border=5><tr><th colspan=4>Select up to four transmitter sites:</th></tr>");

printf("<tr>\n");
printf("<td align='center'>\n");


$i = 0;
printf("<input type='text' name='measure_name1' onKeyUp='suggestName1();' >\n");
printf("<select name='callsign1' size=3
onClick='javascript:theSource.measure_name1.value=this.options[options.selectedIndex].text;'>\n");
printf("<option value='empty' selected='selected'>empty</option> ");
while ($i < @CALL) {
   printf("<option value='$CALL[$i]'> $CALL[$i]</option> ");
   $i++;
}
printf("</td>\n");
printf("<td align='center'>\n");
$i = 0;
printf("<input type='text' name='measure_name2' onKeyUp='suggestName2();' >\n");
printf("<select name='callsign2' size=3
onClick='javascript:theSource.measure_name2.value=this.options[options.selectedIndex].text;'>\n");
printf("<option value='empty' selected='selected'>empty </option> ");
while ($i < @CALL) {
   printf("<option value='$CALL[$i]'> $CALL[$i]</option> ");
   $i++;
}
printf("</select>\n");
printf("</td>\n");

printf("<td align='center'>\n");
$i = 0;
printf("<input type='text' name='measure_name3' onKeyUp='suggestName3();' >\n");
printf("<select name='callsign3' size=3
onClick='javascript:theSource.measure_name3.value=this.options[options.selectedIndex].text;'>\n");
printf("<option value='empty' selected='selected'>empty </option> ");
while ($i < @CALL) {
   printf("<option value='$CALL[$i]'> $CALL[$i]</option> ");
   $i++;
}
printf("</select>\n");
printf("</td>\n");

printf("<td align='center'>\n");
$i = 0;
printf("<input type='text' name='measure_name4' onKeyUp='suggestName4();' >\n");
printf("<select name='callsign4' size=3
onClick='javascript:theSource.measure_name4.value=this.options[options.selectedIndex].text;'>\n");
printf("<option value='empty' selected='selected'>empty </option> ");
while ($i < @CALL) {
   printf("<option value='$CALL[$i]'> $CALL[$i]</option> ");
   $i++;
}
printf("</select>\n");
printf("</td>\n");

printf("</tr>\n");

# other stuff in next rows
printf("<tr>\n");

printf("<td >Frequency of Operation<br> (20.0 mhz to 20000 mhz): <input type='text' width=4 name='frequency' value='146.0'></td>\n");
printf("<td >Target's antenna height<br>(feet AGL): <input type='text' width=4 name='targetheight' value='5.0'></td>\n");
printf("<td >");
printf("Antenna Polarization<br> <input type='radio' checked='checked' name='polarization' value='vertical'> Vertical<br>\n");
printf("<input type='radio'  name='polarization' value='horizontal'>Horizontal<br>\n");
printf("</td>\n");
printf("<td bgcolor='cyan'>\n");
printf("Type of Plot <br>\n");
printf("<input type=\"radio\" name=\"plot\" value=\"-c \"> Line of Sight Coverage<br>\n");
printf("<input type=\"radio\" checked=\"checked\" name=\"plot\" value=\"-L \"> Longley-Rice Path Loss Analysis<br>\n");

printf("</td>\n");

printf("</tr>\n");
printf("</table>\n");

printf("</center>\n");

$tabledata->finish();
$dbh->disconnect();



print "<br><center>  <input type=\"reset\" value=\"Start Over\">
  <input type=\"submit\" value=\"Create a Plot\"><br>
</center>
</form>
    ";

print end_html();

