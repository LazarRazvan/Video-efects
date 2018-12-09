#!/bin/bash

if [ $# -lt 3 ]
then
        echo "Usage : ./run_lpt.sh example.mp4 threads_number filter"
        echo "Filter can be sepia/negative/blur"
        exit
fi

# get the width, height and frames number of the video
echo "Get the video width, height and frames number..."
width=$(mediainfo --fullscan $1  | grep Sampled_Width | sed 's/^.*: \(.*\)$/\1/')
height=$(mediainfo --fullscan $1  | grep Sampled_Height | sed 's/^.*: \(.*\)$/\1/')
nr_frm=$(ffprobe -select_streams v -show_streams $1 | grep nb_frames | cut -d "=" -f 2)
filter=$3
echo "+++++++++++++++++++++++++++++++++++++++"
echo "Video resolution is : $width x $height, frames number : $nr_frm"
echo "Start application... "

#compile
make -f Makefile_lpt

#run
./effect $width $height $nr_frm $filter $2

#clean
make -f Makefile_lpt clean
