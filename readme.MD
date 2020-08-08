### R-PI Linux Camera, Web Cam Streaming, Time lapse, Motion
### A light version of liveimage project with cloud support.


 ![last_demo](https://github.com/comarius/pisica/blob/master/pisicaweb/pisicacon.png)



[![Donate](https://img.shields.io/badge/Donate-PayPal-green.svg)](https://www.paypal.com/cgi-bin/webscr?cmd=_s-xclick&hosted_button_id=L9RVWU5NUZ4YG)   [donations are taken care by meeiot domain]


Description: work in progress.

  * No ports open on router
  * Lag under 1/4 of a second
  * Played with any browser
  * md5 + symetric encryption
  * Motion, Time lapse, Live Video. (no audio) 
    
    
### Brief
   * start client
   * start server
   * open the browser to pisica.php
     
### Updates 8 Aug 2020

#### Prerequisites
    * qtcreator good to have
    * make server
    * make client
    
##### Start server

```
./pisiscasrv camera_port web_port server_password camera_password numeric_camera_token

```

##### Start a camera


```
./pisiscacam server_url_or_ip:camera_port(same as above) server_password camera_password numeric_camera_token /dev/videoX
# ? I dony know whhere I am using the password. Ill come back later with explanations, but for now only the token is used.

```

##### Copy the demo pisica.php to the server.
###### Server prerequisites
    * A web server apache lightpd nginx
    * Access the web page pisica.php
```
 # use camera mac to get to the specific camera and the token to get authenticated by the camea stream
 http://yourserver_ip/pisica.php?srv=server_password&cam=numeric_camera_token&mac=00006B8B4567&port=web_port
 # where mac is printed when you run 'pisiscacam'
```

#####  Demo image Aug 8 2020
 
 ![last_demo](https://raw.githubusercontent.com/comarius/pisica/master/pisicaweb/lastfix.png)
 

https://youtu.be/Tq8YrUxraMg



