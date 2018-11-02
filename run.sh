#!/bin/bash

if [ $# -eq 0 ]
then
	echo "Usage : ./run.sh example.mp4"
	exit
fi

# get the width and height of the video
echo "Get the video width and height..."
width=$(mediainfo --fullscan $1  | grep Sampled_Width | sed 's/^.*: \(.*\)$/\1/')
height=$(mediainfo --fullscan $1  | grep Sampled_Height | sed 's/^.*: \(.*\)$/\1/')

echo "+++++++++++++++++++++++++++++++++++++++"
echo "Video resolution is : $width x $height"
echo "Start application... "
#compile
make

#run
./filter $width $height

#clean
make clean
