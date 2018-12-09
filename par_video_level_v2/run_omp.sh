#!/bin/bash

if [ $# -lt 3 ]
then
    echo "Usage : ./run_omp.sh example.mp4 filter num_threads"
    echo "Filter can be sepia/negative/blur"
    exit
fi

# get the width and height of the video
echo "Get the video width and height..."
width=$(mediainfo --fullscan $1  | grep Sampled_Width | sed 's/^.*: \(.*\)$/\1/')
height=$(mediainfo --fullscan $1  | grep Sampled_Height | sed 's/^.*: \(.*\)$/\1/')
nr_frm=$(ffprobe -select_streams v -show_streams $1 | grep nb_frames | cut -d "=" -f 2)
filter=$2
nr_threads=$3

echo "+++++++++++++++++++++++++++++++++++++++"
echo "Video resolution is : $width x $height"
echo "Start application... "
#compile
make -f Makefile_omp

#run
./effect $width $height $nr_frm $filter $nr_threads

#clean
make -f Makefile_omp clean
