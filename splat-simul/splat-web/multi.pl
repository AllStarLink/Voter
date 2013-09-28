#!/usr/bin/perl
use CGI qw/:standard/;
use DBI;


require "/var/www/html/SPLAT/DB.pl";

#
$callsign1=param("callsign1");
$callsign2=param("callsign2");
$callsign3=param("callsign3");
$callsign4=param("callsign4");

$plotparams = param("plot");
$frequency = param("frequency");

$polarization = param("polarization");

if ($polarization =~ m/vertical/ic) {
    $polarization = 1;
}
if ($polarization =~ m/horizontal/ic) {
    $polarization = 0;
}

$targetheight=param("targetheight");

$plotparams = $plotparams . "  $targetheight";

#$plotparams = $plotparams . "  -R 50.0";


if ($plotparams =~ /L/ic) {
    logit("$callsign1 $callsign2 $callsign3 $callsign4 multi LR",$frequency);
} else {
   logit("$callsign1 $callsign2 $callsign3 $callsign4 multi LOS", $frequency);
}


my ($server, $sock, $host);

$host = "localhost";

$dbh = DBI->connect("DBI:mysql:$database:$server","$dbuser","$password") || die "can't connect";


if (! $dbh) {
    print h1("No database");
  exit(0);
}


$PROC = "$$";
$DIR= "/var/www/html/SPLAT/PLOTS/$PROC";

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
$dbh->disconnect();



print header();
print start_html();


# now we need to create several  temporary QTH files - one for each callsign
$PROC = "$$";

# write a qth file for each station
$CITYLIST = "";
$BOUNDARYLIST = "";
$QTHLIST = "";


$counter = 0;
foreach $call  ($callsign1, $callsign2,$callsign3,$callsign4) {
    next if ($call =~ m/empty/ic); # skip empty calls
    $filenumber = $NUMBER{$call};
    while (length($filenumber) < 2) {
	$filenumber = "0$filenumber";
    }
    $QTHFILE="$DIR/$call.qth";
    $QTHLIST = "$QTHLIST $call";
    if ( $CITYLIST !~ m/pl$filenumber/) {
	$CITYLIST = "$CITYLIST $SPLATHOME/Cities/pl$filenumber*";
	$BOUNDARYLIST = "$BOUNDARYLIST $SPLATHOME/Boundaries/co$filenumber*";
    }
#    push(@rmlist,"$QTHFILE");

    open(QTH,">$QTHFILE");
    printf(QTH "$call\n");
    printf(QTH "$LAT{$call}\n");
    printf(QTH "$LONG{$call}\n");
    printf (QTH "$HGT{$call}\n");
    close(QTH);
    # be sure the qth file has the right mode
    chmod (0744, "$QTHFILE");

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
	    $ERP{$call}        #  Transmitter Effective Radiated Power in Watts (optional)
    );
	   
    open(LRP,">$DIR/$call.lrp");
    my $zed = 0;
    while ($zed < @LRP) {
	printf (LRP "$LRP[$zed]\n");
	$zed++;
    }
    close(LRP);
# permissions!
    chmod (0744, "$DIR/$call.lrp");



}


printf("<h1>Hang on!</h1><br><h2>This can take a VERY long time!</h2><br>\n");

$PPMFILE="$PROC.ppm";

$PNGFILE="$PROC.jpg";
$PNG1FILE="$PROC.png";
$THUMBFILE="$PROC"."_thumb.jpg";

# I need to do some keep-alives here somehow


#print "(cd $DIR;/usr/local/bin/splat -t $QTHLIST -d $SPLATHOME/SDF/$GEOG{$callsign1} $plotparams -f $frequency -s $call_locationfile $CITYLIST  -b $BOUNDARYLIST -dsf -o $PPMFILE)\n";
`(cd $DIR;/usr/local/bin/splat -t $QTHLIST -d $SPLATHOME/SDF/$GEOG{$callsign1} $plotparams -f $frequency -s $call_locationfile $CITYLIST  -b $BOUNDARYLIST -dsf -o $PPMFILE)>/dev/null`;
`convert $DIR/$PPMFILE $DIR/$PNGFILE`;
`convert $DIR/$PPMFILE $DIR/$PNG1FILE`;
`convert -resize 360x240 $DIR/$PNGFILE $DIR/$THUMBFILE`;
chmod (0744, "$DIR/$PNGFILE");
chmod (0744, "$DIR/$PNG1FILE");
chmod (0744, "$DIR/$THUMBFILE");

printf("<center>\n");
printf("<a href=\"PLOTS/$PROC/$PNGFILE\"><img src=\"PLOTS/$PROC/$THUMBFILE\"><br>Click for Larger Image</a>\n");
printf("<br><br><form action=\"simul.cgi\" method=\"post\" target=\"_blank\">\n");

foreach $call  ($callsign1,$callsign2,$callsign3,$callsign4) {
    next if ($call =~ m/empty/ic); # skip empty calls
    printf("$call Launch Delay (uSecs) <INPUT TYPE=\"text\" name=\"$DIR/$call\" value=\"0\"><BR>\n");
}
printf("<INPUT TYPE=\"submit\" value=\"Get Simulcast Plot\">\n</FORM>\n");
printf("</center>\n");




## now we need to do the site reports - perhaps while we are waiting for graphics ?

# site report
foreach $call  ($callsign1, $callsign2,$callsign3,$callsign4) {
    next if ($call =~ m/empty/ic); # skip empty calls
    printf("<h1>Site Report</h1>\n");
    $SITEREPORT="$DIR/$call-site_report.txt";
    printf("<pre>\n");
    open(SITEREPORT,"<"."$SITEREPORT") || die "no site report file";
    while ($line = <SITEREPORT>) {
	chomp($line);
	printf("$line\n");
    }
    close(SITEREPORT);
#    push(@rmlist,"$SITEREPORT");
    printf("</pre>\n");
    printf("<hr>\n");
    
}


# remove these files
push(@rmlist,"$PPMFILE");

unlink(@rmlist);


print end_html();
    


