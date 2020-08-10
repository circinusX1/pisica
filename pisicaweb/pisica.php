Usage: srv=localhost&cam=token&mac=macaddr&port=5555
<?php
    if(!isset($_GET['srv'])) die("missing parameter server password: srv=marius");
    if(!isset($_GET['cam'])) die("missing parameter camera token: cam=token");
    if(!isset($_GET['mac'])) die("missing parameter camera token: mac=mak");
    if(!isset($_GET['port'])) die("missing parameter server port: port=port");

    //ask for token
    $MAC = $_GET['mac'];
    $PORT = $_GET['port'];
    $thisip = $_SERVER['SERVER_ADDR'];
    echo "<br>getting http://{$thisip}:{$PORT}/{$MAC}?token<br>";
    $token = @file_get_contents("http://{$thisip}:{$PORT}/{$MAC}?token");
    if(strlen($token))
    {
        // salt the token + cam token + srv password
        $sig = md5($token.$_GET['cam']."-".$_GET['srv']);
        echo "GOT: {$token}{$MAC}?auth={$sig}<br>";
        echo "getting <img width='640px' src='http://{$thisip}:{$PORT}/{$MAC}?auth={$sig}<br>";
        echo "<img width='640' heigth='480' src='http://{$thisip}:{$PORT}/{$MAC}?auth={$sig}' />";
        //echo "<img width='640px' src='http://{$thisip}:{$PORT}/?mac={$MAC}&auth={$sig}' />";
        //echo "<img width='640px' src='http://{$thisip}:{$PORT}/{$MAC}?auth={$sig}' />";
    }
    else
    {
        echo "<br><li>No image.. refresh the page!<br>";
    }
?>
