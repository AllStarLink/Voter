<?php
// Reads output lines from Asterisk Manager
function get_response($fp) {
    $response = '';
    while (TRUE) {
        $str = fgets($fp, 1024);
        if (strlen(trim($str)) != 0 ) {
            $response .= $str;
        } else {
            return($response);
        }
    }
}

function connect($host) {
    // Set default port if not provided
    $arr = split(":", $host);
    $ip = $arr[0];
    if (isset($arr[1])) {
        $port = $arr[1];
    } else {
        $port = 5038;
    }
    
    // Open a manager socket.
    $i = 1;
    $fp = @fsockopen($ip, $port);
    if (!$fp) {
        die("Could not connect.\n");
    }
    return ($fp);
}

function login($fp, $user, $password) {
    // Login
    fwrite($fp,"ACTION: LOGIN\r\nUSERNAME: $user\r\nSECRET: $password\r\nEVENTS: 0\r\n\r\n");
    $login = get_response($fp);
    if (!preg_match('/Response: Success.*Message: Authentication accepted/s', $login)) {
        die("Login failed!\n");
    }
}


?>