#!/bin/bash
g++ -I../common -I./ -lpthread  -ljpeg -lv4l2 *.cpp -o pisicacam

