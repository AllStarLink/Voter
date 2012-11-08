<?php
#print date("Y-m-d H:i:s") . "<br/>\n";
include('allmon.inc.php');
#print "<pre>"; print_r($_GET); print "</pre>";

// Sanity check
if (empty($_GET['node']) && empty($_GET['group'])) {
    die ("No group or node provided.\n");
}

// Read parameters passed to us
$node = @trim(strip_tags($_GET['node']));
$group = @trim(strip_tags($_GET['group']));

// Get Allstar database file
$db = "astdb.txt";
$astdb = array();
if (file_exists($db)) {
    $fh = fopen($db, "r");
    if (flock($fh, LOCK_SH)){
        while(($line = fgets($fh)) !== FALSE) {
            $arr = split("\|", trim($line));
            $astdb[$arr[0]] = $arr;
        }
    }
    flock($fh, LOCK_UN);
    fclose($fh);
}

// Read allmon INI file
if (!file_exists('allmon.ini')) {
    die("Couldn't load ini file.\n");
}
$config = parse_ini_file('allmon.ini', true);
#print "<pre>"; print_r($config); print "</pre>";

// If it's a group build a list of nodes
$nodes = array();
if (!empty($group)) {
    // Read Groups INI file
    if (!file_exists('groups.ini')) {
        die("Couldn't load group ini file.\n");
    }
    $gconfig = parse_ini_file('groups.ini', true);
    
    $group = $_GET['group'];
    $nodes = split(",", $gconfig[$group]['nodes']);
    #print "<pre>"; print_r($nodes); print "</pre>";
    if (count($nodes) > 0) {
        print "<table class=\"gridtable\">\n";
        foreach ($nodes as $node) {
            if (isset($_COOKIE['allmon_loggedin']) && $_COOKIE['allmon_loggedin'] == 'yes') {
                $colspan = 8;
            } else {
                $colspan = 7;
            }
            
            print "<tr><th colspan='$colspan'>Node $node</th></tr>\n";
            // Open a socket to Asterisk Manager
            $fp = connect($config[$node]['host']);
            login($fp, $config[$node]['user'], $config[$node]['passwd']);

            $response = getNode($fp, $node);
            printNode($node, $response);
        }
        print "</table>";
    }
} else {
    // Open a socket to Asterisk Manager
    $fp = connect($config[$node]['host']);
    login($fp, $config[$node]['user'], $config[$node]['passwd']);

    $response = getNode($fp, $node);
    print "<table class='gridtable'>";
    printNode($node, $response);
    print "</table>";
}
exit;

// Get status for this $node
function getNode($fp, $node) {
    
    if ((@fwrite($fp,"ACTION: RptStatus\r\nCOMMAND: XStat\r\nNODE: $node\r\n\r\n")) > 0 ) {
        // Get RptStatus
        $rptStatus = get_response($fp);
        #print "<pre>===== start =====\n";
        #print_r($rptStatus);            
        #print "===== end =====\n</pre>";
    } else {
        die("Get node XStat failed!\n");
    }
    
    // format Conn: Node# isKeyed lastKeySecAgo lastUnkeySecAgo
    if ((@fwrite($fp,"ACTION: RptStatus\r\nCOMMAND: SawStat\r\nNODE: $node\r\n\r\n")) > 0 ) {
        // Get RptStatus
        $sawStatus = get_response($fp);
        #print "<pre>===== \$sawStat start =====\n";
        #print_r($sawStatus);            
        #print "===== end =====\n</pre>";
    } else {
        die("Get node sawSta failed!\n");
    }
    
    // Parse this $node. Retuns a list of currently connected nodes
    $current = parseRptStatus($rptStatus, $sawStatus);
    #print "<pre>===== \$current start =====\n";
    #print_r($current);
    #print "===== end =====\n</pre>";

    // Save $current
    return($current);
}


// Ready to print, let's go
function printNode ($localNode, $connectedNodes) {
    $info = getAstInfo($localNode);
    if (isset($_COOKIE['allmon_loggedin']) && $_COOKIE['allmon_loggedin'] == 'yes') {
        print "<tr>
        <th>Discon</th>
        <th>Node</th>
        <th>Node Information</th>
        <th>Received</th>
        <th>Link</th>
        <th>Direction</th>
        <th>Connected</th>
        <th>Mode</th>
        </tr>\n";
    } else {
        print "<tr>
        <th>Node</th>
        <th>Node Information</th>
        <th>Received</th>
        <th>Link</th>
        <th>Direction</th>
        <th>Connected</th>
        <th>Mode</th>
        </tr>\n";
    }
    
    // skip nodes with no connected links
    if (count($connectedNodes) == 0) {
        if (isset($_COOKIE['allmon_loggedin']) && $_COOKIE['allmon_loggedin'] == 'yes') {
            print "<tr><td colspan=8>No connections.</td></tr>\n</table></br>\n";
        } else {
            print "<tr><td colspan=7>No connections.</td></tr>\n</table></br>\n";
        }
        return;
    } 
    
    // sort nodes by last keyed time
    $sortedNodes = sortNodes($connectedNodes);
    
    foreach($sortedNodes as $nodeNum => $keyed_epoch) {

        $node = $connectedNodes[$nodeNum];

        // Skip long range links.
        if ($node['link'] == "n/a") {
            continue;
        }

        // Make a nice color if receiving
        if ($node['keyed'] == 'yes') {
            $tr = '<tr class="rColor">';
            #$node['last_keyed'] = 0;
        } else {
            $tr = '<tr>';
        }

        // Build last_keyed string
        if ($node['last_keyed'] > -1) {
            $t = $node['last_keyed'];
            $h = floor($t / 3600);
            $m = floor(($t / 60) % 60);
            $s = $t % 60;
            $last_keyed = sprintf("%02d:%02d:%02d", $h, $m, $s);
        } else {
            $last_keyed = "Never";
        }

        // Translate mode
        if ($node['mode'] == 'R') {
            $node['mode'] = 'Rx only';
        } elseif ($node['mode'] == 'T') {
            $node['mode'] = 'Transceive';
        } elseif ($node['mode'] == 'C') {
            $node['mode'] = 'Connecting';
        }

        // Translate link
        if ($node['link'] == "n/a") {
            #continue;
            $node['link'] = "Long Range";
            $last_keyed = '&nbsp;';
            $node['direction'] = '&nbsp;';
            $node['elapsed'] = '&nbsp;';
        }

        // Allstar database
        $info = getAstInfo($node['node'], $node);

        // print table rows
        print "$tr\n";
        if (isset($_COOKIE['allmon_loggedin']) && $_COOKIE['allmon_loggedin'] == 'yes') {
            if ($node['link'] == 'ESTABLISHED') {
                $hrefLink=$localNode . "#" . $node['node'];
                #print "<td align='center'><a href='#' onClick=\"disconnect('$localNode, ". $node['node'] . "')\">X</a></td>\n";
                print "<td align='center'><a href='$hrefLink' class='disconnect'>X</a></td>\n";
                #print "<td align='center'>" . $node['node'] . "' class='disconnect'>X</a></td>\n";
            } else {
                print "<td>&nbsp;</td>\n";
            }
        }
        print "<td>" . $node['node'] . "</td>\n";
        print "<td>$info</td>\n";
        print "<td>$last_keyed</td>\n";
        print "<td>" . $node['link'] . "</td>\n";
        print "<td>" . $node['direction'] . "</td>\n";
        print "<td>" . $node['elapsed'] . "</td>\n";
        print "<td>" . $node['mode'] . "</td>\n";
        print "</tr>\n";
    }
}
#print "<pre>cookie: "; print_r($_COOKIE); print "</pre>";

########## ##########
function sortNodes($nodes) {
    
    #print "<pre>\n"; print_r($nodes); print "</pre>";
    
    $arr=array();
    $never_heard=array();
    
    // build list of heard and unheard
    foreach($nodes as $nodeNum => $row) {
//        if (strtoupper($row['link']) == "ESTABLISHED") {
        if (strtoupper($row['last_keyed']) > -1) {
            $arr[$nodeNum]=$row['last_keyed'];
        } else {
            $never_heard[$nodeNum]=$row['elapsed'] . $row['node'];
        }
    }
    
    // Sort them
    if (count($arr) > 0) {
        asort($arr, SORT_NUMERIC);
    }
    if (count($never_heard) > 0) {
        ksort($never_heard, SORT_NUMERIC);
    }
    
    // Add the never heard calls to the end
    if (count($never_heard) > 0) {
        foreach($never_heard as $nodeNum => $row) {
            $arr[$nodeNum]=$row;
        }
    }
    return ($arr);
}

function getAstInfo($nodeNum, $node=array()) {
    global $astdb;
    #print '<pre>'; print_r($node); print '</pre>';
    
    // Build info string
    if (array_key_exists($nodeNum, $astdb)) {
        $dbNode = $astdb[$nodeNum];
        $info = $dbNode[4] . ' ' . $dbNode[5] . ' ' . $dbNode[6];
    } elseif ($nodeNum > 3000000) {
        $info = "Echolink";
    } elseif (!empty($node['ip'])) {
        if (strlen(trim($node['ip'])) > 3) {
            $info = '(' . $node['ip'] . ')';
        } else {
            $info = '&nbsp;';
        }
    } else {
        $info = '&nbsp;';
    }

    return $info;
}

function parseRptStatus($rptStatus, $sawStatus) {
    #global $savedNodes;

    $curNodes = array();
    $links = array();
    $conns = array();
    $alinks = array();

    // Parse 'rptStat Conn:' lines.
    $lines = split("\n", $rptStatus);
    foreach ($lines as $line) {
        if (preg_match('/Conn: (.*)/', $line, $matches)) {
            $arr = preg_split("/\s+/", trim($matches[1]));
            if(is_numeric($arr[0]) && $arr[0] > 3000000) {
                // no ip when echolink
                $conns[] = array($arr[0], "", $arr[1], $arr[2], $arr[3], $arr[4]);
            } else {
                $conns[] = $arr;
            }
        }
    }
    #print "Conns: ";
    #print_r($conns);

    // Parse 'sawStat Conn:' lines.
    $keyups = array();
    $lines = split("\n", $sawStatus);
    foreach ($lines as $line) {
        if (preg_match('/Conn: (.*)/', $line, $matches)) {
            $arr = preg_split("/\s+/", trim($matches[1]));
            $keyups[$arr[0]] = array('node' => $arr[0], 'isKeyed' => $arr[1], 'keyed' => $arr[2], 'unkeyed' => $arr[3]);
        }
    }
    #print "<pre>====== \$keyups start ======\n"; print_r($keyups); print '====== end ======</pre>'; 
    #print "Conns: ";
    #print_r($conns);

    // Parse 'LinkedNodes:' line.
    if (preg_match("/LinkedNodes: (.*)/", $rptStatus, $matches)) {
        $links = preg_split("/, /", trim($matches[1]));
    }
    #print_r($links);

    // Parse 'RPT_ALINKS' line. Contains keyed information.
    if (preg_match("/Var: RPT_ALINKS=\d+,(.*)/", $rptStatus, $matches)) {
        $alinks = preg_split("/,/", trim($matches[1]));
    }
    #print_r($alinks);

    // Pull above arrays together into $curNodes
    if (count($links) > 0 && $links[0] != "<NONE>") {
        // Long range sensors.
        foreach($links as $node) {
            $mode = substr($node,0,1);
            $nodeNum = substr($node,1);
            $curNodes[$nodeNum]['node'] = $nodeNum;
            $curNodes[$nodeNum]['ip'] = 'n/a';
            $curNodes[$nodeNum]['direction'] = 'n/a';
            $curNodes[$nodeNum]['elapsed'] = 'n/a';
            $curNodes[$nodeNum]['link'] = 'n/a';
            $curNodes[$nodeNum]['keyed'] = 'n/a';
            $curNodes[$nodeNum]['last_keyed'] = 'n/a';
            $curNodes[$nodeNum]['mode'] = $mode;
        }

        // Local connects
        foreach($conns as $node) {
            $curNodes[$node[0]]['node'] = $node[0];
            $curNodes[$node[0]]['ip'] = $node[1];
            $curNodes[$node[0]]['direction'] = $node[3];
            $curNodes[$node[0]]['elapsed'] = $node[4];
            $curNodes[$node[0]]['link'] = @$node[5];
        }

        // Pullin keyed
        foreach($keyups as $node => $arr) {
            if ($arr['isKeyed'] == 1) {
                $curNodes[$node]['keyed'] = 'yes';
            } else {
                $curNodes[$node]['keyed'] = 'no';
            }
            $curNodes[$node]['last_keyed'] = $arr['keyed'];
        }

    }
    
    return $curNodes;
}

?>