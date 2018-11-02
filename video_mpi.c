#include "mpi.h"
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <limits.h>
#define  MASTER		0

int main (int argc, char *argv[])
{
	int numtasks, taskid, *send_counts, *displs;
	int width, height, frame_size;
	unsigned char *frame, *thread_frame;
	FILE *in, *out;

	/*
	 * Check if the application was start corectly
	 */
	if (argc != 3) {
		printf("Usage : ./filter width height\n");
		exit(-1);
	}

	MPI_Init(&argc, &argv);
	MPI_Comm_size(MPI_COMM_WORLD, &numtasks);
	MPI_Comm_rank(MPI_COMM_WORLD,&taskid);

	send_counts = (int *)calloc(numtasks, sizeof(int));
	displs = (int *)calloc(numtasks, sizeof(int));
	width = atoi(argv[1]);
	height = atoi(argv[2]);
	frame_size = height * width * 3;

	/*
	 * The masteropen the input and output pipes for video frames communication.
	 * After opening the input pipe, it will read a frame from the vide, check
	 * it's size and make the properly division to the each process.
	 */ 
	if (taskid == MASTER) {
		frame = (unsigned char *)calloc(frame_size, sizeof(unsigned char));
		if (!frame)
			exit(-1);

		/*
		 * Video streams will be read using pipes. A pipe can only be used
		 * for read or for write because it's unidirectional. Flags :
		 * -i 		: specify the input file
		 * -f 		: force the input format to pipe
		 * -vcodec 	: set the video codec
		 * -pix_fmt	: set the pixels format. In our case rgb24 means that 
		 * we have 3 components with 24 bits (1 byte per color)
		 * 
		 */
		in = popen("ffmpeg -i teapot.mp4 -f image2pipe -vcodec rawvideo -pix_fmt rgb24 -", "r");
		if (!in) {
			printf("Error in opening pipe for reading streams\n");
			exit(-1);
		}

		/*
		 * Flags for output pipe :
		 * -y		: ensure overwrite if necessary
		 * -f 		: set the pipe infos back to a video
		 * -vcodec 	: set the video codec
		 * -pix_fmt	: set the pixels format. In our case rgb24 means that 
		 * we have 3 components with 24 bits (1 byte per color)
		 * -s 		: set the frame rezolution
		 * -r 		: frames per second
		 * -i 		: read data from stdin
		 * -f 		: video format
		 * -an 		: no audio in the file
		 */
		out = popen("ffmpeg -y -f rawvideo -vcodec rawvideo -pix_fmt rgb24 -s 1280x720 -r 30 -i - -f mp4 -q:v 5 -an -vcodec mpeg4 output.mp4", "w");
		if (!out) {
			pclose(in);
			printf("Error in opening pipe for output streams\n");
			exit(-1);
		}

		/* Compute the number of bytes each thread will process */
		/* Compute index in frame to start sending bytes to process */
		for (int i = 0; i < numtasks; i++) {
			if (i != numtasks - 1) {
				send_counts[i] = frame_size / numtasks;
				displs[i] = i * (frame_size / numtasks);
			} else {
				/* last thread gets what's remaining */
				send_counts[i] = frame_size / numtasks + frame_size % numtasks;
				displs[i] = i * (frame_size / numtasks);
			}
		}
		/* Send the arrays to each thread, and also width and height of the frame */
		for (int thr = 1; thr < numtasks; thr++) {
			MPI_Send(send_counts, numtasks, MPI_INT, thr, 0, MPI_COMM_WORLD);
			MPI_Send(displs, numtasks, MPI_INT, thr, 0, MPI_COMM_WORLD);
		}
	} else {
		/* 
		 * Each thread receive information about all threads chunk sizes
		 * and indexes in initial frame. Arrays computed by MASTER.
		 */
		MPI_Recv(send_counts, numtasks, MPI_INT, 0, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
		MPI_Recv(displs, numtasks, MPI_INT, 0, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
	}

	/* alloc a buffer for each thread to receive chunks from frame */
	thread_frame = (unsigned char *)calloc(send_counts[taskid], sizoef(unsigned char*));
	if (!thread_frame) {
		printf("Falt to alloc thread frame buffer. Exit !\n");
		exit(-1);
	}

	/* perform read, process, and write */
	for (;;) {
		if (taskid == MASTER) {
			count = fread(frame, 1, frame_size, in);
			if (count != frame_size)
				break;
		}
	}
	// free memory
	free(send_counts);
	free(displs);
	/*
	 * Master close opened pipes
	 */
	if (taskid == MASTER) {
		fflush(in);
		pclose(in);
		fflush(out);
		pclose(out);
	}

	MPI_Finalize();
	return 0;
}