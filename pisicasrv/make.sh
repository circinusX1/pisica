#!/bin/bash
g++ -Wall -std=c++11 -I../common -I./ *.cpp ../common/*.cpp -lpthread -o pisicasrv

