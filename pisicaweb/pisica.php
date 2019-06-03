<?php
    if(!isset($_GET['srv'])) die("missing parameter server password: srv=marius");
    if(!isset($_GET['cam'])) die("missing parameter camera token: cam=token");
    if(!isset($_GET['mac'])) die("missing parameter camera token: mac=mak");

    //ask for token
    $MAC = $_GET['mac'];
    $thisip = $_SERVER['SERVER_ADDR'];
    $token = @file_get_contents("http://{$thisip}:8889/{$MAC}?token");
    if(strlen($token))
    {
        // salt the token + cam token + srv password
        $sig = md5($token.$_GET['cam']."-".$_GET['srv']);
        echo "{$MAC}?auth={$sig}<br>";
        echo "<img width='640px' src='http://{$thisip}:8889/{$MAC}?auth={$sig}' />";
    }
    else
    {
        echo "<br><li>No image.. refresh the page!<br>";
    }
?>
