<?php
// Set title 
$uri = urldecode(array_pop(explode('/', $_SERVER['REQUEST_URI'])));
$pageTitle = strtoupper($_SERVER['SERVER_NAME']) . " | "; 
if (preg_match("/voter\.php\?node=(\d+)$/", $uri, $matches)) {
    $pageTitle .= "Voter " . $matches[1];
} elseif (preg_match("/link\.php\?node=(\d+)$/", $uri, $matches)) {
    $pageTitle .= "Node " . $matches[1];
} elseif (preg_match("/link\.php\?group=(.*)$/", $uri, $matches)) {
    $pageTitle .= $matches[1];
} elseif (preg_match("/about/", $uri, $matches)) {
    $pageTitle .= ucfirst($matches[0]);
}
?>
<!DOCTYPE html>
<html>
<head>
<title><?php echo $pageTitle; ?></title>
<meta http-equiv="Content-Type" content="text/html; charset=utf-8">
<meta name="generator" content="By hand with a text editor">
<meta name="description" content="Allstar node manager">
<meta name="keywords" content="allstar monitor, app_rpt, asterisk">
<meta name="author" content="Tim Sawyer, WD6AWP">
<link type="text/css" rel="stylesheet" href="allmon.css">
<link type="text/css" rel="stylesheet" href="http://ajax.googleapis.com/ajax/libs/jqueryui/1.8/themes/base/jquery-ui.css">
<script type="text/javascript" src="jquery.js"></script>
<script type="text/javascript" src="http://ajax.googleapis.com/ajax/libs/jqueryui/1.8/jquery-ui.min.js"></script>
<script type="text/javascript" src="jquery.cookie.js"></script>
<script type="text/javascript" src="allmon.js"></script>
</head>
<body>
<div id="header">
<div id="headerTitle">Allstar Monitor</div>
<div id="headerTag">Monitoring the World One Node at a Time</div>
<div id="headerImg"><img src="allstarLogo.png" alt="Allstar Logo"></div>
</div>
<?php include "menu.php"; ?>
