# Video-efects
Add efects and filters on videos

	Project used to apply three different filters on a video :
		- negative
		- sepia
		- blur


	The video is splitted in frames, read/write using pipes and stored in memory in an
unsigned char array. 
	There are 4 different implementation :
		- sequential
		- parallel using OpenMP
		- parallel using MPI
		- parallel using pthreads

	In parallel implementaion, each thread/process receive a chunk of frames from the
entire video, apply desired filter on it and recompose the video.

application on your PC.

	Find run*.sh scripts to run all four implementaion. You can find an mp4 file that
is used for tests.

	Open scripts to find the arguments.