<?php
    $MAC = "9cb654c5f074";
    if(!isset($_GET['srv'])) die("missing parameter server password: srv=marius");
    if(!isset($_GET['cam'])) die("missing parameter camera token: cam=token");

    //ask for token
    $token = @file_get_contents("http://localhost:8889/{$MAC}?token");
    if(strlen($token))
    {
        // salt the token + cam token + srv password
        $sig = md5($token.$_GET['cam']."-".$_GET['srv']);
        echo "9cb654c5f074?auth={$sig}<br>";
        echo "<img width='640px' src='http://localhost:8889/9cb654c5f074?auth={$sig}' />";
    }
    echo "<br><li>No image.. refresh the page!<br>";

?>
