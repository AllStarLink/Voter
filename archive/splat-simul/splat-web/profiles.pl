#!/usr/bin/perl
use CGI qw/:standard/;
use DBI;

require "/var/www/html/SPLAT/DB.pl";


$callsign=param("callsign");

$callsign1=param("callsign1");

$frequency = param("frequency");

$domap = param("domap");

$polarization = param("polarization");

if ($polarization =~ m/vertical/ic) {
    $polarization = 1;
}
if ($polarization =~ m/horizontal/ic) {
    $polarization = 0;
}




if ($domap =~ m/yes/ic) {
    logit("$callsign to $callsign1 profile with topomap", $frequency);
} else {
    logit ("$callsign to $callsign1 profile", $frequency);
}


$PROC=$$;
$DIR= "PLOTS/$PROC";

mkdir($DIR);
chmod (0755,$DIR);

$call_locationfile = "$SPLATHOME/call_locations.txt";


$dbh = DBI->connect("DBI:mysql:$database:$server","$dbuser","$password") || die "can't connect";

if (! $dbh) {
    print h1("No database");
  exit(0);
}

print header();
print start_html();

printf("<h1>This will take a second ...</h1>\n");

my $tabledata = $dbh->prepare("select callsign, latitude, longitude, height, number, geography from calls, states where calls.state = states.name  order by callsign asc");

$tabledata->execute();

while (@fields = $tabledata->fetchrow_array) {
    $LAT{$fields[0]} = $fields[1];
    $LONG{$fields[0]} = $fields[2];
    $HGT{$fields[0]} = $fields[3];
    $NUMBER{$fields[0]} = $fields[4];
    $GEOG{$fields[0]} = $fields[5];
}

$tabledata->finish();
$dbh->disconnect();





$PROC = "$$";


# search the database for this callsign - if found, edit the file, and if
#  not found, add this data to the database.  In either event,
#  create "$QTHFILE" with the data in it.



# now we need to create a temporary QTH file
# write a qth file for this station, and then add the data to the callsign_0 file
$QTHFILE="$callsign.qth";
$QTHFILE1="$callsign1.qth";

# now we need to create a temporary QTH file
# write a qth file for this station, and then add the data to the callsign_0 file

open(QTH,">$DIR/$QTHFILE");
printf(QTH "$callsign\n");
printf(QTH "$LAT{$callsign}\n");
printf(QTH "$LONG{$callsign}\n");
printf (QTH "$HGT{$callsign}\n");
close(QTH);

open(QTH,">$DIR/$QTHFILE1");
printf(QTH "$callsign1\n");
printf(QTH "$LAT{$callsign1}\n");
printf(QTH "$LONG{$callsign1}\n");
printf (QTH "$HGT{$callsign1}\n");
close(QTH);

chmod(0744,"$DIR/$QTHFILE");
chmod(0744,"$DIR/$QTHFILE1");

my @LRP = (
	    15.000,     #  Earth Dielectric Constant (Relative permittivity)
	    0.005,      #  Earth Conductivity (Siemens per meter)
	    301.000,    #  Atmospheric Bending Constant (N-Units)
	    $frequency, # Frequency in MHz (20 MHz to 20 GHz)
	    5,          #  Radio Climate
	    $polarization,          #  Polarization (0 = Horizontal, 1 = Vertical)
	    0.50,       #  Fraction of situations
	    0.50,       #  Fraction of time
	    0.00        #  Transmitter Effective Radiated Power in Watts (optional)
    );
	   
open(LRP,">$DIR/$callsign.lrp");
my $zed = 0;
while ($zed < @LRP) {
    printf (LRP "$LRP[$zed]\n");
    $zed++;
}
close(LRP);
chmod (0744, "$DIR/$callsign.lrp");


open(LRP,">$DIR/$callsign1.lrp");
my $zed = 0;
while ($zed < @LRP) {
    printf (LRP "$LRP[$zed]\n");
    $zed++;
}
close(LRP);
chmod (0744, "$DIR/$callsign1.lrp");

    

# be sure the qth file has the right mode
chmod (0744, "$call_locationfile");


# all of these plots can generate png files, and they are small enough
# just to put them on a page without thumbnails

$NHPNGFILE="$PROC"."_NH.png";
`(cd $DIR;/usr/local/bin/splat -f $frequency -t $QTHFILE -r $QTHFILE1  -d /var/www/html/SPLAT/SDF/$GEOG{$callsign}  -H $NHPNGFILE)>/dev/null`;
chmod (0744, "$DIR/$NHPNGFILE");

    
$PathLossPNGFILE="$PROC"."_PathLoss.png";
# path loss profile between two stations
`(cd $DIR;/usr/local/bin/splat -f $frequency -t $QTHFILE -r $QTHFILE1 -d /var/www/html//SPLAT/SDF/$GEOG{$callsign}  -l $PathLossPNGFILE)>/dev/null`;
chmod (0744, "$DIR/$PathLossPNGFILE");

$TOPOPPM="$PROC"."_Topo.ppm";
$TOPOPNG="$PROC"."_Topo.jpg";
$TOPOTHUMB="$PROC"."_topo_thumb.jpg";

if ($domap =~ m/yes/ic) { # do topo map

# we need to select the right boundaries and cities files - could be in two states!
    $filenumber = $NUMBER{$callsign};
    while (length($filenumber) < 2) {
	$filenumber = "0$filenumber";
    }

    $filenumber1 = $NUMBER{$callsign1};
    while (length($filenumber1) < 2) {
	$filenumber1 = "0$filenumber1";
    }


    if ($filenumber != $filenumber1) {# two states involved
	$parmstring="/var/www/html//SPLAT/Cities/pl$filenumber* /var/www/html/SPLAT/Cities/pl$filenumber1* -b /var/www/html/SPLAT/Boundaries/co$filenumber* /var/www/html/SPLAT/Boundaries/co$filenumber1* ";
	
    } else { # one state with two stations
	$parmstring="/var/www/html/SPLAT/Cities/pl$filenumber*  -b /var/www/html/SPLAT/Boundaries/co$filenumber* ";
    }

# path loss profile between two stations - could be in different states!
    `(cd $DIR;/usr/local/bin/splat -f $frequency -t $QTHFILE -r $QTHFILE1 -s $call_locationfile $parmstring -d /var/www/html//SPLAT/SDF/$GEOG{$callsign} -o $TOPOPPM)>/dev/null`;
    `convert $DIR/$TOPOPPM $DIR/$TOPOPNG`;
    `convert -resize 360x240 $DIR/$TOPOPNG $DIR/$TOPOTHUMB`;
    chmod (0744, "$DIR/$TOPOPNG");
    chmod (0744, "$DIR/$TOPOTHUMB");
    chmod (0744, "$DIR/$TOPOPPM");
    unlink("$DIR/$TOPOPPM");
}
    
printf("<center>\n");
printf("<img src=\"$DIR/$NHPNGFILE\"><br>\n");
printf("Normalized Height Profile ($callsign on right)<br>\n");
printf("<img src=\"$DIR/$PathLossPNGFILE\"><br>\n");
printf("Path Loss Profile ($callsign on right)<br>\n");
if ($domap =~ m/yes/ic) {
    printf("<a href=\"$DIR/$TOPOPNG\"><img src=\"$DIR/$TOPOTHUMB\"><br>Click here for larger map</a><br>\n");
    printf("Topo Map of Path Between Stations\n");
}

printf("</center>\n");

# site report
printf("<h1>Site Report</h1>\n");
$SITEREPORT="$DIR/$callsign-site_report.txt";
printf("<pre>\n");
open(SITEREPORT,"<"."$SITEREPORT") || die "no site report file";
while ($line = <SITEREPORT>) {
    chomp($line);
    printf("$line\n");
}
close(SITEREPORT);

printf("</pre>\n");
#push(@rmlist,"$SITEREPORT");

# site-to-target report
printf("<h1>Site to Target Report</h1>\n");
$SITEREPORT="$DIR/$callsign"."-to-"."$callsign1".".txt";
printf("<pre>\n");
open(SITEREPORT,"<"."$SITEREPORT") || die "no site report file";
while ($line = <SITEREPORT>) {
    chomp($line);
    printf("$line\n");
}
close(SITEREPORT);
printf("</pre>\n");
#push(@rmlist,"$SITEREPORT");


# remove these files
#push(@rmlist,"$QTHFILE");
#push(@rmlist,"$THUMBFILE");

unlink(@rmlist);


print end_html();
    


