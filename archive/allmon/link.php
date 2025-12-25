<?php 
include "header.php";

$node = @trim(strip_tags($_GET['node']));
$group = @trim(strip_tags($_GET['group']));
$nodeURL = "http://stats.allstarlink.org/nodeinfo.cgi?node=$node";

if (empty($node) AND empty($group)) {
    die ("Please provide node number or group name. (ie link.php?node=1234 | link.php?group=name)");
}

// Type = group or node for server.php?
if (!empty($group)) {
    $type = 'group';
    $node = $group;
} else {
    $type = 'node';
}

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

if (array_key_exists($node, $astdb)) {
    $nodeRow = $astdb[$node];
    $info = $nodeRow[4] . ' ' . $nodeRow[5] . ' ' . $nodeRow[6];
}

// Build a list of nodes in the group
$nodes = array();
if (!empty($group)) {
    // Read Groups INI file
    if (!file_exists('groups.ini')) {
        die("Couldn't load group ini file.\n");
    }
    $gconfig = parse_ini_file('groups.ini', true);
    
    $group = $_GET['group'];
    $nodes = split(",", $gconfig[$group]['nodes']);
}
#print_r($nodes); print "$type $node";

?>
<script type="text/javascript">
    // prevent IE caching
    $.ajaxSetup ({  
        cache: false  
    });    

    // when DOM is ready
    $(document).ready(function() {
        var ajax_request;
        
        // Ajax display
        function updateServer( ) {
            if(typeof ajax_request !== 'undefined') {
                ajax_request.abort();
            }
            ajax_request = $.ajax( { url:'server.php', data: { '<?php echo $type; ?>' : '<?php echo $node; ?>'}, type:'get', success: function(result) {
                    $('#link_list').html(result);
                }
            });
        }
        
        // Go and repeat every 1 second.
        updateServer();
        setInterval(updateServer, 1000);

    });
</script>
<h2>
<?php 
    if ($type == 'node') {
        if (empty($info)) {
            print "Node <a href='$nodeURL' target='_blank'>$node</a>"; 
        } else {
            print "Node <a href='$nodeURL' target='_blank'>$node</a> $info";
        }
    } else {
        print "$group";
    }
?>
</h2>

<!-- Connect form -->
<div id="connect_form">
<?php 
if (count($nodes) > 0) {
    print "<select id=\"localnode\">";
    foreach ($nodes as $node) {
        print "<option value=\"$node\">$node</option>";
    }
    print "</select>\n";
} else {
    print "<input type=\"hidden\" id=\"localnode\" value=\"$node\">\n";
}
?>
    <input type="text" id="node">
    Permanent <input type="checkbox">
    <input type="button" value="Connect" id="connect">
</div>

<div id="link_list">Loading...</div>
<?php include "footer.php"; ?>