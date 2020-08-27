#!/bin/bash
g++ -Wall -std=c++11 -I../common -I./ *.cpp ../common/*.cpp -ljpeg -lv4l2  -lpthread -o pisicacam

