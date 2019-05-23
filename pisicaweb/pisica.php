
<html>
  <head>
  </head>
  <body bgcolor="#E6E6FA">
    <center>
<?php

    $fc = file_get_contents("http://{$_SERVER['HTTP_HOST']}:8889");
    if(strstr($fc,"img"))
    {
        $lines= explode("\r\n",$fc);
        foreach($lines as $l)
        {
            if(strstr($l,"img"))
            {
                $link=str_replace("&#60;","<",$l);
                $link=str_replace("&#62;",">",$link);
                $link=str_replace("127.0.0.1",$_SERVER['HTTP_HOST'],$link);
                echo $link;
            }
        }
    }
?>
    </center>
  </body>
</html>

