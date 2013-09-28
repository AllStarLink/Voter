#!/usr/bin/perl

use CGI qw/:standard/;
use DBI;

require "/var/www/html/SPLAT/DB.pl";

#

$callsign=param("callsign");
$plotparams = param("plot");
$frequency = param("frequency");
$targetheight=param("targetheight");
$polarization=param("polarization");

if ($polarization =~ m/vertical/ic) {
    $polarization = 1;
}
if ($polarization =~ m/horizontal/ic) {
    $polarization = 0;
}

$plotparams = $plotparams . "  $targetheight";


if ($plotparams =~ m/-L/) {
#    logit("$callsign coverage LR",$frequency);
} else {
#    logit("$callsign coverage LOS", $frequency);
}



$dbh = DBI->connect("DBI:mysql:$database:$server","$dbuser","$password") || die "can't connect";

if (! $dbh) {
    print h1("No database");
  exit(0);
}

$PROC = "$$";
$DIR= "$SPLATHOME/PLOTS/$PROC";

mkdir($DIR);
chmod (0755,$DIR);

$call_locationfile = "$SPLATHOME/call_locations.txt";




my $tabledata = $dbh->prepare("select callsign, latitude, longitude, height, number, geography, erp from calls, states where calls.state = states.name  order by callsign asc");

$tabledata->execute();

while (@fields = $tabledata->fetchrow_array() ) {
    $LAT{$fields[0]} = $fields[1];
    $LONG{$fields[0]} = $fields[2];
    $HGT{$fields[0]} = $fields[3];
    $NUMBER{$fields[0]} = $fields[4];
    $GEOG{$fields[0]} = $fields[5];
    $ERP{$fields[0]} = $fields[6];
}


$tabledata->finish();



print header();
print start_html();

$QTHFILE="$DIR/$callsign.qth";

# now we need to create a temporary QTH file
# write a qth file for this station, and then add the data to the callsign_0 file
open(QTH,">$QTHFILE");
printf(QTH "$callsign\n");
printf(QTH "$LAT{$callsign}\n");
printf(QTH "$LONG{$callsign}\n");
printf (QTH "$HGT{$callsign}\n");
close(QTH);

# I also need a .lrp file for this call $callsign

my @LRP = (
	    15.000,     #  Earth Dielectric Constant (Relative permittivity)
	    0.005,      #  Earth Conductivity (Siemens per meter)
	    301.000,    #  Atmospheric Bending Constant (N-Units)
	    $frequency, # Frequency in MHz (20 MHz to 20 GHz)
	    5,          #  Radio Climate
	    $polarization,          #  Polarization (0 = Horizontal, 1 = Vertical)
	    0.50,       #  Fraction of situations
	    0.50,       #  Fraction of time
	    $ERP{$callsign}        #  Transmitter Effective Radiated Power in Watts (optional)
    );
	   
open(LRP,">$DIR/$callsign.lrp");
my $zed = 0;
while ($zed < @LRP) {
    printf (LRP "$LRP[$zed]\n");
    $zed++;
}
close(LRP);
# permissions!
chmod (0744, "$DIR/$callsign.lrp");




# be sure the qth file has the right mode
    chmod (0744, "$QTHFILE");
    

# be sure the call_locatiotns file has the right mode
chmod (0744, "$call_locationfile");



# we need to select the right boundaries and cities files
$filenumber = $NUMBER{$callsign};
while (length($filenumber) < 2) {
    $filenumber = "0$filenumber";
}





printf("<h1>Hang on!</h1><br><h2>This takes four or five  minutes!</h2><br>\n");

$PPMFILE="$PROC.ppm";
$PNGFILE="$PROC.jpg";
$KMLFILE="$PROC.kml";
$THUMBFILE="$PROC"."_thumb.jpg";
#`(cd $DIR;/usr/local/bin/splat -t $callsign -d /var/www/html/SPLAT/SDF/$GEOG{$callsign} $plotparams -f $frequency -s $call_locationfile /var/www/html/SPLAT/Cities/pl$filenumber*  -b /var/www/html/SPLAT/Boundaries/co$filenumber* -o $PPMFILE)>/dev/null`;
`(cd $DIR;/usr/local/bin/splat -t $callsign -d /var/www/html/SPLAT/SDF/$GEOG{$callsign} $plotparams -f $frequency -kml $KMLFILE -o $PPMFILE)>/dev/null`;

printf("(cd $DIR;splat -t $callsign -d /var/www/html/SPLAT/SDF/$GEOG{$callsign} $plotparams -f $frequency -s $call_locationfile /var/www/html/SPLAT/Cities/pl$filenumber*  -b /var/www/html/SPLAT/Boundaries/co$filenumber* -o $PPMFILE)>/dev/null\n");
`convert $DIR/$PPMFILE $DIR/$PNGFILE`;
`convert -resize 360x240 $DIR/$PNGFILE $DIR/$THUMBFILE`;
chmod (0744, "$DIR/$PNGFILE");
chmod (0744, "$DIR/$THUMBFILE");

    

printf("<center>\n");
printf("<a href=\"PLOTS/$PROC/$PNGFILE\"><img src=\"PLOTS/$PROC/$THUMBFILE\"><br>Click for Larger Image</a>\n");
printf("</center>\n");

# site report
printf("<h1>Site Report</h1>\n");
$SITEREPORT="$DIR/$callsign-site_report.txt";
chmod(0766,$SITEREPORT);
printf("<pre>\n");
open(SITEREPORT,"<"."$SITEREPORT") || die "no site report file";
while ($line = <SITEREPORT>) {
    chomp($line);
    printf("$line\n");
}
close(SITEREPORT);

push(@rmlist,"$SITEREPORT");
push(@rmlist,"$DIR/$PPMFILE");

printf("</pre>\n");


# remove these files
unlink(@rmlist);

print end_html();

$dbh->disconnect();



