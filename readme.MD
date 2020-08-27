### Linux camera, Web Cam Streaming Sharing Server
#### 


[![Donate](https://img.shields.io/badge/Donate-PayPal-green.svg)](https://www.paypal.com/cgi-bin/webscr?cmd=_s-xclick&hosted_button_id=L9RVWU5NUZ4YG)   [donations are taken care by meeiot domain]


Description: work in progress.

  * No ports open on router
  * Lag under 1/4 of a second
  * Played with any browser
  * symetric encryption
      * Streaming (working)
      * Motion Detection (to be debunked, this is 15 months old project)
      * Time Lapse (likewise, 15 months old project and I have not commented the code)
      * Recording (likewise,  ... and I have not commented the damn code)
    
    
### Brief
   * compile both projects. There is a compile.sh script in pisicasrv. Adapt it for pisicam too.
   * start client
   * start server
   * open the browser to server:port
     
### Updates 8 Aug 2020

#### Prerequisites
    * qtcreator good to have
    * get a server on the net. Eg contabo.net
![last_demo](https://raw.githubusercontent.com/circinusX1/pisica/master/docs/pisi0.png)
    * clone the git and build the server
![last_demo](https://raw.githubusercontent.com/circinusX1/pisica/master/docs/pisi1.png)

    * make client on the pc
    
    
##### Start server
![last_demo](https://raw.githubusercontent.com/circinusX1/pisica/master/docs/pisi2.png)


##### Start a camera
###### ./pisicacam SERVER_IP:CAM_PORT somepassword camerapas /dev/yourcam_device

![last_demo](https://raw.githubusercontent.com/circinusX1/pisica/master/docs/pisi4.png)


##### Open the browsers  http://SERVEIP:SRV_PORT
###### navigate

![last_demo](https://raw.githubusercontent.com/circinusX1/pisica/master/docs/pisi6.png)

The link has the password inthe query for demo purposes

![last_demo](https://raw.githubusercontent.com/circinusX1/pisica/master/docs/pisi7.png)

##### 'Utub

https://www.youtube.com/watch?v=tHoIRJGTjPk


###  Other cool projects

[reverse ssh as a service](http://www.mylinuz.com)

[key value database as a service](https://www.meeiot.org)

