<?php
#print date("Y-m-d H:i:s") . "<br/>\n";
#print microtime() . "<br/>New voter app under development.<br/>\n";
include('allmon.inc.php');

// Read parameters passed to us
$node = $_GET['node'];

// Read config INI file
if (!file_exists('allmon.ini')) {
    die("Couldn't load ini file.\n");
}
$config = parse_ini_file('allmon.ini', true);

#print "<pre>"; print_r($config[$node]); print "</pre>";

// Open a socket to Asterisk Manager
$fp = connect($config[$node]['host']);
login($fp, $config[$node]['user'], $config[$node]['passwd']);

// Get voter response
$response = get_voter($fp);
if ($response === FALSE) {
    die ("Bad voter response!<br/>");
}

$lines = preg_split("/\n/", $response);
#print "<pre>$node: "; print_r($lines); print "</pre>";
$voted = array();
$nodes=array();
foreach ($lines as $line) {
	$line = trim($line);
	if (strlen($line) == 0) {
    	continue;
	}
    list($key, $value) = split(": ", $line);
    $$key = $value;

    if ($key == "Node") {
        $nodes[$Node]=array();
    }
    
    if ($key == "RSSI") {
        $nodes[$Node][$Client]['rssi']=$RSSI;
        $nodes[$Node][$Client]['ip']=$IP;
    }
    
    if ($key == 'Voted') {
        $voted[$Node] = $value;
    } 
}
#print "<pre>\$nodes: "; print_r($nodes);print "</pre>"; 

foreach($nodes as $nodeNum => $clients) {
    // Ready to print, let's go
    print $nodeNum;
    print "<table id='mytable'>
    <tr><td align='center'><b>Client</b></td><td align='center'><b>RSSI</b></td></tr>\n";

    if (count($clients) == 0) {
        print "<td><div style='width: 120px;'>&nbsp;</div></td>\n";
        print "<td><div style='width: 339px;'>&nbsp;</div></td>\n";
    }
    foreach($clients as $clientName => $client) {
        
        $rssi = $client['rssi'];
        $percent = ($rssi/255)*100;
        if ($percent == 0) {
            $percent = 1;
        }

        // find voted if any
        if (@$voted[$nodeNum] != 'none' && strstr($clientName, @$voted[$nodeNum])) {
                $barcolor = 'greenyellow';
                $textcolor = 'black';
        } elseif (strstr($clientName, 'Mix')) {
            $barcolor = 'cyan';
            $textcolor = 'black';
        } else {
            $barcolor = "#0099FF";
            $textcolor = 'white';
        }

        // print table rows
        print "<tr>\n";
        print "<td><div style='width: 120px;'>$clientName</div></td>\n";
        print "<td><div class='text'>&nbsp;<div class='barbox_a'>";
        print "<div class='bar' style='width: " . $percent * 3 . "px; background-color: $barcolor; color: $textcolor'>$rssi</div>";
        print "</div></td>\n</tr>\n";
    }
    print "</table><br/>\n";
}

function get_voter($fp) {

    // Voter status
    if ((@fwrite($fp,"ACTION: VoterStatus\r\n\r\n")) > 0) {
        // Get Voter Status
        return get_response($fp);
    } else {
        return FALSE;
    }
}

#print "<pre>"; print_r($nodes); print "</pre>";
?>