#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <limits.h>
#include <string.h>

#define NEGATIVE	1
#define SEPIA		2
#define BLUR		3

/*
 * Structure used by every thread at start.
 * thread_frame : raw data of pixels
 * width		: the width of a frame 
 * frame_size	: dimension of a thread frame
 * filter		: filter to be applied
 */
typedef struct thread_info {
	unsigned char *thread_frame;
	int send_count, displs, width, filter_id;
} thread_info;