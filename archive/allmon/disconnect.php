<?php
include('allmon.inc.php');
#print "<pre>"; print_r($_GET); print "</pre>";

$sendCommand = FALSE; 
if (isset($_GET['disconnect']) && isset($_GET['node'])) {
    $localnode = $_GET['node'];
    $node = $_GET['disconnect'];
} else {
    die("Bad \$_GET\n");
}
print "<b>Disconnecting $node</b>";

if (!file_exists('allmon.ini')) {
    die("Couldn't load ini file.\n");
}
$config = parse_ini_file('allmon.ini', true);

// Open a socket to Asterisk Manager
$fp = connect($config[$localnode]['host']);
login($fp, $config[$localnode]['user'], $config[$localnode]['passwd']);

// Do it
if ((@fwrite($fp,"ACTION: COMMAND\r\nCOMMAND: rpt cmd $localnode ilink 11 $node\r\n\r\n")) > 0 ) {
    // Get response, but do nothing with it
    $rptStatus = get_response($fp);
    #print "<pre>===== start =====\n";
    #print_r($rptStatus);            
    #print "===== end =====\n</pre>";
} else {
    die("Command failed!\n");
}
?>