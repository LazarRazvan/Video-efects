#!/bin/bash

if [ $# -eq 0 ] || [ $# -eq 1 ]
then
    echo "Usage : ./run.sh example.mp4 filter"
    echo "Filter can be sepia/negative/blur"
    exit
fi

# get the width and height of the video
echo "Get the video width and height..."
width=$(mediainfo --fullscan $1  | grep Sampled_Width | sed 's/^.*: \(.*\)$/\1/')
height=$(mediainfo --fullscan $1  | grep Sampled_Height | sed 's/^.*: \(.*\)$/\1/')
filter=$2

echo "+++++++++++++++++++++++++++++++++++++++"
echo "Video resolution is : $width x $height"
echo "Start application... "
#compile
make

#run
./effect $width $height $filter

#clean
make clean