### Linux camera, Webcam sharing streaming server relay. Multiple cameras, multiple views.


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
######  ./pisicasrv CAM_PORT WEB_PORT some_server_password

![last_demo](https://raw.githubusercontent.com/circinusX1/pisica/master/docs/pisi2.png)


##### Start the camera client locally on a PC with a webcam or a raspberry PI
###### ./pisicacam SERVER_IP:CAM_PORT server_password camera_password /dev/yourcam_device

![last_demo](https://raw.githubusercontent.com/circinusX1/pisica/master/docs/pisi4.png)


##### Open the browsers  http://SERVEIP:SRV_PORT
###### navigate
  * On local http there can be links to pisicasrv ports and provide the link as:
  * http://localhost:SERVER_PORT/CAMERA_MAC?server=server_password&camera=camera_password&image=1
      * where CAMERA_MAC is the camera token printed when you start the camera
      * server_password is the server password you provided as a command line parameter when you started the pisecasrv
      * camera_password is the password you provided when you started the camera

![last_demo](https://raw.githubusercontent.com/circinusX1/pisica/master/docs/pisi6.png)

The link has the password inthe query for demo purposes

![last_demo](https://raw.githubusercontent.com/circinusX1/pisica/master/docs/pisi7.png)

##### 'Utub

[youtube video](https://www.youtube.com/watch?v=rDwoClB92no)
[![YOUTUBE](https://img.youtube.com/vi/watch?v=rDwoClB92no/0.jpg)](https://img.youtube.com/vi/watch?v=rDwoClB92no/0.jpg)

###  Other cool projects

[reverse ssh as a service](http://www.mylinuz.com)

[key value database as a service](https://www.meeiot.org)

