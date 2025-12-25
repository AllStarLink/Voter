<?php

/*
 * L O S T -- Logger Of Signal Transitions
 * Version 1.0, 10/24/2015
 *
 * Copyright 2015, Jim Dixon <jim@lambdatel.com>, All Rights Reserved
 *
 * 
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 * 
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston,
 * MA 02110-1301, USA.
 * 
 */

$nchannels = 4;

$link = mysql_connect('localhost', 'root', 'mypassword');
if (!$link) {
    die('Could not connect: ' . mysql_error());
}
print "Connected successfully\n";

if (mysql_select_db("lost") == FALSE) {
    die('Could not select db: ' . mysql_error());
}

# We need to determine the lowest and highest sequence numbers
# to work with. The start sequence number corresponds to the
# last start of the logger program. The end sequence number
# was the last one in the log file when this program started.
# This is all done so that through multiple queries, all
# the accumulated data of various types will all be recerencing
# the same data set, thus allowing this analysis process to
# operate in concurrence with the logging operation.


# Determine the start seq no
$sql = "SELECT MAX(seqno) from log WHERE channel = '-1'";
$result = mysql_query($sql);
if (!$result) {
    die('Could not find any start records!!');
}
if ($row = mysql_fetch_row($result))
{
    // Get starting seqno + 1
    $x = $row[0] + 1;	
}
else
{
    die('Unable to fetch start record information!!');
}


# determine the timestamp of the first entry past the starting row
$sql = "SELECT secs from log WHERE seqno = '$x'";
$result = mysql_query($sql);
if (!$result) {
    die('Could not find any start records!!');
}
if ($row = mysql_fetch_row($result))
{
    // get first recorded second + 1
    $firstsec = floor($row[0]) + 1;	
}
else
{
    die('Unable to fetch start record information!!');
}

# determine the seqno of the entry whose timestamp is in the next second
# past the first entry past the starting row
$sql = "SELECT MIN(seqno) from log WHERE secs >= '$firstsec'";
$result = mysql_query($sql);
if (!$result) {
    die('Could not find any start records!!');
}
if ($row = mysql_fetch_row($result))
{
    $lowseq = $row[0];	
}
else
{
    die('Unable to fetch start record information!!');
}

# Determine the end seq no
$sql = "SELECT MAX(seqno) from log;";
$result = mysql_query($sql);
if (!$result) {
    die('Could not find any records!!');
}
if ($row = mysql_fetch_row($result))
{
    $x = $row[0];	
}
else
{
    die('Unable to fetch end record information!!');
}

# determine the timestamp of the last entry 
$sql = "SELECT secs from log WHERE seqno = '$x'";
$result = mysql_query($sql);
if (!$result) {
    die('Could not find any start records!!');
}
if ($row = mysql_fetch_row($result))
{
    // get last recorded second - 1
    $lastsec = floor($row[0]);	
}
else
{
    die('Unable to fetch end record information!!');
}

# determine the seqno of the entry whose timestamp is in the last second
# before the last entry 
$sql = "SELECT MAX(seqno) from log WHERE secs <= '$lastsec'";
$result = mysql_query($sql);
if (!$result) {
    die('Could not find any start records!!');
}
if ($row = mysql_fetch_row($result))
{
    $hiseq = $row[0];	
}
else
{
    die('Unable to fetch start record information!!');
}

#print "seq is $lowseq ($firstsec), $hiseq ($lastsec)\n";

$nrows = ($hiseq - $lowseq) + 1;
$nsecs = $lastsec - $firstsec;
$x = $nrows / $nsecs;

print "$nrows rows in $nsecs seconds ($x)\n";

$sql = "SELECT COUNT(*) from log WHERE gpsok = '1' AND seqno >= '$lowseq' AND seqno <= '$hiseq'";
$result = mysql_query($sql);
if (!$result) {
    die('Could not find any start records!!');
}
if ($row = mysql_fetch_row($result))
{
    $x = $row[0];	
}
else
{
    die('Unable to fetch start record information!!');
}

$y = (100 * $x) / $nrows;
print "$y percent of entries had good GPS\n";

if ($x < $nrows) // If had GPS "issues"
{
	$sql = "SELECT MAX(seqno) from gps WHERE what = 'STARTUP'";
	$result = mysql_query($sql);
	if (!$result) {
	    die('Could not find any start records!!');
	}
	if ($row = mysql_fetch_row($result))
	{
	    $gpsstart = $row[0];	
	}
	else
	{
	    die('Unable to fetch start record information!!');
	}
	$last = "";
	$sql = "SELECT what,systime from gps WHERE seqno > '$gpsstart' ORDER BY systime";
	$result = mysql_query($sql);
	if (!$result) {
	    die('Could not find any GPS records!!');
	}
	$n = 0;
	$ng = 0;
	while ($row = mysql_fetch_row($result))
	{
		$ok = "INACTIVE";
		if (substr($row[0],0,4) != 'TSIP')
		{
			if (substr($row[0],0,8) != 'GPS STAT') continue;
			$n++;
			if (substr($row[0],15,1) == '1') $ok = "ACTIVE";
			if ($ok == 'ACTIVE') $ng++;
			if ($ok != $last)
			{
				$last = $ok;
				print "*** GPS changed to $last at ".$row[1]."\n";
			}
			continue;
		}
		$n++;
		if (substr($row[0],9,1) == '1') $ng++;
		if ($row[0] != $last) // If a "change" in sequence
		{
			$last = $row[0];
			print "*** GPS changed to $last at ".$row[1]."\n";
			$stx = explode(' ',substr($row[0],20,18));
			for($i = 0; $i < count($stx); $i++)
				$st[$i] = hexdec($stx[$i]);
			if ($st[1] & 16) print "        DAC at rail!!\n";
			if ($st[2] & 16) print "        PPS not generated!!\n";
			if ($st[2] & 8) print "        Almanac not complete!!\n";
			if ($st[2] & 2) print "        Position is questionable!!\n";
			if ($st[2] & 1) print "        In test mode!!\n";
			if ($st[3] & 128) print "        Leap Second Pending (ok)\n";
			if ($st[3] & 64) print "        No stored position (ok)\n";
			if ($st[3] & 32) print "        Survey in progress (ok)\n";
			if ($st[3] & 16) print "        Not Disciplining oscillator\n";
			if ($st[3] & 8) print "        Not tracking satellites\n";
			if ($st[3] & 4) print "        Antenna shorted\n";
			if ($st[3] & 2) print "        Antenna open!!\n";
			if ($st[3] & 1) print "        DAC near rail!!\n";
			switch ($st[4])
			{
			    case 0:
				print "        Doing Fixes (ok)\n";
				break;
			    case 1:
				print "        Don't have GPS time!!\n";
				break;
			    case 3:
				print "        PDOP is too high!!\n";
				break;
			    case 8:
				print "        No usable SATs!!\n";
				break;
			    case 9:
				print "        Only 1 usable SAT!!\n";
				break;
			    case 10:
				print "        Only 2 usable SATs!!\n";
				break;
			    case 11:
				print "        Only 3 usable SATs!!\n";
				break;
			    case 12:
				print "        The chose SAT is unusable!!\n";
				break;
			    case 16:
				print "        TRAIM rejected the fix!!\n";
				break;
			    default:
				print "        Unknown decodng status: ".$st[4]." (decimal)\n";
				break;
			}
			switch ($st[5])
			{
			    case 0:
				print "        Phased locked (ok)\n";
				break;
			    case 1:
				print "        Oscillator warming-up!!\n";
				break;
			    case 2:
				print "        Frequency locking!!\n";
				break;
			    case 3:
				print "        Placing PPS!!\n";
				break;
			    case 4:
				print "        Initializing loop filter!!\n";
				break;
			    case 5:
				print "        Compensating OCXO (holdover)!!\n";
				break;
			    case 6:
				print "        Inactive!!\n";
				break;
			    case 8:
				print "        Recovery Mode!!\n";
				break;
			    case 9:
				print "        Calibration/control voltage!!\n";
				break;
			    default:
				print "        Unknown disciplining activity: ".$st[5]." (decimal)\n";
				break;
			}
		}
	}
	$yg = (100 * $ng) / $n;
	print "\n$yg percent of GPS were \"ok\" ($ng out of $n)\n";	
}

$laststate = "";
for($i = 0; $i < $nchannels; $i++)
	$laststate .= 'X';
$t1 = array();
$t2 = array();
$fs = 0;
$chx = "";
$sql = "SELECT channel,val,secs from log WHERE seqno >= '$lowseq' AND seqno <= '$hiseq' ORDER BY secs";
$result = mysql_query($sql);
if (!$result) {
    die('Could not find any log records!!');
}
while ($row = mysql_fetch_row($result))
{
	$thisstate = substr_replace($laststate,$row[1],$row[0],1);
	$chy = $chx;
	$chx = $laststate."->".$thisstate;
	$ch = $chx."(".$chy.")";
	$laststate = $thisstate;
	if ($fs == 0)
	{
		$fs = floor($row[2]);
		continue;
	}
	else if (floor($row[2]) == $fs)
	{
		continue;
	}
	if (!isset($t1[$ch]))
		$t1[$ch] = 0;
	if (!isset($t2[$ch]))
		$t2[$ch] = 0;
	$t1[$ch]++;	
	$t2[$ch] += $row[2] - floor($row[2]);
}

$avgs = array();

foreach($t1 as $k => $v)
{
	$avgs[$k] = $t2[$k] / $v;
}

print "Number of transitions:\n";
print_r($t1);
print "Average time within each second:\n";
print_r($avgs);

$laststate = "";
for($i = 0; $i < $nchannels; $i++)
	$laststate .= 'X';
$fs = 0;
$res = array();
$tres = array();
$sql = "SELECT channel,val,secs from log WHERE seqno >= '$lowseq' AND seqno <= '$hiseq' ORDER BY secs";
$result = mysql_query($sql);
$lasttime = 0;
$chx = "";
if (!$result) {
    die('Could not find any log records!!');
}
while ($row = mysql_fetch_row($result))
{
	$thisstate = substr_replace($laststate,$row[1],$row[0],1);
	$chy = $chx;
	$chx = $laststate."->".$thisstate;
	$ch = $chx."(".$chy.")";
	$laststate = $thisstate;
	$t = bcsub($row[2],$lasttime,8);
	$lasttime = $row[2];
	if ($fs == 0)
	{
		$fs = floor($row[2]);
		continue;
	}
	else if (floor($row[2]) == $fs)
	{
		continue;
	}
	$d = bcsub($avgs[$ch],bcsub($row[2],floor($row[2]),8),8);
	$k = $ch.":".$d;
	if (!isset($res[$k]))
		$res[$k] = 0;
	$res[$k]++;
	$k = $ch.":".$t;
	if (!isset($tres[$k]))
		$tres[$k] = 0;
	$tres[$k]++;
}

ksort($res);
ksort($tres);
print "\nres:\n";
print_r($res);
print "\ntres:\n";
print_r($tres);

$fp = fopen("time.csv","w");
fprintf($fp,"\"Time\"");
foreach($avgs as $k => $v)
{
	fprintf($fp,",\"%s\"",$k);
}
fprintf($fp,"\n");
foreach($res as $k => $v)
{
	$y = array();
	foreach($avgs as $k1 => $v1)
	{
		$y[$k1] = "";
	}
	$x = explode(':',$k);
	$y[$x[0]] = $v;
	fprintf($fp,"\"%s\"",bcadd($x[1],$avgs[$x[0]],8));
	foreach($avgs as $k => $v)
	{
		fprintf($fp,",\"%s\"",$y[$k]);
	}
	fprintf($fp,"\n");
}
fclose($fp);

$fp = fopen("timediff.csv","w");
fprintf($fp,"\"Time\"");
foreach($avgs as $k => $v)
{
	fprintf($fp,",\"%s\"",$k);
}
fprintf($fp,"\n");
foreach($tres as $k => $v)
{
	$y = array();
	foreach($avgs as $k1 => $v1)
	{
		$y[$k1] = "";
	}
	$x = explode(':',$k);
	$y[$x[0]] = $v;
	fprintf($fp,"\"%s\"",$x[1]);
	foreach($avgs as $k => $v)
	{
		fprintf($fp,",\"%s\"",$y[$k]);
	}
	fprintf($fp,"\n");
}
fclose($fp);

exit;

$fp = fopen("timediff.csv","w");
fprintf($fp,"\"Time\",\"Hits\",\"Trans\"\n");
foreach($tres as $k => $v)
{
	$x = explode(':',$k);
	fprintf($fp,"\"%s\",\"%s\",\"%s\"\n",bcadd($x[1],$avgs[$x[0]],8),$v,$x[0]);
}
fclose($fp);


?>

