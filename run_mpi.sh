#!/bin/bash

if [ $# -eq 0 ]
then
        echo "Usage : ./run.sh example.mp4 processes_number"
        exit
fi

# get the width and height of the video
echo "Get the video width, height and frames number..."
width=$(mediainfo --fullscan $1  | grep Sampled_Width | sed 's/^.*: \(.*\)$/\1/')
height=$(mediainfo --fullscan $1  | grep Sampled_Height | sed 's/^.*: \(.*\)$/\1/')
nr_frm=$(ffprobe -select_streams v -show_streams $1 | grep nb_frames | cut -d "=" -f 2)
echo "+++++++++++++++++++++++++++++++++++++++"
echo "Video resolution is : $width x $height, frames number : $nr_frm"
echo "Start application... "

#compile
make -f Makefile_mpi

#run
mpirun -np $2 ./effect $width $height $nr_frm

#clean
make -f Makefile_mpi clean

