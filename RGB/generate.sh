#!/bin/bash

octave --no-gui RGB.m
for i in $(ls -1 ./data/); do cat ./data/$i |tail +2; done >rgb.txt

echo "place rgb.txt in the same folder as mypaint exe"
