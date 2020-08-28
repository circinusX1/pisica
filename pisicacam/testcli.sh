!#/bin/bash
echo "start the testserver.sh"
./pisicacam  localhost:8000 server _cam /dev/video0
# use cam password starting with _ for web friendly with full access links
