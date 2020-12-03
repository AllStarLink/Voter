<?php 
include "header.php";

$node = @trim(strip_tags($_GET['node']));
$nodeURL = "http://stats.allstarlink.org/nodeinfo.cgi?node=$node";

// If no node number use first non-voter in INI
if (empty($node)) {
    die ("Please provide voter node number. (ie voter.php?node=1234)");
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

?>
<script>
    // prevent IE caching
    $.ajaxSetup ({  
        cache: false  
    });    

    // when DOM is ready
    $(document).ready(function() {
        var ajax_request;
        
        // Ajax display
        function updateVoter( ) {            
            if(typeof ajax_request !== 'undefined') {
                // Abort slow requests
                ajax_request.abort();
            }
            ajax_request = $.ajax( { url:'voterserver.php', data: { 'node' :  <?php echo $node; ?>}, type:'get', success: function(result) {
                    $('#link_list').html(result);
                }
            });

        }
        
        // Go and repeat every 0.35 seconds.
        updateVoter();
        setInterval(updateVoter, 350);
    });            
</script>
<h2>
<?php 
    if (empty($info)) {
        print "Voter <a href='$nodeURL' target=_blank>$node</a>"; 
    } else {
        print "Voter <a href='$nodeURL' target=_blank>$node</a> $info";
    }
?>
</h2>
<div id="link_list">Loading voter.</div>
<div style='width:500px; text-align:left;'>
The numbers indicate the relative signal strength. The value ranges from 0 to 255, a range of approximately 30db.
A value of zero means that no signal is being received. The color of the bars indicate the type of RTCM client.
</div>
<div style='width: 240px; text-align:left; position: relative; left: 160px;'>
<div style='background-color: #0099FF; color: white; text-align: center;'>A blue bar indicates a voting station.</div>
<div style='background-color: greenyellow; color: black; text-align: center;'>Green indicates the station is voted.</div>
<div style='background-color: cyan; color: black; text-align: center;'>Cyan is a non-voting mix station. </div>
</div>
<?php include "footer.php"; ?>