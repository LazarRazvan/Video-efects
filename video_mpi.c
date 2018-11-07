#include "mpi.h"
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <limits.h>
#include <string.h>
#define  MASTER		0

/*
 * Function receive a frame and apply effects to it.
 * Frames are stored using a 3D matrix, but instead of matrix we use
 * a linearized array.
 * You can see the offset of eache pixel_{r,g,b} taken out from the
 * liniarized array
 */
void apply_negative_on_frame(unsigned char *frame, int frame_size, int width) {
	int pixel_r, pixel_g, pixel_b, height;

	height = frame_size / (width * 3);

	for (int i = 0; i < height; i++)
		for (int j = 0; j < width; j++) {
			pixel_r = i * (width * 3) + j * 3 + 0;
			pixel_g = i * (width * 3) + j * 3 + 1;
			pixel_b = i * (width * 3) + j * 3 + 2;
			frame[pixel_r] = 255 - frame[pixel_r];
			frame[pixel_g] = 255 - frame[pixel_g];
			frame[pixel_b] = 255 - frame[pixel_b];
		}
}

void apply_sepia_on_frame(unsigned char *frame, int frame_size, int width) {
	int pixel_r, pixel_g, pixel_b, height;

	height = frame_size / (width * 3);

	for (int i = 0; i < height; i++)
		for (int j = 0; j < width; j++) {
			int tr, tg, tb;
			pixel_r = i * (width * 3) + j * 3 + 0;
			pixel_g = i * (width * 3) + j * 3 + 1;
			pixel_b = i * (width * 3) + j * 3 + 2;
			tr = (frame[pixel_r] * 0.393) + (frame[pixel_g] * 0.769) + (frame[pixel_b] * 0.189);
			tg = (frame[pixel_r] * 0.349) + (frame[pixel_g] * 0.686) + (frame[pixel_b] * 0.168);
			tb = (frame[pixel_r] * 0.272) + (frame[pixel_g] * 0.534) + (frame[pixel_b] * 0.131);

			if (tr > 255)
				frame[pixel_r] = 255;
			else
				frame[pixel_r] = tr;
			if (tg > 255)
				frame[pixel_g] = 255;
			else
				frame[pixel_g] = tg;
			if (tb > 255)
				frame[pixel_b] = 255;
			else
				frame[pixel_b] = tb;
		}
}

int main (int argc, char *argv[])
{
	int numtasks, taskid, *send_counts, *displs;
	int width, height, frame_size, frames_nr;
	unsigned char *frame, *thread_frame;
	char *filter;
	FILE *in, *out;
	clock_t start, end;
	double time;

	/*
	 * Check if the application was start corectly
	 */
	if (argc != 5) {
		printf("Usage : ./filter width height frames_number filter\n");
		exit(-1);
	}

	MPI_Init(&argc, &argv);
	MPI_Comm_size(MPI_COMM_WORLD, &numtasks);
	MPI_Comm_rank(MPI_COMM_WORLD,&taskid);

	send_counts = (int *)calloc(numtasks, sizeof(int));
	displs = (int *)calloc(numtasks, sizeof(int));
	width = atoi(argv[1]);
	height = atoi(argv[2]);
	frames_nr = atoi(argv[3]);
	filter = strdup(argv[4]);
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
	thread_frame = (unsigned char *)calloc(send_counts[taskid], sizeof(unsigned char*));
	if (!thread_frame) {
		printf("Falt to alloc thread frame buffer. Exit !\n");
		exit(-1);
	}

	/* start counting the time */
	if (taskid == MASTER)
		start = clock();

	if (strcmp(filter, "sepia") == 0) {
		for (int  frm_nr = 0; frm_nr < frames_nr; frm_nr++) {
			if (taskid == MASTER) {
				/* read a frame from pipe */
				fread(frame, 1, frame_size, in);
			}
			/* send chunks to all processes */
			MPI_Scatterv(frame, send_counts, displs, MPI_CHAR, thread_frame, send_counts[taskid], MPI_CHAR, 0, MPI_COMM_WORLD);

			/* each process perform changes of chunk of data it receive */
			apply_negative_on_frame(thread_frame, send_counts[taskid], width);

			/* send data back to master */
			MPI_Gatherv(thread_frame, send_counts[taskid], MPI_CHAR, frame, send_counts, displs, MPI_CHAR, 0, MPI_COMM_WORLD);

			/* master write frame on output pipe */
			if (taskid == MASTER) {
				/* read a frame from pipe */
				fwrite(frame, 1, frame_size, out);
			}
		}
	}
	else if (strcmp(filter, "negative") == 0) {
		for (int  frm_nr = 0; frm_nr < frames_nr; frm_nr++) {
			if (taskid == MASTER) {
				/* read a frame from pipe */
				fread(frame, 1, frame_size, in);
			}
			/* send chunks to all processes */
			MPI_Scatterv(frame, send_counts, displs, MPI_CHAR, thread_frame, send_counts[taskid], MPI_CHAR, 0, MPI_COMM_WORLD);

			/* each process perform changes of chunk of data it receive */
			apply_sepia_on_frame(thread_frame, send_counts[taskid], width);

			/* send data back to master */
			MPI_Gatherv(thread_frame, send_counts[taskid], MPI_CHAR, frame, send_counts, displs, MPI_CHAR, 0, MPI_COMM_WORLD);

			/* master write frame on output pipe */
			if (taskid == MASTER) {
				/* read a frame from pipe */
				fwrite(frame, 1, frame_size, out);
			}
		}
	}
	else if (strcmp(filter, "blur") == 0) {

	}
	else {
		printf("Please choose a filter from sepia/negative/blur\n");
		exit(-1);
	}
	

	/* stop counting the time */
	if (taskid == MASTER) {
		end = clock();
		time = (double)(end - start ) / CLOCKS_PER_SEC;
		printf("-----------------------------------\n");
		printf("| Time with %d procs : %lf    |\n", numtasks, time);
		printf("-----------------------------------\n");
		free(frame);
	}
	// free memory
	free(send_counts);
	free(displs);
	free(thread_frame);
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